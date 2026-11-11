#include "RuntimeDebugWebInspector.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/NamedSlotInterface.h"
#include "Components/PanelWidget.h"
#include "Debugging/SlateDebugging.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformMemory.h"
#include "HttpServerRequest.h"
#include "Layout/WidgetPath.h"
#include "Misc/App.h"
#include "Rendering/DrawElementTypes.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SViewport.h"

namespace
{
	TArray<TSharedPtr<FJsonValue>> VectorJson(FVector2D Value)
	{
		return {MakeShared<FJsonValueNumber>(Value.X), MakeShared<FJsonValueNumber>(Value.Y)};
	}

	class SWebWidgetHighlight : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SWebWidgetHighlight) {}
			SLATE_ARGUMENT(UWidget*, Target)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args)
		{
			Target = Args._Target;
			SetVisibility(EVisibility::HitTestInvisible);
		}
	private:
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const auto* Widget = Target.Get();
			if (!Widget || !Widget->IsVisible() || !Widget->GetCachedWidget().IsValid()) return Layer;
			const FGeometry& Bounds = Widget->GetCachedGeometry();
			const FVector2D Size = Bounds.GetLocalSize();
			if (Size.X <= 0 || Size.Y <= 0) return Layer;
			TArray<FVector2f> Points;
			for (FVector2D Corner : {FVector2D(0, 0), FVector2D(Size.X, 0), Size, FVector2D(0, Size.Y), FVector2D(0, 0)})
			{
				Points.Add(FVector2f(Geometry.AbsoluteToLocal(Bounds.LocalToAbsolute(Corner))));
			}
			FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), MoveTemp(Points),
				ESlateDrawEffect::None, FLinearColor::Yellow, true, 2);
			return Layer + 1;
		}
		TWeakObjectPtr<UWidget> Target;
	};
}

FRuntimeDebugWebInspector::FRuntimeDebugWebInspector()
{
#if WITH_SLATE_DEBUGGING
	FSlateDebugging::InputEvent.AddRaw(this, &FRuntimeDebugWebInspector::OnInput);
#endif
}

FRuntimeDebugWebInspector::~FRuntimeDebugWebInspector()
{
#if WITH_SLATE_DEBUGGING
	FSlateDebugging::InputEvent.RemoveAll(this);
#endif
	Reset();
}

void FRuntimeDebugWebInspector::Reset()
{
	SetHighlight(false);
	InspectedWorld.Reset();
	Selected.Reset();
	Widgets.Reset();
	WidgetIds.Reset();
	Roots.Reset();
	LastRefresh = -1;
	bRecordInput = false;
}

void FRuntimeDebugWebInspector::Record(const FString& Message)
{
	const FString Entry = FDateTime::Now().ToString(TEXT("%H:%M:%S")) + TEXT("  ") + Message.Left(512);
	if (Events.Num() < 256)
	{
		Events.Add(Entry);
	}
	else
	{
		Events[EventCursor] = Entry;
		EventCursor = (EventCursor + 1) % 256;
	}
}

void FRuntimeDebugWebInspector::OnInput(const FSlateDebuggingInputEventArgs& Event)
{
	if (!bRecordInput || !InspectedWorld.IsValid() || !Event.HandlerWidget.IsValid() || !FSlateApplication::IsInitialized()) return;
	const auto* Viewport = InspectedWorld->GetGameViewport();
	const auto ViewportWidget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
	if (!ViewportWidget) return;
	FWidgetPath Path;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked(Event.HandlerWidget.ToSharedRef(), Path);
	if (Path.ContainsWidget(ViewportWidget.Get()))
	{
		Record(Event.ToText().ToString());
	}
}

void FRuntimeDebugWebInspector::SetHighlight(bool bEnabled)
{
	if (auto* Viewport = HighlightViewport.Get(); Viewport && HighlightOverlay)
	{
		Viewport->RemoveViewportWidgetContent(HighlightOverlay.ToSharedRef());
	}
	HighlightOverlay.Reset();
	HighlightViewport.Reset();
	if (bEnabled && Selected.IsValid() && InspectedWorld.IsValid())
	{
		if (auto* Viewport = InspectedWorld->GetGameViewport())
		{
			HighlightOverlay = SNew(SWebWidgetHighlight).Target(Selected.Get());
			HighlightViewport = Viewport;
			Viewport->AddViewportWidgetContent(HighlightOverlay.ToSharedRef(), 10000);
		}
	}
}

