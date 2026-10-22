#include "KrerusCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "InputActionValue.h"

void AKrerusCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (!Controller)
	{
		return;
	}

	FRotator MovementRotation;

	if (bIsCameraControlActive)
	{
		// Movimiento relativo a la cámara
		MovementRotation = Controller->GetControlRotation();
	}
	else
	{
		// Guardamos la orientación al empezar a movernos
		if (!bWasMoving)
		{
			FreeMovementRotation = GetActorRotation();
		}

		MovementRotation = FreeMovementRotation;
	}

	const FRotator YawRotation(0.0f, MovementRotation.Yaw, 0.0f);

	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);

	bWasMoving = !MovementVector.IsNearlyZero();
}

void AKrerusCharacter::StartSprint()
{
	bIsDecelerating = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 2.0f;
}

void AKrerusCharacter::StopSprint()
{
	bIsDecelerating = true;
}

void AKrerusCharacter::StopMovementInput()
{
	bWasMoving = false;
}

void AKrerusCharacter::UpdateSprint(float DeltaTime)
{
	if (bIsDecelerating)
	{
		GetCharacterMovement()->MaxWalkSpeed = FMath::FInterpTo(GetCharacterMovement()->MaxWalkSpeed, WalkSpeed, DeltaTime, 5.0f);

		if (FMath::IsNearlyEqual(GetCharacterMovement()->MaxWalkSpeed, WalkSpeed, 1.0f))
		{
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
			bIsDecelerating = false;
		}
	}
}
