#include "Pawn/PunchOutCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"

UAbilitySystemComponent* APunchOutCharacter::GetAbilitySystemComponent() const
{
	return CachedAbilitySystem.Get();
}

void APunchOutCharacter::InitializeAbilitySystem()
{
	const auto* Provider = Cast<IAbilitySystemInterface>(GetPlayerState());
	UAbilitySystemComponent* AbilitySystem = Provider ? Provider->GetAbilitySystemComponent() : nullptr;
	if (CachedAbilitySystem.Get() != AbilitySystem)
	{
		ReleaseAbilitySystem();
		CachedAbilitySystem = AbilitySystem;
	}
	if (AbilitySystem) AbilitySystem->InitAbilityActorInfo(GetPlayerState(), this);
}

void APunchOutCharacter::ReleaseAbilitySystem()
{
	if (UAbilitySystemComponent* AbilitySystem = CachedAbilitySystem.Get())
	{
		// A late teardown from the old Pawn must not detach its replacement.
		if (AbilitySystem->GetAvatarActor() == this) AbilitySystem->ClearActorInfo();
	}
	CachedAbilitySystem.Reset();
}

void APunchOutCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void APunchOutCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

void APunchOutCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeAbilitySystem();
}

void APunchOutCharacter::UnPossessed()
{
	ReleaseAbilitySystem();
	Super::UnPossessed();
}

void APunchOutCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseAbilitySystem();
	Super::EndPlay(EndPlayReason);
}
