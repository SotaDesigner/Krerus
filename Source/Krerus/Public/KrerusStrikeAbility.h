#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "KrerusTargetPositioningLibrary.h"
#include "KrerusStrikeAbility.generated.h"

class AKrerusEnemy;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EKrerusAbilityMovementType : uint8
{
    None UMETA(DisplayName = "None"),
    Teleport UMETA(DisplayName = "Teleport"),
    Charge UMETA(DisplayName = "Charge")
};
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
    TSubclassOf<UGameplayEffect> AdditionalEffectClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Combat")
    float BaseAbilityDamage = 15.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Krerus|Combat")
    float AbilityRange = 600.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Ability|Movement")
    EKrerusAbilityMovementType MovementType = EKrerusAbilityMovementType::None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Ability|Movement", meta = (EditCondition = "MovementType != EKrerusAbilityMovementType::None"))
    EKrerusTargetPosition PositionType =  EKrerusTargetPosition::Behind;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Ability|Movement", meta = (ClampMin = "0.0", EditCondition = "MovementType != EKrerusAbilityMovementType::None"))
    float MovementDistance = 150.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Ability|Movement", meta = (ClampMin = "1.0", EditCondition = "MovementType == EKrerusAbilityMovementType::Charge"))
    float MovementSpeed = 1200.0f;

    // Montaje opcional de la habilidad.
    // Si es nullptr, el daño se aplica inmediatamente.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Ability|Animation")
    TObjectPtr<UAnimMontage> AttackMontage = nullptr;

    // Evento GAS que indica el momento del impacto.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Krerus|Ability|Animation", meta = (EditCondition = "AttackMontage != nullptr"))
    FGameplayTag ImpactEventTag;

    UFUNCTION()
    void OnAbilityImpact(FGameplayEventData Payload);

    UFUNCTION()
    void OnAbilityMontageCompleted();

    UFUNCTION()
    void OnAbilityMontageInterrupted();

    UFUNCTION()
    void OnAbilityMontageCancelled();

    // Aplica el daño y los efectos configurados al objetivo.
    bool ApplyAbilityImpact(AKrerusEnemy* Target);

    // Termina la habilidad y limpia su estado.
    void FinishStrikeAbility(bool bWasCancelled);

    // Limpieza al terminar, incluso si GAS cancela externamente.
    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled
    ) override;

private:

    UPROPERTY(Transient)
    TObjectPtr<AKrerusEnemy> PendingImpactTarget = nullptr;

    bool bImpactApplied = false;
};