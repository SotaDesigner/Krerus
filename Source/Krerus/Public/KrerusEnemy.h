#pragma once

#include "CoreMinimal.h"
#include "KrerusNPC.h"
#include "AbilitySystemInterface.h"
#include "KrerusEnemy.generated.h"

class UAbilitySystemComponent;
class UKrerusAttributeSet;

/**
 * NPC atacable con ASC propio y atributos GAS, además del AttributeComponent heredado.
 * Las reacciones, muerte y estado de cadáver siguen dependiendo de la vida tradicional.
 */
UCLASS()
class KRERUS_API AKrerusEnemy : public AKrerusNPC, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AKrerusEnemy();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UKrerusAttributeSet> GASAttributeSet;

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

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
