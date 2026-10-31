
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KrerusAutoAttackAbility.generated.h"

class UKrerusCombatComponent;
class AKrerusEnemy;
class AKrerusProjectile;
class UAbilityTask_WaitDelay;
class UAbilitySystemComponent;
class UGameplayEffect;

struct FOnAttributeChangeData;

/**
 * Habilidad GAS autoritativa que gestiona el ciclo completo
 * del autoataque, incluyendo su cadencia y tareas.
 */
UCLASS()
class KRERUS_API UKrerusAutoAttackAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UKrerusAutoAttackAbility();

    void StopAttack();
    void SendImpactEvent();
    void RefreshPause();

    void InvalidateImpact()
    {
        bImpactConsumed = true;
    }

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle H,
        const FGameplayAbilityActorInfo* Info,
        const FGameplayAbilityActivationInfo Activation,
        const FGameplayEventData* Event
    ) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle H,
        const FGameplayAbilityActorInfo* Info,
        const FGameplayAbilityActivationInfo Activation,
        bool bReplicate,
        bool bCancelled
    ) override;

    // Efecto de recursos aplicado al acertar un ataque.
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Krerus|Combat|Resources"
    )
    TSubclassOf<UGameplayEffect> OnHitResourceEffectClass;

    // Clase de proyectil utilizada por los ataques a distancia.
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Krerus|Combat|AutoAttack|Projectile"
    )
    TSubclassOf<AKrerusProjectile> ProjectileClass;


private:
    // Ciclo del autoataque.
    UFUNCTION()
    void TrySwing();

    UFUNCTION()
    void OnImpact(FGameplayEventData Payload);

    void Schedule(float Delay);
    void OnHealthChanged(const FOnAttributeChangeData& Data);

    // Referencias de combate.
    TWeakObjectPtr<UKrerusCombatComponent> Combat;
    TWeakObjectPtr<AKrerusEnemy> Target;
    TWeakObjectPtr<UAbilitySystemComponent> TargetASC;

    // Tarea encargada de la cadencia.
    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitDelay> DelayTask;

    // Evita procesar varias veces el mismo impacto.
    bool bImpactConsumed = true;

    // Conserva la cadencia entre reinicios y cambios de objetivo.
    double NextSwingTime = 0.0;
};
