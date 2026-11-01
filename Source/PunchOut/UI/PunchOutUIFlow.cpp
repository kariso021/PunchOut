#include "PunchOutUIFlow.h"

#include "PunchOutUIView.h"
#include "PunchOutViewModel.h"
#include "Player/PunchOutPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

void UPunchOutGameInstance::Init()
{
	Super::Init();
	NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
	TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
}

void UPunchOutGameInstance::Shutdown()
{
	GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	Super::Shutdown();
}

void UPunchOutGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Message)
{
	if (World != GetWorld()) return;
	ShowMainMenu = true;
	if (auto* Controller = Cast<APunchOutUIPlayerController>(GetFirstLocalPlayerController()))
	{
		Controller->ReportConnectionFailure(Message);
	}
}

void UPunchOutGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Message)
{
	HandleNetworkFailure(World, nullptr, ENetworkFailure::FailureReceived, Message);
}

void APunchOutGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APunchOutGameState, MatchStarted);
}

APunchOutGameMode::APunchOutGameMode()
{
	GameStateClass = APunchOutGameState::StaticClass();
	PlayerControllerClass = APunchOutUIPlayerController::StaticClass();
	PlayerStateClass = APunchOutPlayerState::StaticClass();
	static ConstructorHelpers::FClassFinder<APawn> Character(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	DefaultPawnClass = Character.Class;
}

APunchOutUIPlayerController::APunchOutUIPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APunchOutUIPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	if (auto* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (const TCHAR* Path : {TEXT("/Game/Input/IMC_Default.IMC_Default"), TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook")})
		{
			if (auto* Mapping = LoadObject<UInputMappingContext>(nullptr, Path)) Input->AddMappingContext(Mapping, 0);
		}
	}
	// Wait until travel has finished installing the local player and viewport.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]
	{
		ViewModel = NewObject<UPunchOutViewModel>(this);
		ViewModel->HostCommand.BindUObject(this, &ThisClass::HostGame);
		ViewModel->JoinCommand.BindUObject(this, &ThisClass::JoinGame);
		ViewModel->StartCommand.BindUObject(this, &ThisClass::StartGame);
		ViewModel->ResumeCommand.BindUObject(this, &ThisClass::ResumeGame);
		ViewModel->LeaveCommand.BindUObject(this, &ThisClass::LeaveGame);
		View = CreateWidget<UPunchOutUIView>(this, UPunchOutUIView::StaticClass());
		View->SetViewModel(ViewModel);
		View->AddToViewport();
		RefreshViewModel();
		ApplyInputState();
	}));
}

void APunchOutUIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::ToggleMenu);
}

void APunchOutUIPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!ViewModel) return;
	RefreshElapsed += DeltaTime;
	if (RefreshElapsed >= 0.1f)
	{
		RefreshElapsed = 0;
		RefreshViewModel();
	}
}

void APunchOutUIPlayerController::RefreshViewModel()
{
	if (ViewModel->IsConnecting) return;
	const auto* Instance = GetGameInstance<UPunchOutGameInstance>();
	const auto* State = GetWorld()->GetGameState<APunchOutGameState>();
	const EPunchOutScreen OldScreen = ViewModel->Screen;
	if (Instance->ShowMainMenu) ViewModel->SetScreen(EPunchOutScreen::MainMenu);
	else if (!State || !State->MatchStarted) ViewModel->SetScreen(EPunchOutScreen::Lobby);
	else if (OldScreen != EPunchOutScreen::PauseMenu) ViewModel->SetScreen(EPunchOutScreen::Playing);
	ViewModel->SetIsHost(HasAuthority());
	ViewModel->SetPlayerCount(State ? State->PlayerArray.Num() : 0);
	if (const auto* PlayerCharacter = Cast<ACharacter>(GetPawn()))
	{
		ViewModel->SetPosture(FText::FromString(PlayerCharacter->bIsCrouched ? TEXT("Crouching") : TEXT("Standing")));
	}
	if (ViewModel->Screen != OldScreen) ApplyInputState();
}

void APunchOutUIPlayerController::ApplyInputState()
{
	const bool IsPlaying = ViewModel->Screen == EPunchOutScreen::Playing;
	bShowMouseCursor = !IsPlaying;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	SetIgnoreMoveInput(!IsPlaying);
	SetIgnoreLookInput(!IsPlaying);
	if (IsPlaying)
	{
		SetInputMode(FInputModeGameOnly());
	}
	else
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(View->TakeWidget());
		SetInputMode(Mode);
	}
}

void APunchOutUIPlayerController::ToggleMenu()
{
	if (!ViewModel) return;
	if (ViewModel->Screen == EPunchOutScreen::Playing) ViewModel->SetScreen(EPunchOutScreen::PauseMenu);
	else if (ViewModel->Screen == EPunchOutScreen::PauseMenu) ViewModel->SetScreen(EPunchOutScreen::Playing);
	else return;
	ApplyInputState();
}

void APunchOutUIPlayerController::HostGame()
{
	if (ViewModel->Screen != EPunchOutScreen::MainMenu || ViewModel->IsConnecting) return;
	ViewModel->SetIsConnecting(true);
	GetGameInstance<UPunchOutGameInstance>()->ShowMainMenu = false;
	UGameplayStatics::OpenLevel(this, TEXT("/Game/Levels/L_OfficePrototype"), true, TEXT("listen"));
}

void APunchOutUIPlayerController::JoinGame(const FString& Address)
{
	if (ViewModel->Screen != EPunchOutScreen::MainMenu || ViewModel->IsConnecting) return;
	const FString Target = Address.TrimStartAndEnd();
	if (Target.IsEmpty() || Target.Contains(TEXT("/")) || Target.Contains(TEXT("?")) || Target.Contains(TEXT(" ")))
	{
		ViewModel->SetStatus(FText::FromString(TEXT("Enter a host address, for example 127.0.0.1:7777.")));
		return;
	}
	GetGameInstance<UPunchOutGameInstance>()->ShowMainMenu = false;
	ViewModel->SetIsConnecting(true);
	ViewModel->SetStatus(FText::FromString(TEXT("Connecting...")));
	ClientTravel(Target, TRAVEL_Absolute);
}

void APunchOutUIPlayerController::StartGame()
{
	if (!HasAuthority() || ViewModel->Screen != EPunchOutScreen::Lobby) return;
	if (auto* State = GetWorld()->GetGameState<APunchOutGameState>())
	{
		State->MatchStarted = true;
		State->ForceNetUpdate();
		RefreshViewModel();
	}
}

void APunchOutUIPlayerController::ResumeGame()
{
	if (ViewModel->Screen != EPunchOutScreen::PauseMenu) return;
	ViewModel->SetScreen(EPunchOutScreen::Playing);
	ApplyInputState();
}

void APunchOutUIPlayerController::LeaveGame()
{
	GetGameInstance<UPunchOutGameInstance>()->ShowMainMenu = true;
	UGameplayStatics::OpenLevel(this, TEXT("/Game/Levels/L_OfficePrototype"));
}

void APunchOutUIPlayerController::ReportConnectionFailure(const FString& Message)
{
	if (!ViewModel) return;
	ViewModel->SetIsConnecting(false);
	ViewModel->SetStatus(FText::FromString(Message));
	ViewModel->SetScreen(EPunchOutScreen::MainMenu);
	ApplyInputState();
}
