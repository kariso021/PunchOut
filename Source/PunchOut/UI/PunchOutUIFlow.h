#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "PunchOutUIFlow.generated.h"

class UPunchOutViewModel;
class UPunchOutUIView;

UCLASS()
class PUNCHOUT_API UPunchOutGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init() override;
	virtual void Shutdown() override;
	bool ShowMainMenu = true;
private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Message);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Message);
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
};

UCLASS()
class PUNCHOUT_API APunchOutGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	bool MatchStarted = false;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS()
class PUNCHOUT_API APunchOutGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	APunchOutGameMode();
};

UCLASS()
class PUNCHOUT_API APunchOutUIPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	APunchOutUIPlayerController();
	void ReportConnectionFailure(const FString& Message);
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
private:
	void RefreshViewModel();
	void ApplyInputState();
	void ToggleMenu();
	void HostGame();
	void JoinGame(const FString& Address);
	void StartGame();
	void ResumeGame();
	void LeaveGame();
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPunchOutViewModel> ViewModel;
	UPROPERTY(Transient)
	TObjectPtr<UPunchOutUIView> View;
	float RefreshElapsed = 0;
};
