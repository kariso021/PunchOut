#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Abilities/PunchOutPunchAbility.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

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
#endif
