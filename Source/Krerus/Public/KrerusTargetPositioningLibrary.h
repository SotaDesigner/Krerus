// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KrerusTargetPositioningLibrary.generated.h"

UENUM(BlueprintType)
enum class EKrerusTargetPosition : uint8
{
	Behind UMETA(DisplayName = "Behind"),
	Around UMETA(DisplayName = "Around")
};

UCLASS()
class KRERUS_API UKrerusTargetPositioningLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Krerus|Combat|Positioning")
	static FVector CalculateBehinPosition(const AActor* Target, float Distance = 150.0f);

	UFUNCTION(BlueprintCallable, Category = "Krerus|Combat|Positioning")
	static bool IsPositionFreeForCharacter(ACharacter* Character,FVector Position);

	UFUNCTION(BlueprintCallable, Category = "Krerus|Combat|Positioning")
	static bool FindGroundPosition(ACharacter* Character, FVector CandidatePosition, FVector& OutGroundPosition);

	UFUNCTION(BlueprintCallable,Category = "Krerus|Targeting")
	static bool FindValidPositionAroundTarget(ACharacter* MovingCharacter, AActor* Target, EKrerusTargetPosition PositionType, float Distance, FVector& OutPosition);
};
