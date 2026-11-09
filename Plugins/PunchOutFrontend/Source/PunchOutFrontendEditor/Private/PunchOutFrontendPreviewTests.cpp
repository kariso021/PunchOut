#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Misc/AutomationTest.h"
#include "PunchOutUIView.h"
#include "PunchOutViewModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPunchOutFrontendPreviewTest, "PunchOut.Frontend.PreviewBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPunchOutFrontendPreviewTest::RunTest(const FString& Parameters)
{
	auto* View = NewObject<UPunchOutUIView>(GetTransientPackage());
	View->Initialize();
	auto* Model = NewObject<UPunchOutViewModel>(View);
	View->SetViewModel(Model);
	const TSharedRef<SWidget> SlateView = View->TakeWidget();

	Model->SetScreen(EPunchOutScreen::Playing);
	Model->SetPlayerCount(4);
	Model->SetPosture(FText::FromString(TEXT("Debug posture")));

	UTextBlock* HUD = nullptr;
	View->WidgetTree->ForEachWidget([&HUD](UWidget* Widget)
	{
		auto* Text = Cast<UTextBlock>(Widget);
		if (Text && Text->GetText().ToString().StartsWith(TEXT("PUNCH OUT  |")))
		{
			HUD = Text;
		}
	});
	if (!TestNotNull(TEXT("HUD text"), HUD)) return false;
	TestTrue(TEXT("FieldNotify updates the HUD in a preview with no player or world"),
		HUD->GetVisibility() == ESlateVisibility::HitTestInvisible &&
		HUD->GetText().ToString().Contains(TEXT("Players: 4  |  Debug posture")));

	auto* DebugModel = NewObject<UPunchOutViewModel>(View);
	DebugModel->SetScreen(EPunchOutScreen::Playing);
	View->SetDebugViewModel(DebugModel);
	DebugModel->SetPlayerCount(99);
	Model->SetPlayerCount(5);

	TestTrue(TEXT("The debug state is displayed while gameplay keeps updating"), HUD->GetText().ToString().Contains(TEXT("Players: 99")));
	View->SetDebugViewModel(nullptr);
	TestTrue(TEXT("Reset restores the latest gameplay state"), HUD->GetText().ToString().Contains(TEXT("Players: 5")));
	Model->SetPlayerCount(6);
	TestTrue(TEXT("Gameplay notifications resume after reset"), HUD->GetText().ToString().Contains(TEXT("Players: 6")));
	return true;
}

#endif
