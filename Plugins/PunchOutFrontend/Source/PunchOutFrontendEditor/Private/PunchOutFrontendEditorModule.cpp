#include "PunchOutUIView.h"
#include "PunchOutViewModel.h"

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

namespace
{
	const FName PreviewTabId(TEXT("PunchOutUIPreview"));
	constexpr uint32 PreviewPort = 8088;

	bool IsLocalRequest(const FHttpServerRequest& Request)
	{
		const auto* Host = Request.Headers.Find(TEXT("host"));
		if (!Host || Host->Num() != 1 ||
			((*Host)[0] != TEXT("127.0.0.1:8088") && (*Host)[0] != TEXT("localhost:8088")))
		{
			return false;
		}

		const auto* Origin = Request.Headers.Find(TEXT("origin"));
		return !Origin || (Origin->Num() == 1 && (*Origin)[0] == TEXT("http://") + (*Host)[0]);
	}

	bool ApplyState(const FHttpServerRequest& Request, UPunchOutViewModel& Model)
	{
		if (Request.Body.IsEmpty() || Request.Body.Num() > 4096)
		{
			return false;
		}

		const FUTF8ToTCHAR Body(reinterpret_cast<const ANSICHAR*>(Request.Body.GetData()), Request.Body.Num());
		TSharedPtr<FJsonObject> State;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(Body.Length(), Body.Get())), State))
		{
			return false;
		}

		FString Screen, Status, Posture;
		double PlayerCount;
		bool bIsHost, bIsConnecting;
		if (!State->TryGetStringField(TEXT("screen"), Screen) ||
			!State->TryGetNumberField(TEXT("playerCount"), PlayerCount) ||
			!State->TryGetBoolField(TEXT("isHost"), bIsHost) ||
			!State->TryGetBoolField(TEXT("isConnecting"), bIsConnecting) ||
			!State->TryGetStringField(TEXT("status"), Status) ||
			!State->TryGetStringField(TEXT("posture"), Posture) ||
			PlayerCount < 0 || PlayerCount > 999 || FMath::FloorToDouble(PlayerCount) != PlayerCount ||
			Status.Len() > 256 || Posture.Len() > 128)
		{
			return false;
		}

		const int64 ScreenValue = StaticEnum<EPunchOutScreen>()->GetValueByNameString(Screen);
		if (ScreenValue < 0 || ScreenValue > static_cast<int64>(EPunchOutScreen::PauseMenu))
		{
			return false;
		}

		Model.SetScreen(static_cast<EPunchOutScreen>(ScreenValue));
		Model.SetPlayerCount(static_cast<int32>(PlayerCount));
		Model.SetIsHost(bIsHost);
		Model.SetIsConnecting(bIsConnecting);
		Model.SetStatus(FText::FromString(Status));
		Model.SetPosture(FText::FromString(Posture));
		return true;
	}

	FString ReadState(const UPunchOutViewModel& Model, bool bLive)
	{
		const auto State = MakeShared<FJsonObject>();
		State->SetStringField(TEXT("screen"), StaticEnum<EPunchOutScreen>()->GetNameStringByValue(static_cast<int64>(Model.Screen)));
		State->SetNumberField(TEXT("playerCount"), Model.PlayerCount);
		State->SetBoolField(TEXT("isHost"), Model.IsHost);
		State->SetBoolField(TEXT("isConnecting"), Model.IsConnecting);
		State->SetStringField(TEXT("status"), Model.Status.ToString());
		State->SetStringField(TEXT("posture"), Model.Posture.ToString());
		State->SetStringField(TEXT("target"), bLive ? TEXT("live") : TEXT("preview"));

		FString Json;
		FJsonSerializer::Serialize(State, TJsonWriterFactory<>::Create(&Json));
		return Json;
	}

	UPunchOutUIView* FindGameView()
	{
		UPunchOutUIView* FirstView = nullptr;
		for (TObjectIterator<UPunchOutUIView> View; View; ++View)
		{
			const UWorld* World = View->GetWorld();
			if (!World || World->WorldType != EWorldType::PIE || !View->IsInViewport() || !View->GetDisplayViewModel())
			{
				continue;
			}
			if (GEditor && World == GEditor->PlayWorld) return *View;
			if (!FirstView) FirstView = *View;
		}
		return FirstView;
	}
}

class FPunchOutFrontendEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FEditorDelegates::BeginPIE.AddRaw(this, &ThisClass::BeginPlay);
		FEditorDelegates::EndPIE.AddRaw(this, &ThisClass::EndPlay);
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PreviewTabId, FOnSpawnTab::CreateRaw(this, &ThisClass::SpawnPreview))
			.SetDisplayName(NSLOCTEXT("PunchOut", "UIPreview", "PunchOut UI Preview"))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());

		OpenCommand = IConsoleManager::Get().RegisterConsoleCommand(TEXT("PunchOut.UIDebug"),
			TEXT("Open the PunchOut UI Preview and its localhost controls."),
			FConsoleCommandDelegate::CreateLambda([] { FGlobalTabmanager::Get()->TryInvokeTab(PreviewTabId); }));
	}

	virtual void ShutdownModule() override
	{
		FEditorDelegates::BeginPIE.RemoveAll(this);
		FEditorDelegates::EndPIE.RemoveAll(this);
		EndPlay(false);
		IConsoleManager::Get().UnregisterConsoleObject(OpenCommand);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PreviewTabId);
		if (const auto Tab = PreviewTab.Pin())
		{
			Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback());
			Tab->RequestCloseTab();
		}
		if (Router)
		{
			if (PageRoute) Router->UnbindRoute(PageRoute);
			if (StateRoute) Router->UnbindRoute(StateRoute);
		}
	}

