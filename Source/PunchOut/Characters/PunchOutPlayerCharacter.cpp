#include "PunchOutPlayerCharacter.h"

#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Math/RotationMatrix.h"

APunchOutPlayerCharacter::APunchOutPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;
}

void APunchOutPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &APunchOutPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &APunchOutPlayerCharacter::MoveRight);
}

void APunchOutPlayerCharacter::MoveForward(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator MovementRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(MovementRotation).GetUnitAxis(EAxis::X);

	AddMovementInput(ForwardDirection, Value);
}

void APunchOutPlayerCharacter::MoveRight(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator MovementRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector RightDirection = FRotationMatrix(MovementRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(RightDirection, Value);
}
