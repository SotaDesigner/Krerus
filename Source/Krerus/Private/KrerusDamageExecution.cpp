
#include "KrerusDamageExecution.h"

#include "KrerusAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_KrerusDataDamage,"Data.Damage");

struct FKrerusDamageStatics
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);

    FKrerusDamageStatics()
    {
        DEFINE_ATTRIBUTE_CAPTUREDEF(UKrerusAttributeSet, Armor, Target, false);
    }
};

static const FKrerusDamageStatics& DamageStatics()
{
    static FKrerusDamageStatics Statics;
    return Statics;
}

UKrerusDamageExecution::UKrerusDamageExecution()
{
    RelevantAttributesToCapture.Add(
        DamageStatics().ArmorDef
    );
}

void UKrerusDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

    const float RawDamage = FMath::Abs(Spec.GetSetByCallerMagnitude(TAG_KrerusDataDamage, false, 0.0f));

    if (!FMath::IsFinite(RawDamage) || RawDamage <= 0.0f)
    {
        return;
    }

    FAggregatorEvaluateParameters EvaluateParameters;

    EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();

    EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    float Armor = 0.0f;

    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude( DamageStatics().ArmorDef, EvaluateParameters, Armor);

    Armor = FMath::IsFinite(Armor) ? FMath::Max(0.0f, Armor) : 0.0f;

    const float SafeConstant = FMath::Max(1.0f, ArmorConstant);
    const float FinalDamage = RawDamage * SafeConstant / (SafeConstant + Armor);

    OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UKrerusAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -FinalDamage));
}
