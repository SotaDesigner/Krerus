#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KrerusClassTypes.h"
#include "KrerusClassDataAsset.generated.h"

class UGameplayAbility;

UCLASS(BlueprintType)
class KRERUS_API UKrerusClassDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Class")
    EKrerusClass Class = EKrerusClass::Warrior;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Abilities")
    TArray<TSubclassOf<UGameplayAbility>> StartingAbilities;
};