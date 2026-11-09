#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "PunchOutPunchAbility.generated.h"

class APawn;

UCLASS()
class PUNCHOUTCOMBAT_API UPunchOutPunchAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UPunchOutPunchAbility();
	void QueueFollowUp();
	// Returns the first blocking hit. Direction is horizontal controller aim.
	static bool FindPunchHit(APawn* Pawn, float Reach, float Radius, FHitResult& Hit, bool bDraw);
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	UPROPERTY(EditDefaultsOnly, Category = "Punch", meta = (ClampMin = "0"))
	float Reach = 140.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Punch", meta = (ClampMin = "1"))
	float Radius = 20.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Punch", meta = (ClampMin = "0"))
	float Damage = 10.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Punch", meta = (ClampMin = "0.01"))
	float RecoverySeconds = 0.45f;
	UPROPERTY(EditDefaultsOnly, Category = "Punch")
	bool bDrawRange = true;
private:
	void ExecutePunch(APawn* Pawn);
	UFUNCTION()
	void FinishRecovery();

	static constexpr int32 MaxComboHits = 3;
	int32 ComboHit = 0;
	bool bFollowUpQueued = false;
};
