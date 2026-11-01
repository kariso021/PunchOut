#include "Player/PunchOutPlayerState.h"
#include "AbilitySystemComponent.h"

APunchOutPlayerState::APunchOutPlayerState()
{
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	SetNetUpdateFrequency(30.0f);
}

UAbilitySystemComponent* APunchOutPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}
