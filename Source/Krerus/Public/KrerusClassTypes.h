#pragma once

#include "CoreMinimal.h"
#include "KrerusResourceTypes.h"
#include "KrerusClassTypes.generated.h"

UENUM(BlueprintType)
enum class EKrerusClass : uint8
{
    Warrior UMETA(DisplayName = "Guerrero"),
    Rogue   UMETA(DisplayName = "Pícaro"),
    Mage    UMETA(DisplayName = "Mago"),
    Paladin UMETA(DisplayName = "Paladín"),
    Druid   UMETA(DisplayName = "Druida")
};

inline EKrerusResourceType GetKrerusClassResource(EKrerusClass Class)
{
    switch (Class)
    {
    case EKrerusClass::Warrior:
        return EKrerusResourceType::Rage;
    case EKrerusClass::Rogue:
        return EKrerusResourceType::Energy;
    case EKrerusClass::Mage:
    case EKrerusClass::Paladin:
    case EKrerusClass::Druid:
        return EKrerusResourceType::Mana;
    default:
        return EKrerusResourceType::None;
    }
}