void FRuntimeDebugWebInspector::Refresh(UWorld* World)
{
	if (InspectedWorld.Get() != World)
	{
		Reset();
		InspectedWorld = World;
	}
	Widgets.Reset();
	Roots.Reset();
	if (World)
	{
		for (TObjectIterator<UUserWidget> Widget; Widget; ++Widget)
		{
			if (Widget->GetWorld() == World && Widget->IsInViewport())
			{
				if (auto Root = BuildWidget(*Widget)) Roots.Add(Root);
			}
		}
	}
	const FString* SelectedId = WidgetIds.Find(Selected);
	if (!Selected.IsValid() || !SelectedId || !Widgets.Contains(*SelectedId))
	{
		SetHighlight(false);
		Selected.Reset();
	}
	for (auto It = WidgetIds.CreateIterator(); It; ++It)
	{
		if (!Widgets.Contains(It.Value())) It.RemoveCurrent();
	}
	LastRefresh = FPlatformTime::Seconds();
}

TSharedPtr<FJsonValue> FRuntimeDebugWebInspector::BuildWidget(UWidget* Widget)
{
	if (!IsValid(Widget)) return nullptr;
	FString& Id = WidgetIds.FindOrAdd(Widget);
	if (Id.IsEmpty()) Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	if (Widgets.Contains(Id)) return nullptr;
	Widgets.Add(Id, Widget);
	const auto Node = MakeShared<FJsonObject>();
	Node->SetStringField(TEXT("id"), Id);
	Node->SetStringField(TEXT("name"), Widget->GetName());
	Node->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
	TArray<UWidget*> Children;
	if (const auto* UserWidget = Cast<UUserWidget>(Widget))
	{
		if (UserWidget->WidgetTree) Children.Add(UserWidget->WidgetTree->RootWidget);
	}
	if (const auto* Panel = Cast<UPanelWidget>(Widget)) Children.Append(Panel->GetAllChildren());
	if (const auto* Slots = Cast<INamedSlotInterface>(Widget))
	{
		TArray<FName> Names;
		Slots->GetSlotNames(Names);
		for (FName Name : Names) Children.Add(Slots->GetContentForSlot(Name));
	}
	TArray<TSharedPtr<FJsonValue>> Nodes;
	for (UWidget* Child : Children)
	{
		if (auto NodeValue = BuildWidget(Child)) Nodes.Add(NodeValue);
	}
	Node->SetArrayField(TEXT("children"), Nodes);
	return MakeShared<FJsonValueObject>(Node);
}

TSharedRef<FJsonObject> FRuntimeDebugWebInspector::Snapshot() const
{
	const auto Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("world"), InspectedWorld.IsValid() ? InspectedWorld->GetName() : TEXT(""));
	Result->SetArrayField(TEXT("widgets"), Roots);
	Result->SetBoolField(TEXT("recordInput"), bRecordInput);
	Result->SetBoolField(TEXT("highlight"), HighlightOverlay.IsValid());
	Result->SetNumberField(TEXT("frameMs"), FApp::GetDeltaTime() * 1000);
	Result->SetNumberField(TEXT("memoryMiB"), FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0));
	TArray<TSharedPtr<FJsonValue>> Log;
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		Log.Add(MakeShared<FJsonValueString>(Events[(EventCursor + Index) % Events.Num()]));
	}
	Result->SetArrayField(TEXT("events"), Log);
	if (const auto* Widget = Selected.Get())
	{
		const auto Details = MakeShared<FJsonObject>();
		Details->SetStringField(TEXT("id"), WidgetIds.FindChecked(Selected));
		Details->SetStringField(TEXT("name"), Widget->GetName());
		Details->SetStringField(TEXT("class"), Widget->GetClass()->GetPathName());
		Details->SetStringField(TEXT("path"), Widget->GetPathName());
		Details->SetStringField(TEXT("visibility"), StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(Widget->GetVisibility())));
		Details->SetBoolField(TEXT("enabled"), Widget->GetIsEnabled());
		Details->SetNumberField(TEXT("opacity"), Widget->GetRenderOpacity());
		Details->SetArrayField(TEXT("position"), VectorJson(Widget->GetCachedGeometry().GetAbsolutePosition()));
		Details->SetArrayField(TEXT("size"), VectorJson(Widget->GetCachedGeometry().GetLocalSize()));
		const auto Transform = Widget->GetRenderTransform();
		Details->SetArrayField(TEXT("translation"), VectorJson(Transform.Translation));
		Details->SetArrayField(TEXT("scale"), VectorJson(Transform.Scale));
		Details->SetNumberField(TEXT("angle"), Transform.Angle);
		TArray<TSharedPtr<FJsonValue>> Properties;
		for (TFieldIterator<FProperty> It(Widget->GetClass()); It; ++It)
		{
			const FProperty* Property = *It;
			if (Property->ArrayDim != 1 || !Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible)) continue;
			if (!CastField<FBoolProperty>(Property) && !CastField<FNumericProperty>(Property) &&
				!CastField<FStrProperty>(Property) && !CastField<FNameProperty>(Property) &&
				!CastField<FTextProperty>(Property)) continue;
			FString Value;
			Property->ExportTextItem_InContainer(Value, Widget, nullptr, nullptr, PPF_None);
			const auto Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("name"), Property->GetName());
			Entry->SetStringField(TEXT("type"), Property->GetCPPType());
			Entry->SetStringField(TEXT("value"), Value.Left(512));
			Properties.Add(MakeShared<FJsonValueObject>(Entry));
		}
		Details->SetArrayField(TEXT("properties"), Properties);
		Result->SetObjectField(TEXT("selected"), Details);
	}
	else
	{
		Result->SetField(TEXT("selected"), MakeShared<FJsonValueNull>());
	}
	return Result;
}

