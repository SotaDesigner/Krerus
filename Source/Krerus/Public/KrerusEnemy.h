#pragma once

#include "CoreMinimal.h"
#include "KrerusNPC.h"
#include "AbilitySystemInterface.h"
#include "Net/UnrealNetwork.h"
#include "KrerusEnemy.generated.h"

class UAbilitySystemComponent;
class UKrerusAttributeSet;
class UKrerusCombatStateComponent;
struct FOnAttributeChangeData;

/**
 * NPC atacable con ASC propio y atributos GAS, además del AttributeComponent heredado.
 * Health GAS decide daño/muerte; muerte y retirada del cadáver son estado persistente replicado.
 */
UCLASS()
class KRERUS_API AKrerusEnemy : public AKrerusNPC
{
    GENERATED_BODY()

public:
    AKrerusEnemy();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintPure, Category = "Combat|State")
    UKrerusCombatStateComponent* GetCombatStateComponent() const;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
    TObjectPtr<UAnimMontage> HitReactionMontage = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
    TObjectPtr<UAnimMontage> DeathMontage = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
    TObjectPtr<UKrerusCombatStateComponent> CombatStateComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AggroRing")
    TObjectPtr<UMaterial> AggroRingAlive;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AggroRing")
    TObjectPtr<UMaterial> AggroRingDeath;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Death", meta = (ClampMin = "0.0"))
    float CorpseDuration = 30.0f;

    // ASC and GASAttributeSet are inherited from NPC; no second attribute system.

    // Estado de muerte autoritativo del enemigo.
    // Se replica para sincronizar el estado entre servidor y clientes.
    UPROPERTY(ReplicatedUsing = OnRep_IsDead, BlueprintReadOnly, Category = "Combat|Death")
    bool bIsDead = false;

    UFUNCTION()
    void OnRep_IsDead();

    UPROPERTY(ReplicatedUsing = OnRep_CorpseExpired)
    bool bCorpseExpired = false;
    UFUNCTION()
    void OnRep_CorpseExpired();
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastHitReaction();
    bool bDeathVisualsApplied = false;
    void ApplyDeathState();
    FTimerHandle CorpseTimerHandle;

    void OnCorpseTimeExpired();

    // Recibe los cambios del atributo Health de GAS.
    // Decide si corresponde reproducir un impacto o iniciar la muerte.
    void OnGASHealthChanged(const FOnAttributeChangeData& Data);

    // Reproduce el montaje de muerte en la instancia local del enemigo.
    // Puede ejecutarse tanto en el servidor como en los clientes.
    void PlayDeathAnimation();

    UFUNCTION()
    void OnEnemyDeath();

    float PreviousHealth = 0.0f;

    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void PlayHitReaction();

    UFUNCTION(BlueprintPure, Category = "Combat|Death")
    bool IsCorpse() const;

    UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Death")
    void BP_OnEnemyDeath();

    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
