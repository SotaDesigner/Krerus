#pragma once

#include "CoreMinimal.h"
#include "KrerusResourceTypes.generated.h"

// Tipos del recurso tradicional; no representan el atributo Mana de GAS.
UENUM(BlueprintType)
enum class EKrerusResourceType : uint8
{
    None UMETA(DisplayName = "None"),
    Mana UMETA(DisplayName = "Mana"),
    Energy UMETA(DisplayName = "Energy"),
    Rage UMETA(DisplayName = "Rage")
};

// Estado del recurso que inicializa y consume UKrerusAttributeComponent.
USTRUCT(BlueprintType)
struct FKrerusResource
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
    EKrerusResourceType Type = EKrerusResourceType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (ClampMin = "0.0"))
    float MaxValue = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
    float CurrentValue = 100.0f;
};