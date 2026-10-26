// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KrerusNPC.h"
#include "KrerusEnemy.generated.h"

UCLASS()
class KRERUS_API AKrerusEnemy : public AKrerusNPC
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AKrerusEnemy();

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> HitReactionMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AggroRing")
	TObjectPtr<UMaterial> AggroRingAlive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AggroRing")
	TObjectPtr<UMaterial> AggroRingDeath;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Death", meta = (ClampMin = "0.0"))
	float CorpseDuration = 30.0f;

	FTimerHandle CorpseTimerHandle;

	void OnCorpseTimeExpired();

	UFUNCTION()
	void OnEnemyHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void OnEnemyDeath();

	float PreviousHealth = 0.0f;

	virtual void BeginPlay() override;

public:	
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
	void PlayHitReaction();

	UFUNCTION(BlueprintPure, Category = "Combat|Death")
	bool IsCorpse() const;

	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
