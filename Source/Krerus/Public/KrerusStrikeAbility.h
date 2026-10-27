#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KrerusStrikeAbility.generated.h"

class UGameplayEffect;

/**
 * Habilidad GAS ejecutada en servidor que toma el enemigo seleccionado en PlayerController.
 * Aplica el Gameplay Effect configurado al ASC enemigo, sin pasar por el daño del autoataque.
 */
UCLASS()
class KRERUS_API UKrerusStrikeAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UKrerusStrikeAbility();

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

    UPROPERTY(EditDefaultsOnly, Category = "Krerus|Combat")
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditDefaultsOnly, Category = "Krerus|Combat")
    float AbilityRange = 600.0f;
};