#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PunchOutViewModel.generated.h"

UENUM(BlueprintType)
enum class EPunchOutScreen : uint8
{
	MainMenu,
	Lobby,
	Playing,
	PauseMenu
};

UCLASS()
class PUNCHOUTFRONTEND_API UPunchOutViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UI")
	EPunchOutScreen Screen = EPunchOutScreen::MainMenu;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UI")
	FText Status;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UI")
	int32 PlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UI")
	bool IsHost = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UI")
	FText Posture;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UI")
	bool IsConnecting = false;

	void SetScreen(EPunchOutScreen Value) { UE_MVVM_SET_PROPERTY_VALUE(Screen, Value); }
	void SetStatus(const FText& Value) { UE_MVVM_SET_PROPERTY_VALUE(Status, Value); }
	void SetPlayerCount(int32 Value) { UE_MVVM_SET_PROPERTY_VALUE(PlayerCount, Value); }
	void SetIsHost(bool Value) { UE_MVVM_SET_PROPERTY_VALUE(IsHost, Value); }
	void SetPosture(const FText& Value) { UE_MVVM_SET_PROPERTY_VALUE(Posture, Value); }
	void SetIsConnecting(bool Value) { UE_MVVM_SET_PROPERTY_VALUE(IsConnecting, Value); }

	FSimpleDelegate HostCommand;
	TDelegate<void(const FString&)> JoinCommand;
	FSimpleDelegate StartCommand;
	FSimpleDelegate ResumeCommand;
	FSimpleDelegate LeaveCommand;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HostGame() { HostCommand.ExecuteIfBound(); }
	UFUNCTION(BlueprintCallable, Category = "UI")
	void JoinGame(const FString& Address) { JoinCommand.ExecuteIfBound(Address); }
	UFUNCTION(BlueprintCallable, Category = "UI")
	void StartGame() { StartCommand.ExecuteIfBound(); }
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResumeGame() { ResumeCommand.ExecuteIfBound(); }
	UFUNCTION(BlueprintCallable, Category = "UI")
	void LeaveGame() { LeaveCommand.ExecuteIfBound(); }
};
