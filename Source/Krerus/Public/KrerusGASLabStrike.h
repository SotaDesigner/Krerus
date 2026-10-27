#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KrerusGASLabStrike.generated.h"

class UGameplayEffect;

/**
 * Habilidad de laboratorio que busca un GASLabTarget y aplica el efecto configurado.
 * Sirve para comprobar el circuito GAS separado de Strike y del autoataque del juego.
 */
UCLASS()
class KRERUS_API UKrerusGASLabStrike : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UKrerusGASLabStrike();

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

    UPROPERTY(EditDefaultsOnly, Category = "GASLab")
    TSubclassOf<UGameplayEffect> DamageEffectClass;
};
