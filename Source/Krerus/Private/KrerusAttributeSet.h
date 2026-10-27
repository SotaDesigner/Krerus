#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "KrerusAttributeSet.generated.h"

/**
 * Vida y maná de GAS, usados por PlayerState y por el ASC de los enemigos.
 * Replica atributos y limita Health/Mana tras efectos; no emite los eventos de muerte del sistema tradicional.
 */
UCLASS()
class KRERUS_API UKrerusAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UKrerusAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana)
    FGameplayAttributeData Mana;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana)
    FGameplayAttributeData MaxMana;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Health)
    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, MaxHealth)
    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Mana)
    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, MaxMana)

protected:
    UFUNCTION()
    void OnRep_Health(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Mana(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
};