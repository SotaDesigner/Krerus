#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "KrerusPlayerState.generated.h"

class UAbilitySystemComponent;
class UKrerusAttributeSet;
class UGameplayAbility;

/**
 * Propietario del ASC y de UKrerusAttributeSet del jugador.
 * AKrerusCharacter lo utiliza como Owner de GAS y se registra como Avatar al poseerse o recibir PlayerState.
 */
UCLASS()
class KRERUS_API AKrerusPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKrerusPlayerState();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UKrerusAttributeSet* GetKrerusAttributeSet() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UKrerusAttributeSet> AttributeSet;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
    TSubclassOf<UGameplayAbility> StartupAbilityClass;

    virtual void BeginPlay() override;
};