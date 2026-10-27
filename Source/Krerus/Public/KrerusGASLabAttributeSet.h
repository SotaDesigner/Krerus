#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "KrerusGASLabAttributeSet.generated.h"

/**
 * Atributos replicados de vida y maná exclusivos del laboratorio GAS.
 * Limita los valores tras Gameplay Effects y notifica sus cambios mediante las macros de GAS.
 */
UCLASS()
class KRERUS_API UKrerusGASLabAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "GASLab|Attributes")
    FGameplayAttributeData Health;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "GASLab|Attributes")
    FGameplayAttributeData MaxHealth;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "GASLab|Attributes")
    FGameplayAttributeData Mana;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "GASLab|Attributes")
    FGameplayAttributeData MaxMana;

    UKrerusGASLabAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    UFUNCTION()
    void OnRep_Health(const FGameplayAttributeData& OldHealth);

    UFUNCTION()
    void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

    UFUNCTION()
    void OnRep_Mana(const FGameplayAttributeData& OldMana);

    UFUNCTION()
    void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana);

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusGASLabAttributeSet, Health)
    ATTRIBUTE_ACCESSORS_BASIC(UKrerusGASLabAttributeSet, MaxHealth)
    ATTRIBUTE_ACCESSORS_BASIC(UKrerusGASLabAttributeSet, Mana)
    ATTRIBUTE_ACCESSORS_BASIC(UKrerusGASLabAttributeSet, MaxMana)
};