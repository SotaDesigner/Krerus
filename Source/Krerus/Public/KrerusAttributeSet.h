#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "KrerusAttributeSet.generated.h"

/**
 * Atributos GAS del jugador y los enemigos: vida, recursos y estadisticas.
 * Replica atributos y limita Health/Mana tras efectos; no emite los eventos de muerte del sistema tradicional.
 */
UCLASS()
class KRERUS_API UKrerusAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UKrerusAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

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

    // Base GAS para migrar las estadisticas; los calculos se conectaran en el siguiente paso.
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "GAS|Primary")
    FGameplayAttributeData Strength;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Strength)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Agility, Category = "GAS|Primary")
    FGameplayAttributeData Agility;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Agility)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intellect, Category = "GAS|Primary")
    FGameplayAttributeData Intellect;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Intellect)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "GAS|Primary")
    FGameplayAttributeData Stamina;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Stamina)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "GAS|Derived")
    FGameplayAttributeData AttackPower;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, AttackPower)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpellPower, Category = "GAS|Derived")
    FGameplayAttributeData SpellPower;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, SpellPower)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "GAS|Derived")
    FGameplayAttributeData Armor;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Armor)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category = "GAS|Derived")
    FGameplayAttributeData CriticalChance;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, CriticalChance)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Energy, Category = "GAS|Resources")
    FGameplayAttributeData Energy;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Energy)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxEnergy, Category = "GAS|Resources")
    FGameplayAttributeData MaxEnergy;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, MaxEnergy)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Rage, Category = "GAS|Resources")
    FGameplayAttributeData Rage;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, Rage)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxRage, Category = "GAS|Resources")
    FGameplayAttributeData MaxRage;

    ATTRIBUTE_ACCESSORS_BASIC(UKrerusAttributeSet, MaxRage)

protected:
    UFUNCTION()
    void OnRep_Strength(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Agility(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Intellect(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Stamina(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_AttackPower(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_SpellPower(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Armor(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_CriticalChance(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Energy(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxEnergy(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Rage(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxRage(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Health(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Mana(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
};