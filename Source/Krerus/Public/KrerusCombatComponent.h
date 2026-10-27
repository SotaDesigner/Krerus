#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusCombatComponent.generated.h"

class AKrerusCharacter;
class AKrerusEnemy;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAutoAttackHit, AActor*, Target, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAutoAttackStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAutoAttackStopped);

/**
 * Orquesta el autoataque del Character con temporizador, montaje e impacto.
 * Consulta y modifica UKrerusAttributeComponent; este circuito no aplica Gameplay Effects de GAS.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KRERUS_API UKrerusCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKrerusCombatComponent();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void SetCombatTarget(AKrerusEnemy* NewTarget);

    UFUNCTION(BlueprintPure, Category = "Combat")
    AKrerusEnemy* GetCombatTarget() const;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool StartAutoAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopAutoAttack();

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsAutoAttacking() const;

    UFUNCTION(BlueprintPure, Category = "Combat")
    float GetAttackInterval() const;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ApplyAutoAttackImpact();

    UFUNCTION(BlueprintPure, Category = "Combat|AutoAttack")
    bool IsAutoAttackPaused() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|AutoAttack")
    void SetAutoAttackPaused(bool bPaused);

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnAutoAttackHit OnAutoAttackHit;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnAutoAttackStarted OnAutoAttackStarted;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnAutoAttackStopped OnAutoAttackStopped;

    void CancelPendingAutoAttackImpact();

protected:
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    float AttackRange = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    float AttackInterval = 2.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<AKrerusEnemy> CombatTarget = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    bool bIsAutoAttacking = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage", meta = (ClampMin = "0.0"))
    float BaseAttackDamage = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage", meta = (ClampMin = "0.0"))
    float AttackPowerCoefficient = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|GAS")
    TSubclassOf<UGameplayEffect> AutoAttackDamageEffectClass;

    bool bAttackImpactConsumed = true;
    bool bAutoAttackPaused = false;

    UFUNCTION()
    void OnCombatTargetDeath();

    UFUNCTION()
    void OnOwnerDeath();

    FTimerHandle AutoAttackTimerHandle;

    void PerformAutoAttack();

private:
    bool IsTargetInRange(const AKrerusCharacter* PlayerCharacter, const AKrerusEnemy* Target) const;
    bool IsTargetInFront(const AKrerusCharacter* PlayerCharacter, const AKrerusEnemy* Target) const;
};