TUniquePtr<FHttpServerResponse> FRuntimeDebugWebInspector::Handle(const FHttpServerRequest& Request, UWorld* World)
{
	check(IsInGameThread());
	if (World != InspectedWorld.Get() || Request.Verb != EHttpServerRequestVerbs::VERB_GET ||
		FPlatformTime::Seconds() - LastRefresh >= 1.0)
	{
		Refresh(World);
	}
	if (Request.Verb == EHttpServerRequestVerbs::VERB_POST)
	{
		if (Request.Body.IsEmpty() || Request.Body.Num() > 4096)
		{
			return FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest);
		}
		const FUTF8ToTCHAR Body(reinterpret_cast<const ANSICHAR*>(Request.Body.GetData()), Request.Body.Num());
		TSharedPtr<FJsonObject> Action;
		FString Name;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(Body.Length(), Body.Get())), Action) ||
			!Action->TryGetStringField(TEXT("action"), Name))
		{
			return FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest);
		}
		if (Name == TEXT("record"))
		{
			bool bEnabled;
			if (!Action->TryGetBoolField(TEXT("enabled"), bEnabled)) return FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest);
			bRecordInput = bEnabled;
		}
		else if (Name == TEXT("clear"))
		{
			Events.Reset();
			EventCursor = 0;
		}
		else if (Name == TEXT("select") || Name == TEXT("edit"))
		{
			FString Id;
			bool bHighlight;
			if (!Action->TryGetStringField(TEXT("id"), Id) || !Action->TryGetBoolField(TEXT("highlight"), bHighlight))
			{
				return FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest);
			}
			const auto* Found = Widgets.Find(Id);
			UWidget* Widget = Found ? Found->Get() : nullptr;
			if (!Widget) return FHttpServerResponse::Error(EHttpServerResponseCodes::NotFound);
			if (Name == TEXT("edit"))
			{
				FString Visibility;
				double Opacity;
				bool bEnabled, bConfirmed;
				if (!Action->TryGetBoolField(TEXT("confirmed"), bConfirmed) || !bConfirmed ||
					!Action->TryGetStringField(TEXT("visibility"), Visibility) ||
					!Action->TryGetBoolField(TEXT("enabled"), bEnabled) ||
					!Action->TryGetNumberField(TEXT("opacity"), Opacity) ||
					!FMath::IsFinite(Opacity) || Opacity < 0 || Opacity > 1)
				{
					return FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest);
				}
				const int64 Value = StaticEnum<ESlateVisibility>()->GetValueByNameString(Visibility);
				if (Value < 0 || Value > static_cast<int64>(ESlateVisibility::SelfHitTestInvisible))
				{
					return FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest);
				}
				Widget->SetVisibility(static_cast<ESlateVisibility>(Value));
				Widget->SetIsEnabled(bEnabled);
				Widget->SetRenderOpacity(static_cast<float>(Opacity));
				Record(TEXT("Widget edited  ") + Widget->GetName());
			}
			Selected = Widget;
			SetHighlight(bHighlight);
		}
		else
		{
			return FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest);
		}
	}
	FString Json;
	FJsonSerializer::Serialize(Snapshot(), TJsonWriterFactory<>::Create(&Json));
	return FHttpServerResponse::Create(Json, TEXT("application/json"));
}
