#include "KrerusAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UKrerusAttributeSet::UKrerusAttributeSet()
{
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitMana(100.f);
    InitMaxMana(100.f);
    InitStrength(10.0f);
    InitAgility(10.0f);
    InitIntellect(10.0f);
    InitStamina(10.0f);
    InitAttackPower(10.0f);
    InitSpellPower(10.0f);
    InitArmor(0.0f);
    InitCriticalChance(0.0f);
    InitEnergy(100.0f);
    InitMaxEnergy(100.0f);
    InitRage(0.0f);
    InitMaxRage(100.0f);
}

// Mantiene las notificaciones GAS de los atributos replicados, incluso al recibir el mismo valor.
void UKrerusAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Health, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Mana, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Strength, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Agility, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Intellect, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, SpellPower, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Armor, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, CriticalChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Energy, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, Rage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UKrerusAttributeSet, MaxRage, COND_None, REPNOTIFY_Always);
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

// Limita recursos tras ejecutar efectos; el ASC observa Health para detectar muerte.
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
    else if (Data.EvaluatedData.Attribute == GetEnergyAttribute()) SetEnergy(FMath::Clamp(GetEnergy(),0.f,GetMaxEnergy()));
    else if (Data.EvaluatedData.Attribute == GetRageAttribute()) SetRage(FMath::Clamp(GetRage(),0.f,GetMaxRage()));
}
void UKrerusAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Strength, OldValue);
}

void UKrerusAttributeSet::OnRep_Agility(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Agility, OldValue);
}

void UKrerusAttributeSet::OnRep_Intellect(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Intellect, OldValue);
}

void UKrerusAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Stamina, OldValue);
}

void UKrerusAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, AttackPower, OldValue);
}

void UKrerusAttributeSet::OnRep_SpellPower(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, SpellPower, OldValue);
}

void UKrerusAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Armor, OldValue);
}

void UKrerusAttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, CriticalChance, OldValue);
}

void UKrerusAttributeSet::OnRep_Energy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Energy, OldValue);
}

void UKrerusAttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, MaxEnergy, OldValue);
}

void UKrerusAttributeSet::OnRep_Rage(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, Rage, OldValue);
}

void UKrerusAttributeSet::OnRep_MaxRage(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UKrerusAttributeSet, MaxRage, OldValue);
}


void UKrerusAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& A,float& V) const
{
    Super::PreAttributeBaseChange(A,V);
    if (!FMath::IsFinite(V)) V=0.f;
    if (A==GetHealthAttribute())V=FMath::Clamp(V,0.f,FMath::Max(1.f,GetMaxHealth()));
    else if(A==GetManaAttribute())V=FMath::Clamp(V,0.f,FMath::Max(0.f,GetMaxMana()));
    else if(A==GetEnergyAttribute())V=FMath::Clamp(V,0.f,FMath::Max(0.f,GetMaxEnergy()));
    else if(A==GetRageAttribute())V=FMath::Clamp(V,0.f,FMath::Max(0.f,GetMaxRage()));
    else if(A==GetMaxHealthAttribute())V=FMath::Max(1.f,V);
    else if(A==GetCriticalChanceAttribute())V=FMath::Clamp(V,0.f,100.f);
    else V=FMath::Max(0.f,V);
}
void UKrerusAttributeSet::PreAttributeChange(const FGameplayAttribute& A,float& V)
{
    Super::PreAttributeChange(A,V);
    PreAttributeBaseChange(A,V);
}
void UKrerusAttributeSet::PostAttributeChange(const FGameplayAttribute& A,float Old,float New)
{
    Super::PostAttributeChange(A,Old,New);
    auto* ASC=GetOwningAbilitySystemComponent();if(!ASC || !ASC->IsOwnerActorAuthoritative())return;
    if(A==GetMaxHealthAttribute() && GetHealth()>New)ASC->SetNumericAttributeBase(GetHealthAttribute(),FMath::Max(0.f,New));
    if(A==GetMaxManaAttribute() && GetMana()>New)ASC->SetNumericAttributeBase(GetManaAttribute(),FMath::Max(0.f,New));
    if(A==GetMaxEnergyAttribute() && GetEnergy()>New)ASC->SetNumericAttributeBase(GetEnergyAttribute(),FMath::Max(0.f,New));
    if(A==GetMaxRageAttribute() && GetRage()>New)ASC->SetNumericAttributeBase(GetRageAttribute(),FMath::Max(0.f,New));
}
