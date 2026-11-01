#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "PunchOutCharacter.generated.h"

class UAbilitySystemComponent;

UCLASS()
class PUNCHOUTCORE_API APunchOutCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void PawnClientRestart() override;
protected:
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void InitializeAbilitySystem();
	void ReleaseAbilitySystem();
	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystem;
};
