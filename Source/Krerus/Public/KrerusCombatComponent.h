#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusCombatComponent.generated.h"

class AKrerusCharacter;
class AKrerusEnemy;
class UGameplayEffect;
class UKrerusAutoAttackAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAutoAttackHit, AActor*, Target, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAutoAttackStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAutoAttackStopped);

/**
 * Interfaz de objetivo, configuracion y eventos del autoataque GAS; sin temporizador ni dano propio.
 * Cadencia e impacto en servidor; daño GAS y estado replicado al propietario.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KRERUS_API UKrerusCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKrerusCombatComponent();
    UKrerusAutoAttackAbility* GetAutoAttackAbility() const;
    bool CanAttackTarget() const;
    float GetAttackDamage() const;
    TSubclassOf<UGameplayEffect> GetDamageEffectClass() const { return AutoAttackDamageEffectClass; }
    void PublishAbilityState(bool bActive);

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
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION()
    void OnRep_AutoAttacking();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    float AttackRange = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    float AttackInterval = 2.0f;

    UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<AKrerusEnemy> CombatTarget = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_AutoAttacking, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    bool bIsAutoAttacking = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage", meta = (ClampMin = "0.0"))
    float BaseAttackDamage = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage", meta = (ClampMin = "0.0"))
    float AttackPowerCoefficient = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|GAS")
    TSubclassOf<UGameplayEffect> AutoAttackDamageEffectClass;

    UPROPERTY(Replicated)
    bool bAutoAttackPaused = false;

private:
    bool IsTargetInRange(const AKrerusCharacter* PlayerCharacter, const AKrerusEnemy* Target) const;
    bool IsTargetInFront(const AKrerusCharacter* PlayerCharacter, const AKrerusEnemy* Target) const;
};
