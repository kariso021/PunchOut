#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PunchOutPlayerCharacter.generated.h"

UCLASS()
class PUNCHOUT_API APunchOutPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APunchOutPlayerCharacter();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
};
