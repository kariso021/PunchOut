#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Abilities/PunchOutPunchAbility.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPunchOutPunchRangeTest, "PunchOut.Combat.PunchRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPunchOutPunchRangeTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APawn* Pawn = World->SpawnActor<APawn>();
	auto Box = [World](FVector Position)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Shape = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Shape);
		Shape->SetBoxExtent(FVector(5));
		Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Shape->SetCollisionResponseToAllChannels(ECR_Block);
		Shape->RegisterComponent();
		Actor->SetActorLocation(Position);
		return Actor;
	};
	AActor* Target = Box(FVector(100, 0, 20));
	FHitResult Hit;
	TestTrue(TEXT("Front target is hit"), UPunchOutPunchAbility::FindPunchHit(Pawn, 140, 20, Hit, false));
	TestTrue(TEXT("Hit is the front target"), Hit.GetActor() == Target);
	Target->SetActorLocation(FVector(100, 60, 20));
	TestFalse(TEXT("Side target misses"), UPunchOutPunchAbility::FindPunchHit(Pawn, 140, 20, Hit, false));
	Target->SetActorLocation(FVector(-100, 0, 20));
	TestFalse(TEXT("Behind target misses"), UPunchOutPunchAbility::FindPunchHit(Pawn, 140, 20, Hit, false));
	Target->SetActorLocation(FVector(190, 0, 20));
	TestFalse(TEXT("Beyond reach misses"), UPunchOutPunchAbility::FindPunchHit(Pawn, 140, 20, Hit, false));
	Target->SetActorLocation(FVector(100, 0, 20));
	AActor* Wall = Box(FVector(50, 0, 20));
	UPunchOutPunchAbility::FindPunchHit(Pawn, 140, 20, Hit, false);
	TestTrue(TEXT("Wall blocks target"), Hit.GetActor() == Wall);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPunchOutPunchComboTest, "PunchOut.Combat.PunchCombo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPunchOutPunchComboTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APawn* Pawn = World->SpawnActor<APawn>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Pawn);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Pawn, Pawn);
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(UPunchOutPunchAbility::StaticClass()));
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	auto* Ability = CastChecked<UPunchOutPunchAbility>(Spec->GetPrimaryInstance());

	// Drive the isolated world's timers across frames without waiting in real time.
	const TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
	auto TickTimers = [World](float DeltaTime)
	{
		++GFrameCounter;
		World->GetTimerManager().Tick(DeltaTime);
	};
	TickTimers(0.0f);

	TestTrue(TEXT("First click starts a punch"), ASC->TryActivateAbility(Handle));
	Ability->QueueFollowUp();
	Ability->QueueFollowUp();
	TickTimers(0.5f);
	TestTrue(TEXT("One buffered click continues after recovery"), Spec->IsActive());
	TickTimers(0.5f);
	TestFalse(TEXT("Extra clicks during the first recovery do not carry into the second"), Spec->IsActive());

	TestTrue(TEXT("A new chain can start"), ASC->TryActivateAbility(Handle));
	Ability->QueueFollowUp();
	TickTimers(0.5f);
	Ability->QueueFollowUp();
	TickTimers(0.5f);
	TestTrue(TEXT("A second buffered click reaches the third punch"), Spec->IsActive());
	Ability->QueueFollowUp();
	TickTimers(0.5f);
	TestFalse(TEXT("The third punch cannot queue a fourth"), Spec->IsActive());

	TestTrue(TEXT("A punch can start before cancellation"), ASC->TryActivateAbility(Handle));
	Ability->QueueFollowUp();
	ASC->CancelAbilityHandle(Handle);
	TestTrue(TEXT("A fresh punch can start after cancellation"), ASC->TryActivateAbility(Handle));
	TickTimers(0.5f);
	TestFalse(TEXT("A canceled chain cannot leak a buffered hit into a fresh punch"), Spec->IsActive());

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
