// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusCombatComponent.generated.h"

class AKrerusCharacter;
class AKrerusEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAutoAttackHit,AActor*, Target,float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAutoAttackStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAutoAttackStopped);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRERUS_API UKrerusCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
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

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnAutoAttackHit OnAutoAttackHit;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnAutoAttackStarted OnAutoAttackStarted;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnAutoAttackStopped OnAutoAttackStopped;

	void CancelPendingAutoAttackImpact();

protected:
	// Called when the game starts
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

	bool bAttackImpactConsumed = true;

	UFUNCTION()
	void OnCombatTargetDeath();

	UFUNCTION()
	void OnOwnerDeath();

	FTimerHandle AutoAttackTimerHandle;

	void PerformAutoAttack();

private:
	bool IsTargetInRange(const AKrerusCharacter* PlayerCharacter,const AKrerusEnemy* Target) const;
	bool IsTargetInFront(const AKrerusCharacter* PlayerCharacter,const AKrerusEnemy* Target) const;

};