private:
	using ThisClass = FPunchOutFrontendEditorModule;

	void BeginPlay(bool bIsSimulating)
	{
		bPlayInEditor = !bIsSimulating;
		if (bPlayInEditor) StartServer();
	}

	void EndPlay(bool)
	{
		bPlayInEditor = false;
		if (auto* View = LiveDebugView.Get()) View->SetDebugViewModel(nullptr);
		LiveDebugView.Reset();
	}

	TSharedRef<SDockTab> SpawnPreview(const FSpawnTabArgs&)
	{
		// The native UMG preview needs no world, player controller, or gameplay commands.
		auto* View = NewObject<UPunchOutUIView>(GetTransientPackage());
		View->Initialize();
		auto* Model = NewObject<UPunchOutViewModel>(View);
		View->SetViewModel(Model);
		PreviewModel = Model;

		const auto Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab)[View->TakeWidget()];
		Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([this](TSharedRef<SDockTab>)
		{
			PreviewModel.Reset();
		}));
		PreviewTab = Tab;
		StartServer();
		return Tab;
	}

	void StartServer()
	{
		if (Router) return;
		auto& Server = FHttpServerModule::Get();
		Server.StartAllListeners();
		Router = Server.GetHttpRouter(PreviewPort, true);
		if (!Router)
		{
			UE_LOG(LogTemp, Warning, TEXT("PunchOut UI Preview could not listen on port %u."), PreviewPort);
			return;
		}
		PageRoute = Router->BindRoute(FHttpPath(TEXT("/punchout-ui")), EHttpServerRequestVerbs::VERB_GET,
			FHttpRequestHandler::CreateRaw(this, &ThisClass::ServePage));
		StateRoute = Router->BindRoute(FHttpPath(TEXT("/punchout-ui/state")),
			EHttpServerRequestVerbs::VERB_GET | EHttpServerRequestVerbs::VERB_POST | EHttpServerRequestVerbs::VERB_DELETE,
			FHttpRequestHandler::CreateRaw(this, &ThisClass::ServeState));
		UE_LOG(LogTemp, Display, TEXT("PunchOut UI Preview controls: http://127.0.0.1:8088/punchout-ui"));
	}

	bool ServePage(const FHttpServerRequest& Request, const FHttpResultCallback& Reply)
	{
		if (!IsLocalRequest(Request))
		{
			Reply(FHttpServerResponse::Error(EHttpServerResponseCodes::Forbidden));
			return true;
		}
		const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("PunchOutFrontend"));
		FString Html;
		if (FFileHelper::LoadFileToString(Html, *(Plugin->GetBaseDir() / TEXT("Resources/UIPreview.html"))))
		{
			Reply(FHttpServerResponse::Create(Html, TEXT("text/html")));
		}
		else
		{
			Reply(FHttpServerResponse::Error(EHttpServerResponseCodes::NotFound));
		}
		return true;
	}

	bool ServeState(const FHttpServerRequest& Request, const FHttpResultCallback& Reply)
	{
		if (!IsLocalRequest(Request))
		{
			Reply(FHttpServerResponse::Error(EHttpServerResponseCodes::Forbidden));
			return true;
		}

		auto* GameView = bPlayInEditor ? FindGameView() : nullptr;
		auto* Model = bPlayInEditor ? (GameView ? GameView->GetDisplayViewModel() : nullptr) : PreviewModel.Get();
		if (!Model)
		{
			Reply(FHttpServerResponse::Error(EHttpServerResponseCodes::Conflict,
				bPlayInEditor ? TEXT("Waiting for the PIE game UI.") : TEXT("Start PIE or open PunchOut UI Preview.")));
			return true;
		}

		if (Request.Verb == EHttpServerRequestVerbs::VERB_POST)
		{
			auto* Target = GameView ? NewObject<UPunchOutViewModel>(GameView) : Model;
			if (!ApplyState(Request, *Target))
			{
				Reply(FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest, TEXT("Invalid UI state.")));
				return true;
			}
			if (GameView)
			{
				if (auto* PreviousView = LiveDebugView.Get(); PreviousView && PreviousView != GameView)
				{
					PreviousView->SetDebugViewModel(nullptr);
				}
				GameView->SetDebugViewModel(Target);
				LiveDebugView = GameView;
				Model = Target;
			}
		}
		else if (Request.Verb == EHttpServerRequestVerbs::VERB_DELETE)
		{
			if (GameView)
			{
				GameView->SetDebugViewModel(nullptr);
				LiveDebugView.Reset();
				Model = GameView->GetDisplayViewModel();
			}
			else
			{
				const auto* Defaults = GetDefault<UPunchOutViewModel>();
				Model->SetScreen(Defaults->Screen);
				Model->SetPlayerCount(Defaults->PlayerCount);
				Model->SetIsHost(Defaults->IsHost);
				Model->SetIsConnecting(Defaults->IsConnecting);
				Model->SetStatus(Defaults->Status);
				Model->SetPosture(Defaults->Posture);
			}
		}
		Reply(FHttpServerResponse::Create(ReadState(*Model, GameView != nullptr), TEXT("application/json")));
		return true;
	}

	IConsoleObject* OpenCommand = nullptr;
	TWeakPtr<SDockTab> PreviewTab;
	TWeakObjectPtr<UPunchOutViewModel> PreviewModel;
	TWeakObjectPtr<UPunchOutUIView> LiveDebugView;
	bool bPlayInEditor = false;
	TSharedPtr<IHttpRouter> Router;
	FHttpRouteHandle PageRoute;
	FHttpRouteHandle StateRoute;
};

IMPLEMENT_MODULE(FPunchOutFrontendEditorModule, PunchOutFrontendEditor)
