#pragma once

#include "CoreMinimal.h"
#include "KrerusAttributeInitialization.generated.h"

USTRUCT(BlueprintType)
struct FKrerusAttributeInitialization
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Strength = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Agility = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Intellect = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseArmor = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Stamina = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxEnergy = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxRage = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StrengthToAttackPower = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AgilityToAttackPower = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float IntellectToSpellPower = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AgilityToCriticalChance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StaminaToArmor = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StaminaToHealth = 10.f;
};

