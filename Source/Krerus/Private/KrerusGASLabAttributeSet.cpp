#include "KrerusGASLabAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "KrerusGASLabAttributeSet.h"
#include "GameplayEffectExtension.h"

UKrerusGASLabAttributeSet::UKrerusGASLabAttributeSet()
{
    InitHealth(100.f);
    InitMaxHealth(100.f);

    InitMana(100.f);
    InitMaxMana(100.f);
}

void UKrerusGASLabAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusGASLabAttributeSet, Health, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusGASLabAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusGASLabAttributeSet, Mana, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusGASLabAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
}

void UKrerusGASLabAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusGASLabAttributeSet, Health, OldHealth);
}

void UKrerusGASLabAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusGASLabAttributeSet, MaxHealth, OldMaxHealth);
}

void UKrerusGASLabAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusGASLabAttributeSet, Mana, OldMana);
}

void UKrerusGASLabAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusGASLabAttributeSet, MaxMana, OldMaxMana);
}

void UKrerusGASLabAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
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