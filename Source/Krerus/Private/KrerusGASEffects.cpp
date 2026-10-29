#include "KrerusGASEffects.h"
#include "KrerusAttributeSet.h"
#include "GameplayEffectAggregator.h"

namespace
{
    void Caller(UGameplayEffect* Effect, const FGameplayAttribute& Attribute, const TCHAR* Name, EGameplayModOp::Type Op = EGameplayModOp::Additive)
    {
        FGameplayModifierInfo M;
        M.Attribute = Attribute; M.ModifierOp = Op;
        FSetByCallerFloat Caller; Caller.DataName = FName(Name);
        M.ModifierMagnitude = FGameplayEffectModifierMagnitude(Caller);
        Effect->Modifiers.Add(M);
    }
    void Calculation(UGameplayEffect* Effect, const FGameplayAttribute& Attribute, TSubclassOf<UGameplayModMagnitudeCalculation> Class)
    {
        FGameplayModifierInfo M; M.Attribute = Attribute; M.ModifierOp = EGameplayModOp::Additive;
        FCustomCalculationBasedFloat C; C.CalculationClassMagnitude = Class;
        M.ModifierMagnitude = FGameplayEffectModifierMagnitude(C); Effect->Modifiers.Add(M);
    }
}
UKrerusInitialStatsEffect::UKrerusInitialStatsEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    Caller(this, UKrerusAttributeSet::GetStrengthAttribute(), TEXT("Strength"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetAgilityAttribute(), TEXT("Agility"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetIntellectAttribute(), TEXT("Intellect"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetStaminaAttribute(), TEXT("Stamina"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetAttackPowerAttribute(), TEXT("AttackPower"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetSpellPowerAttribute(), TEXT("SpellPower"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetArmorAttribute(), TEXT("Armor"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetCriticalChanceAttribute(), TEXT("CriticalChance"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetMaxEnergyAttribute(), TEXT("MaxEnergy"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetEnergyAttribute(), TEXT("Energy"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetMaxRageAttribute(), TEXT("MaxRage"), EGameplayModOp::Override);
    Caller(this, UKrerusAttributeSet::GetRageAttribute(), TEXT("Rage"), EGameplayModOp::Override);
}
UKrerusAttributeBonusEffect::UKrerusAttributeBonusEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    Caller(this, UKrerusAttributeSet::GetStrengthAttribute(), TEXT("Strength"));
    Caller(this, UKrerusAttributeSet::GetAgilityAttribute(), TEXT("Agility"));
    Caller(this, UKrerusAttributeSet::GetIntellectAttribute(), TEXT("Intellect"));
    Caller(this, UKrerusAttributeSet::GetStaminaAttribute(), TEXT("Stamina"));
    Caller(this, UKrerusAttributeSet::GetAttackPowerAttribute(), TEXT("AttackPower"));
    Caller(this, UKrerusAttributeSet::GetSpellPowerAttribute(), TEXT("SpellPower"));
    Caller(this, UKrerusAttributeSet::GetArmorAttribute(), TEXT("Armor"));
    Caller(this, UKrerusAttributeSet::GetCriticalChanceAttribute(), TEXT("CriticalChance"));
}
UKrerusResourceDeltaEffect::UKrerusResourceDeltaEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    Caller(this, UKrerusAttributeSet::GetHealthAttribute(), TEXT("Health"));
    Caller(this, UKrerusAttributeSet::GetManaAttribute(), TEXT("Mana"));
    Caller(this, UKrerusAttributeSet::GetEnergyAttribute(), TEXT("Energy"));
    Caller(this, UKrerusAttributeSet::GetRageAttribute(), TEXT("Rage"));
}
UKrerusDerivedStatsEffect::UKrerusDerivedStatsEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    Calculation(this, UKrerusAttributeSet::GetAttackPowerAttribute(), UKrerusAttackPowerMagnitude::StaticClass());
    Calculation(this, UKrerusAttributeSet::GetSpellPowerAttribute(), UKrerusSpellPowerMagnitude::StaticClass());
    Calculation(this, UKrerusAttributeSet::GetArmorAttribute(), UKrerusArmorMagnitude::StaticClass());
    Calculation(this, UKrerusAttributeSet::GetCriticalChanceAttribute(), UKrerusCriticalMagnitude::StaticClass());
    Calculation(this, UKrerusAttributeSet::GetMaxHealthAttribute(), UKrerusHealthMagnitude::StaticClass());
}
UKrerusDerivedMagnitude::UKrerusDerivedMagnitude()
{
    StrengthCapture = FGameplayEffectAttributeCaptureDefinition(UKrerusAttributeSet::GetStrengthAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    AgilityCapture = FGameplayEffectAttributeCaptureDefinition(UKrerusAttributeSet::GetAgilityAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    IntellectCapture = FGameplayEffectAttributeCaptureDefinition(UKrerusAttributeSet::GetIntellectAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    StaminaCapture = FGameplayEffectAttributeCaptureDefinition(UKrerusAttributeSet::GetStaminaAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    RelevantAttributesToCapture = { StrengthCapture, AgilityCapture, IntellectCapture, StaminaCapture };
}
float UKrerusDerivedMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    FAggregatorEvaluateParameters Params;
    Params.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags(); Params.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
    auto Capture = [&](const FGameplayEffectAttributeCaptureDefinition& Def) { float V=0.f; GetCapturedAttributeMagnitude(Def, Spec, Params, V); return FMath::Max(0.f,V); };
    auto Coef = [&](const TCHAR* Name) { return Spec.GetSetByCallerMagnitude(FName(Name), false, 0.f); };
    switch (Stat)
    {
    case 0: return Capture(StrengthCapture)*Coef(TEXT("StrengthToAttackPower")) + Capture(AgilityCapture)*Coef(TEXT("AgilityToAttackPower"));
    case 1: return Capture(IntellectCapture)*Coef(TEXT("IntellectToSpellPower"));
    case 2: return Capture(StaminaCapture)*Coef(TEXT("StaminaToArmor"));
    case 3: return Capture(AgilityCapture)*Coef(TEXT("AgilityToCriticalChance"));
    // Preserve the existing GAS health/max-health at migration. Subsequent stamina changes scale the maximum.
    case 4: return (Capture(StaminaCapture)-Coef(TEXT("InitialStamina")))*Coef(TEXT("StaminaToHealth"));
    default: return 0.f;
    }
}
