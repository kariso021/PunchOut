#include "Abilities/PunchOutPunchAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UPunchOutPunchAbility::UPunchOutPunchAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

bool UPunchOutPunchAbility::FindPunchHit(APawn* Pawn, float Reach, float Radius, FHitResult& Hit, bool bDraw)
{
	const FRotator Aim = Pawn->GetController() ? Pawn->GetController()->GetControlRotation() : Pawn->GetActorRotation();
	const FVector Direction = FRotator(0, Aim.Yaw, 0).Vector();
	const FVector Start = Pawn->GetActorLocation() + FVector(0, 0, 20);
	const FVector End = Start + Direction * Reach;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(Punch), false, Pawn);
	const bool bHit = Pawn->GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(Radius), Query);
	if (bDraw)
	{
		const FColor Color = bHit ? FColor::Red : FColor::Green;
		DrawDebugCapsule(Pawn->GetWorld(), (Start + End) * 0.5f, Reach * 0.5f + Radius, Radius,
			FQuat::FindBetweenNormals(FVector::UpVector, Direction), Color, false, 0.5f);
		if (bHit) DrawDebugPoint(Pawn->GetWorld(), Hit.ImpactPoint, 12, FColor::Yellow, false, 0.5f);
	}
	return bHit;
}

void UPunchOutPunchAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!Pawn || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboHit = 1;
	bFollowUpQueued = false;
	ExecutePunch(Pawn);
}

void UPunchOutPunchAbility::QueueFollowUp()
{
	if (IsActive() && ComboHit < MaxComboHits)
	{
		bFollowUpQueued = true;
	}
}

void UPunchOutPunchAbility::ExecutePunch(APawn* Pawn)
{
	FHitResult Hit;
	if (FindPunchHit(Pawn, Reach, Radius, Hit, bDrawRange))
	{
		// Prototype damage bridge; replace with a GameplayEffect when HealthSet is introduced.
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal(),
			Hit, Pawn->GetController(), Pawn, nullptr);
	}
	if (!IsActive())
	{
		return;
	}

	UAbilityTask_WaitDelay* Recovery = UAbilityTask_WaitDelay::WaitDelay(this, RecoverySeconds);
	Recovery->OnFinish.AddDynamic(this, &ThisClass::FinishRecovery);
	Recovery->ReadyForActivation();
}

void UPunchOutPunchAbility::FinishRecovery()
{
	APawn* Pawn = Cast<APawn>(CurrentActorInfo->AvatarActor.Get());
	if (bFollowUpQueued && Pawn)
	{
		bFollowUpQueued = false;
		++ComboHit;
		ExecutePunch(Pawn);
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
