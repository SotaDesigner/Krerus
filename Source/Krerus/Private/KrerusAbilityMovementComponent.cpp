// Fill out your copyright notice in the Description page of Project Settings.

#include "KrerusAbilityMovementComponent.h"
#include "KrerusTargetPositioningLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UKrerusAbilityMovementComponent::UKrerusAbilityMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

bool UKrerusAbilityMovementComponent::TeleportToTargetPosition(AActor* Target, EKrerusTargetPosition PositionType, float Distance)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character) || !IsValid(Target))
	{
		return false;
	}

	if (!Character->HasAuthority())
	{
		return false;
	}

	FVector Destination;

	const bool bFoundPosition = UKrerusTargetPositioningLibrary::FindValidPositionAroundTarget(Character, Target, PositionType, Distance, Destination);

	if (!bFoundPosition)
	{
		return false;
	}

	FVector LookDirection = Target->GetActorLocation() - Destination;

	LookDirection.Z = 0.0f;

	if (!LookDirection.Normalize())
	{
		return false;
	}

	const FRotator LookRotation = LookDirection.Rotation();

	const bool bTeleported = Character->TeleportTo(Destination, LookRotation, false, true);

	return bTeleported;
}

bool UKrerusAbilityMovementComponent::TeleportToPosition(const FVector& Destination, AActor* LookAtTarget)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!IsValid(Character) ||
		!IsValid(LookAtTarget) ||
		!Character->HasAuthority())
	{
		return false;
	}

	FVector Direction =
		LookAtTarget->GetActorLocation() - Destination;

	Direction.Z = 0.0f;

	if (!Direction.Normalize())
	{
		return false;
	}

	const FRotator Rotation = Direction.Rotation();

	return Character->TeleportTo(
		Destination,
		Rotation,
		false,
		true
	);
}
