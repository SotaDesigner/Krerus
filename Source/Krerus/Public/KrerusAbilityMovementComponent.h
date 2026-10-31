// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusTargetPositioningLibrary.h"
#include "KrerusAbilityMovementComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KRERUS_API UKrerusAbilityMovementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKrerusAbilityMovementComponent();

    UFUNCTION(BlueprintCallable, Category = "Krerus|Ability|Movement")
    bool TeleportToTargetPosition(
        AActor* Target,
        EKrerusTargetPosition PositionType,
        float Distance = 150.0f
    );

    UFUNCTION(BlueprintCallable, Category = "Krerus|Ability|Movement")
    bool TeleportToPosition(const FVector& Destination, AActor* LookAtTarget);
};
