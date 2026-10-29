#pragma once

#include "CoreMinimal.h"
#include "KrerusResourceTypes.generated.h"

// Selector del recurso GAS que expone la interfaz de Blueprint.
UENUM(BlueprintType)
enum class EKrerusResourceType : uint8
{
    None UMETA(DisplayName = "None"),
    Mana UMETA(DisplayName = "Mana"),
    Energy UMETA(DisplayName = "Energy"),
    Rage UMETA(DisplayName = "Rage")
};

// Selección del recurso que expone la API Blueprint; valores y máximos pertenecen a GAS.
USTRUCT(BlueprintType)
struct FKrerusResource
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
    EKrerusResourceType Type = EKrerusResourceType::None;

};
