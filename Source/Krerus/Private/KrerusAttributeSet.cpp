#include "KrerusAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UKrerusAttributeSet::UKrerusAttributeSet()
{
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitMana(100.f);
    InitMaxMana(100.f);
}

// Mantiene las notificaciones GAS de los cuatro atributos replicados, incluso al recibir el mismo valor.
void UKrerusAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Health, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Mana, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
}

void UKrerusAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Health, OldValue);
}

void UKrerusAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, MaxHealth, OldValue);
}

void UKrerusAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Mana, OldValue);
}

void UKrerusAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, MaxMana, OldValue);
}

// Limita vida y maná tras ejecutar efectos; no conecta esta vida con OnDeath del AttributeComponent.
void UKrerusAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    }
    else if (Data.EvaluatedData.Attribute == GetManaAttribute())
    {
        SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
    }
}