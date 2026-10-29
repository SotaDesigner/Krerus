#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.h"
#include "KrerusGASEffects.generated.h"

UCLASS()
class KRERUS_API UKrerusInitialStatsEffect : public UGameplayEffect
{ GENERATED_BODY() public: UKrerusInitialStatsEffect(); };
UCLASS()
class KRERUS_API UKrerusAttributeBonusEffect : public UGameplayEffect
{ GENERATED_BODY() public: UKrerusAttributeBonusEffect(); };
UCLASS()
class KRERUS_API UKrerusResourceDeltaEffect : public UGameplayEffect
{ GENERATED_BODY() public: UKrerusResourceDeltaEffect(); };
UCLASS()
class KRERUS_API UKrerusDerivedStatsEffect : public UGameplayEffect
{ GENERATED_BODY() public: UKrerusDerivedStatsEffect(); };

UCLASS(Abstract)
class KRERUS_API UKrerusDerivedMagnitude : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()
public:
    UKrerusDerivedMagnitude();
    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
protected:
    int32 Stat = 0;
    FGameplayEffectAttributeCaptureDefinition StrengthCapture, AgilityCapture, IntellectCapture, StaminaCapture;
};
UCLASS()
class KRERUS_API UKrerusAttackPowerMagnitude : public UKrerusDerivedMagnitude
{ GENERATED_BODY() public: UKrerusAttackPowerMagnitude() { Stat = 0; } };
UCLASS()
class KRERUS_API UKrerusSpellPowerMagnitude : public UKrerusDerivedMagnitude
{ GENERATED_BODY() public: UKrerusSpellPowerMagnitude() { Stat = 1; } };
UCLASS()
class KRERUS_API UKrerusArmorMagnitude : public UKrerusDerivedMagnitude
{ GENERATED_BODY() public: UKrerusArmorMagnitude() { Stat = 2; } };
UCLASS()
class KRERUS_API UKrerusCriticalMagnitude : public UKrerusDerivedMagnitude
{ GENERATED_BODY() public: UKrerusCriticalMagnitude() { Stat = 3; } };
UCLASS()
class KRERUS_API UKrerusHealthMagnitude : public UKrerusDerivedMagnitude
{ GENERATED_BODY() public: UKrerusHealthMagnitude() { Stat = 4; } };
