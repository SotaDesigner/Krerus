#include "KrerusAttributeComponent.h"
#include "KrerusAttributeSet.h"
#include "KrerusGASEffects.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "KrerusPlayerState.h"
#include "KrerusClassTypes.h"
#include "GameFramework/Pawn.h"

UKrerusAttributeComponent::UKrerusAttributeComponent() { PrimaryComponentTick.bCanEverTick=false; }
UAbilitySystemComponent* UKrerusAttributeComponent::GetASC() const { return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()); }
const UKrerusAttributeSet* UKrerusAttributeComponent::GetAttributes() const { auto* ASC=GetASC(); return ASC ? ASC->GetSet<UKrerusAttributeSet>() : nullptr; }

float UKrerusAttributeComponent::GetCurrentHealth() const { auto* A=GetAttributes();return A ? A->GetHealth() : 0.f; }
float UKrerusAttributeComponent::GetMaxHealth() const { auto* A=GetAttributes();return A ? A->GetMaxHealth() : 0.f; }
bool UKrerusAttributeComponent::IsAlive() const { return GetCurrentHealth()>0.f; }

EKrerusResourceType UKrerusAttributeComponent::GetResourceType() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());

    if (!Pawn)
    {
        return EKrerusResourceType::None;
    }

    const AKrerusPlayerState* PS = Pawn->GetPlayerState<AKrerusPlayerState>();

    if (!PS)
    {
        return EKrerusResourceType::None;
    }

    return GetKrerusClassResource(PS->GetCharacterClass());
}

float UKrerusAttributeComponent::GetCurrentResource() const
{
    auto* A=GetAttributes();
    if (!A)return 0.f;
    switch(GetResourceType()) 
    { 
        case EKrerusResourceType::Mana:
            return A->GetMana();
        case EKrerusResourceType::Energy:
            return A->GetEnergy();
        case EKrerusResourceType::Rage:
            return A->GetRage();
        default:
            return 0.f;
    }
}

float UKrerusAttributeComponent::GetMaxResource() const
{
    auto* A=GetAttributes();
    if (!A)return 0.f;
    switch(GetResourceType()) 
    { 
        case EKrerusResourceType::Mana:
            return A->GetMaxMana();
        case EKrerusResourceType::Energy:
            return A->GetMaxEnergy();
        case EKrerusResourceType::Rage:
            return A->GetMaxRage();
        default:
            return 0.f; 
    }
}
  
bool UKrerusAttributeComponent::ApplyDelta(FName Name,float Amount)
{
    auto* ASC=GetASC();if(!ASC || !GetOwner()->HasAuthority() || Name.IsNone() || !FMath::IsFinite(Amount))return false;
    auto Spec=ASC->MakeOutgoingSpec(UKrerusResourceDeltaEffect::StaticClass(),1.f,ASC->MakeEffectContext());if(!Spec.IsValid())return false;
    for (const auto* N : {TEXT("Health"),TEXT("Mana"),TEXT("Energy"),TEXT("Rage")}) Spec.Data->SetSetByCallerMagnitude(FName(N),Name==FName(N)?Amount:0.f);
    return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()).WasSuccessfullyApplied();
}

void UKrerusAttributeComponent::ApplyDamage(float Amount) { if (FMath::IsFinite(Amount) && Amount>0.f && IsAlive()) ApplyDelta(FName(TEXT("Health")),-Amount); }
void UKrerusAttributeComponent::Heal(float Amount) { if (FMath::IsFinite(Amount) && Amount>0.f && IsAlive()) ApplyDelta(FName(TEXT("Health")),Amount); }
bool UKrerusAttributeComponent::TryConsumeResource(float Amount) { return FMath::IsFinite(Amount) && Amount>0.f && GetCurrentResource()>=Amount && ApplyDelta(ResourceName(),-Amount); }
void UKrerusAttributeComponent::RestoreResource(float Amount) { if (FMath::IsFinite(Amount) && Amount>0.f) ApplyDelta(ResourceName(),Amount); }

FKrerusPrimaryAttributes UKrerusAttributeComponent::GetFinalAttributes() const
{
    FKrerusPrimaryAttributes V;auto* A=GetAttributes();
    V.Strength=A?A->GetStrength():0.f;V.Agility=A?A->GetAgility():0.f;V.Intellect=A?A->GetIntellect():0.f;V.Stamina=A?A->GetStamina():0.f;return V;
}

FKrerusDerivedAttributes UKrerusAttributeComponent::GetDerivedAttributes() const
{
    FKrerusDerivedAttributes V;auto* A=GetAttributes();
    if (A) {V.AttackPower=A->GetAttackPower();V.SpellPower=A->GetSpellPower();V.Armor=A->GetArmor();V.CriticalChance=A->GetCriticalChance();}return V;
}

FGuid UKrerusAttributeComponent::AddAttributeModifier(FKrerusAttributeModifier Modifier)
{
    auto* ASC=GetASC();if(!ASC || !GetOwner()->HasAuthority())return FGuid();
    auto Spec=ASC->MakeOutgoingSpec(UKrerusAttributeBonusEffect::StaticClass(),1.f,ASC->MakeEffectContext());if(!Spec.IsValid())return FGuid();
    if(!FMath::IsFinite(Modifier.Strength))return FGuid();
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("Strength")),Modifier.Strength);
    if(!FMath::IsFinite(Modifier.Agility))return FGuid();
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("Agility")),Modifier.Agility);
    if(!FMath::IsFinite(Modifier.Intellect))return FGuid();
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("Intellect")),Modifier.Intellect);
    if(!FMath::IsFinite(Modifier.Stamina))return FGuid();
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("Stamina")),Modifier.Stamina);
    if(!FMath::IsFinite(Modifier.AttackPower))return FGuid();
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("AttackPower")),Modifier.AttackPower);
    if(!FMath::IsFinite(Modifier.SpellPower))return FGuid();
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("SpellPower")),Modifier.SpellPower);
    if(!FMath::IsFinite(Modifier.Armor))return FGuid();
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("Armor")),Modifier.Armor);
    if(!FMath::IsFinite(Modifier.CriticalChance))return FGuid();
    Spec.Data->SetSetByCallerMagnitude(FName(TEXT("CriticalChance")),Modifier.CriticalChance);
    auto Handle=ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());if(!Handle.IsValid())return FGuid();
    auto Id=FGuid::NewGuid();ModifierHandles.Add(Id,Handle);return Id;
}

bool UKrerusAttributeComponent::RemoveAttributeModifier(FGuid Id)
{
    auto* ASC=GetASC();auto* H=ModifierHandles.Find(Id);if(!ASC || !GetOwner()->HasAuthority() || !H)return false;
    bool Result=ASC->RemoveActiveGameplayEffect(*H);ModifierHandles.Remove(Id);return Result;
}

FName UKrerusAttributeComponent::ResourceName() const
{
    switch (GetResourceType())
    {
    case EKrerusResourceType::Mana:
        return TEXT("Mana");

    case EKrerusResourceType::Energy:
        return TEXT("Energy");

    case EKrerusResourceType::Rage:
        return TEXT("Rage");

    default:
        return NAME_None;
    }
}
