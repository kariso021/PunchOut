#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MVVMViewModelBase.h"
#include "PunchOutUIView.generated.h"

class UPunchOutViewModel;
class UBorder;
class UButton;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

UCLASS()
class PUNCHOUTFRONTEND_API UPunchOutUIView : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetViewModel(UPunchOutViewModel* Model);
	UPunchOutViewModel* GetDisplayViewModel() const;
#if WITH_EDITOR
	void SetDebugViewModel(UPunchOutViewModel* Model);
#endif
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
	UTextBlock* AddLabel(UVerticalBox* Panel, const FString& Text, int32 Size = 18);
	UButton* AddButton(UVerticalBox* Panel, const FString& Text);
	void Refresh();
	void BindDisplayViewModel();
	void OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field);
	UFUNCTION() void Host();
	UFUNCTION() void Join();
	UFUNCTION() void Start();
	UFUNCTION() void Resume();
	UFUNCTION() void Leave();
	UPROPERTY(Transient) TObjectPtr<UPunchOutViewModel> ViewModel;
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient) TObjectPtr<UPunchOutViewModel> DebugViewModel;
#endif
	UPROPERTY(Transient) TObjectPtr<UBorder> MenuFrame;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Panels;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> Address;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LobbyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HUDText;
	UPROPERTY(Transient) TObjectPtr<UButton> StartButton;
	UPROPERTY(Transient) TObjectPtr<UButton> HostButton;
	UPROPERTY(Transient) TObjectPtr<UButton> JoinButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelButton;
};
