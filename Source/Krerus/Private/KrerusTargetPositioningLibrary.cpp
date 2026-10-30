// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusTargetPositioningLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


FVector UKrerusTargetPositioningLibrary::CalculateBehinPosition(const AActor* Target, float Distance)
{
	if (!IsValid(Target))
	{
		return FVector::ZeroVector;
	}

	const FVector TargetLocation = Target->GetActorLocation();

	FVector Forward = Target->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward.Normalize();

	return TargetLocation - Forward * Distance;
}

bool UKrerusTargetPositioningLibrary::IsPositionFreeForCharacter(ACharacter* Character, FVector Position)
{
	if (!IsValid(Character))
	{
		return false;
	}

	UWorld* World = Character->GetWorld();

	if(!IsValid(World))
	{ 
		return false;
	}

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

	if (!IsValid(Capsule))
	{
		return false;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());

	const bool bBlocked = World->OverlapBlockingTestByChannel(Position, FQuat::Identity, Capsule->GetCollisionObjectType(), CapsuleShape, QueryParams);

	return !bBlocked;
}

bool UKrerusTargetPositioningLibrary::FindGroundPosition(ACharacter* Character, FVector CandidatePosition, FVector& OutGroundPosition)
{

	OutGroundPosition = FVector::ZeroVector;

	if (!IsValid(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Character"))
		return false;
	}

	UWorld* World = Character->GetWorld();
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

	if (!World || !Capsule)
	{
		UE_LOG(LogTemp, Warning, TEXT("World capsule"))
		return false;
	}

	const FVector Start = CandidatePosition + FVector(0, 0, 150);
	const FVector End = CandidatePosition - FVector(0, 0, 250);

	FHitResult Hit;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	const bool bHit = World->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params);

	DrawDebugLine(World,Start,End,FColor::Yellow,false,5.0f,0,3.0f);

	if (bHit)
	{
		DrawDebugSphere(World,Hit.ImpactPoint,20.0f,12,FColor::Green,false,5.0f,2.0f);

		UE_LOG(LogTemp, Warning,
			TEXT("KRERUS GROUND: Impacto en %s | Actor: %s | NormalZ: %.2f"),
			*Hit.ImpactPoint.ToString(),
			*GetNameSafe(Hit.GetActor()),
			Hit.ImpactNormal.Z
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("KRERUS GROUND: Sin impacto | Start=%s | End=%s"),
			*Start.ToString(),
			*End.ToString()
		);
	}

	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("bHit"))
		return false;
	}

	if (Hit.ImpactNormal.Z < 0.7f)
	{
		UE_LOG(LogTemp, Warning,TEXT("ImpactNormal"))
		return false;
	}

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	OutGroundPosition = Hit.ImpactPoint + FVector(0, 0, CapsuleHalfHeight + 2.0f);

	return true;
}

bool UKrerusTargetPositioningLibrary::FindValidPositionAroundTarget(ACharacter* MovingCharacter, AActor* Target, EKrerusTargetPosition PositionType, float Distance, FVector& OutPosition)
{
	OutPosition = FVector::ZeroVector;

	if (!IsValid(MovingCharacter) || !IsValid(Target) || Distance <= 0.0f)
	{
		return false;
	}

	FVector Forward = Target->GetActorForwardVector();
	Forward.Z = 0.0f;

	if (!Forward.Normalize())
	{
		return false;
	}

	// Elegimos la dirección según el tipo de posicionamiento.
	FVector BaseDirection;

	switch (PositionType)
	{
	case EKrerusTargetPosition::Behind:

		// Posición detrás del enemigo, según su orientación.
		BaseDirection = -Forward;
		break;

	case EKrerusTargetPosition::Around:

		// Dirección desde el enemigo hacia el personaje.
		// Así la carga termina en el lado más cercano.
		BaseDirection =	MovingCharacter->GetActorLocation()	- Target->GetActorLocation();

		BaseDirection.Z = 0.0f;

		if (!BaseDirection.Normalize())
		{
			return false;
		}

		break;

	default:
		return false;
	}

	// Intentamos primero la posición central.
	// Si falla, buscamos alternativas laterales.
	constexpr float Angles[] = { 0.0f, -20.0f,	20.0f,	-40.0f,	40.0f};

	for (const float Angle : Angles)
	{
		const FVector Direction = BaseDirection.RotateAngleAxis(Angle, FVector::UpVector);

		const FVector Candidate = Target->GetActorLocation() + Direction * Distance;

		FVector GroundPosition;

		if (!FindGroundPosition( MovingCharacter, Candidate, GroundPosition))
		{
			continue;
		}

		// Conservamos temporalmente la corrección
		// de altura que ya hemos probado en pendientes.
		for (int32 Step = 0; Step <= 10; ++Step)
		{
			const FVector AdjustedPosition = GroundPosition + FVector(0.0f, 0.0f, Step * 5.0f);

			if (IsPositionFreeForCharacter( MovingCharacter, AdjustedPosition))
			{
				OutPosition = AdjustedPosition;
				return true;
			}
		}
	}

	return false;
}
