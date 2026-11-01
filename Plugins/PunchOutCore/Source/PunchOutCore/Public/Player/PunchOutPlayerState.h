#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "PunchOutPlayerState.generated.h"

class UAbilitySystemComponent;

UCLASS()
class PUNCHOUTCORE_API APunchOutPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	APunchOutPlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;
};
