#include "PunchOutUIView.h"

#include "PunchOutViewModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

UTextBlock* UPunchOutUIView::AddLabel(UVerticalBox* Panel, const FString& Text, int32 Size)
{
	auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(FText::FromString(Text));
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = Size;
	Label->SetFont(Font);
	Label->SetAutoWrapText(true);
	Panel->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 8));
	return Label;
}

UButton* UPunchOutUIView::AddButton(UVerticalBox* Panel, const FString& Text)
{
	auto* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetBackgroundColor(FLinearColor(0.85f, 0.4f, 0.08f));
	auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(FText::FromString(Text));
	Label->SetMargin(FMargin(16, 10));
	Button->AddChild(Label);
	Panel->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 5));
	return Button;
}

TSharedRef<SWidget> UPunchOutUIView::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		auto* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;
		HUDText = WidgetTree->ConstructWidget<UTextBlock>();
		HUDText->SetMargin(FMargin(24));
		auto* HUDSlot = Root->AddChildToOverlay(HUDText);
		HUDSlot->SetHorizontalAlignment(HAlign_Left);
		HUDSlot->SetVerticalAlignment(VAlign_Bottom);
		MenuFrame = WidgetTree->ConstructWidget<UBorder>();
		MenuFrame->SetBrushColor(FLinearColor(0.025f, 0.045f, 0.06f, 0.96f));
		MenuFrame->SetPadding(FMargin(32));
		auto* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetWidthOverride(420);
		Width->AddChild(MenuFrame);
		auto* MenuSlot = Root->AddChildToOverlay(Width);
		MenuSlot->SetHorizontalAlignment(HAlign_Center);
		MenuSlot->SetVerticalAlignment(VAlign_Center);
		Panels = WidgetTree->ConstructWidget<UWidgetSwitcher>();
		MenuFrame->AddChild(Panels);

		auto* Main = WidgetTree->ConstructWidget<UVerticalBox>();
		Panels->AddChild(Main);
		AddLabel(Main, TEXT("PUNCH OUT"), 36);
		AddLabel(Main, TEXT("The office escape prototype"));
		HostButton = AddButton(Main, TEXT("Host game"));
		HostButton->OnClicked.AddDynamic(this, &ThisClass::Host);
		AddLabel(Main, TEXT("Host address"), 14);
		Address = WidgetTree->ConstructWidget<UEditableTextBox>();
		Address->SetText(FText::FromString(TEXT("127.0.0.1:7777")));
		Address->SetForegroundColor(FLinearColor(0.05f, 0.05f, 0.05f));
		Main->AddChildToVerticalBox(Address)->SetPadding(FMargin(0, 6));
		JoinButton = AddButton(Main, TEXT("Join game"));
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::Join);
		CancelButton = AddButton(Main, TEXT("Cancel connection"));
		CancelButton->OnClicked.AddDynamic(this, &ThisClass::Leave);
		StatusText = AddLabel(Main, TEXT(""), 14);

		auto* Lobby = WidgetTree->ConstructWidget<UVerticalBox>();
		Panels->AddChild(Lobby);
		AddLabel(Lobby, TEXT("OFFICE LOBBY"), 28);
		LobbyText = AddLabel(Lobby, TEXT(""));
		StartButton = AddButton(Lobby, TEXT("Start game"));
		StartButton->OnClicked.AddDynamic(this, &ThisClass::Start);
		AddButton(Lobby, TEXT("Leave lobby"))->OnClicked.AddDynamic(this, &ThisClass::Leave);

		auto* Pause = WidgetTree->ConstructWidget<UVerticalBox>();
		Panels->AddChild(Pause);
		AddLabel(Pause, TEXT("MENU"), 28);
		AddLabel(Pause, TEXT("The multiplayer game continues."), 14);
		AddButton(Pause, TEXT("Resume"))->OnClicked.AddDynamic(this, &ThisClass::Resume);
		AddButton(Pause, TEXT("Return to main menu"))->OnClicked.AddDynamic(this, &ThisClass::Leave);
	}
	return Super::RebuildWidget();
}

void UPunchOutUIView::SetViewModel(UPunchOutViewModel* Model)
{
	ViewModel = Model;
}

UPunchOutViewModel* UPunchOutUIView::GetDisplayViewModel() const
{
#if WITH_EDITOR
	if (DebugViewModel) return DebugViewModel;
#endif
	return ViewModel;
}

#if WITH_EDITOR
void UPunchOutUIView::SetDebugViewModel(UPunchOutViewModel* Model)
{
	if (DebugViewModel == Model) return;
	GetDisplayViewModel()->RemoveAllFieldValueChangedDelegates(this);
	DebugViewModel = Model;
	if (GetCachedWidget().IsValid())
	{
		BindDisplayViewModel();
		Refresh();
	}
}
#endif

void UPunchOutUIView::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BindDisplayViewModel();
	Refresh();
}

void UPunchOutUIView::BindDisplayViewModel()
{
	auto* Model = GetDisplayViewModel();
	Model->GetFieldNotificationDescriptor().ForEachField(Model->GetClass(), [this, Model](UE::FieldNotification::FFieldId Field)
	{
		Model->AddFieldValueChangedDelegate(Field, INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::OnFieldChanged));
		return true;
	});
}

void UPunchOutUIView::NativeDestruct()
{
	if (auto* Model = GetDisplayViewModel()) Model->RemoveAllFieldValueChangedDelegates(this);
	Super::NativeDestruct();
}

void UPunchOutUIView::OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field)
{
	Refresh();
}

void UPunchOutUIView::Refresh()
{
	const auto* Model = GetDisplayViewModel();
	if (!Panels || !Model) return;
	const bool Playing = Model->Screen == EPunchOutScreen::Playing;
	MenuFrame->SetVisibility(Playing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	HUDText->SetVisibility(Playing ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Panels->SetActiveWidgetIndex(Model->Screen == EPunchOutScreen::Lobby ? 1 : Model->Screen == EPunchOutScreen::PauseMenu ? 2 : 0);
	StatusText->SetText(Model->Status);
	HostButton->SetIsEnabled(!Model->IsConnecting);
	JoinButton->SetIsEnabled(!Model->IsConnecting);
	CancelButton->SetVisibility(Model->IsConnecting ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	LobbyText->SetText(FText::FromString(FString::Printf(TEXT("Players: %d\n%s"), Model->PlayerCount, Model->IsHost ? TEXT("You are the host. Start when everyone is here.") : TEXT("Waiting for the host to start."))));
	StartButton->SetVisibility(Model->IsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	HUDText->SetText(FText::FromString(FString::Printf(TEXT("PUNCH OUT  |  Players: %d  |  %s\nWASD: Move    Mouse: Look    Space: Jump    Esc: Menu"), Model->PlayerCount, *Model->Posture.ToString())));
}

FReply UPunchOutUIView::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape && ViewModel->Screen == EPunchOutScreen::PauseMenu)
	{
		ViewModel->ResumeGame();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

void UPunchOutUIView::Host() { ViewModel->HostGame(); }
void UPunchOutUIView::Join() { ViewModel->JoinGame(Address->GetText().ToString()); }
void UPunchOutUIView::Start() { ViewModel->StartGame(); }
void UPunchOutUIView::Resume() { ViewModel->ResumeGame(); }
void UPunchOutUIView::Leave() { ViewModel->LeaveGame(); }
