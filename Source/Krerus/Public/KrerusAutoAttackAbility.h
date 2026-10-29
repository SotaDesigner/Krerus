#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KrerusAutoAttackAbility.generated.h"
class UKrerusCombatComponent;
class AKrerusEnemy;
class UAbilityTask_WaitDelay;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/** One authoritative GAS ability owns the entire autoattack lifetime and its tasks. */
UCLASS()
class KRERUS_API UKrerusAutoAttackAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UKrerusAutoAttackAbility();
    void StopAttack();
    void SendImpactEvent();
    void InvalidateImpact() { bImpactConsumed=true; }
    void RefreshPause();
protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle H,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle H,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo Activation,bool bReplicate,bool bCancelled) override;
private:
    UFUNCTION() void TrySwing();
    UFUNCTION() void OnImpact(FGameplayEventData Payload);
    void Schedule(float Delay);
    void OnHealthChanged(const FOnAttributeChangeData& Data);
    TWeakObjectPtr<UKrerusCombatComponent> Combat;
    TWeakObjectPtr<AKrerusEnemy> Target;
    TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
    UPROPERTY() TObjectPtr<UAbilityTask_WaitDelay> DelayTask;
    bool bImpactConsumed=true;
    // Retained by the instanced GAS ability across stop/restart and target changes.
    double NextSwingTime=0.0;
};
