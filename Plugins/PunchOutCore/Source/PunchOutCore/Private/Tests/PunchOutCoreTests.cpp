#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Pawn/PunchOutCharacter.h"
#include "Player/PunchOutPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPunchOutAvatarOwnershipTest, "PunchOut.Core.AvatarOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPunchOutAvatarOwnershipTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	APunchOutPlayerState* State = World->SpawnActor<APunchOutPlayerState>();
	State->SetOwner(Controller);
	Controller->PlayerState = State;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APunchOutCharacter* First = World->SpawnActor<APunchOutCharacter>(SpawnParameters);
	APunchOutCharacter* Second = World->SpawnActor<APunchOutCharacter>(SpawnParameters);
	Controller->Possess(First);
	UAbilitySystemComponent* ASC = State->GetAbilitySystemComponent();
	TestTrue(TEXT("PlayerState owns GAS"), ASC->GetOwnerActor() == State);
	TestTrue(TEXT("First Pawn becomes avatar"), ASC->GetAvatarActor() == First);
	TestTrue(TEXT("Pawn exposes shared ASC"), First->GetAbilitySystemComponent() == ASC);
	Controller->UnPossess();
	TestNull(TEXT("Unpossessed Pawn releases GAS"), First->GetAbilitySystemComponent());
	TestNull(TEXT("Avatar is cleared"), ASC->GetAvatarActor());
	Controller->Possess(Second);
	First->Destroy();
	TestTrue(TEXT("Old Pawn teardown cannot clear new avatar"), ASC->GetAvatarActor() == Second);
	TestTrue(TEXT("New Pawn uses the same ASC"), Second->GetAbilitySystemComponent() == ASC);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
