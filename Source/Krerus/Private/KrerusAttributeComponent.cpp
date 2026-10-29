#include "KrerusAttributeComponent.h"
#include "KrerusAttributeSet.h"
#include "KrerusGASEffects.h"
#include "KrerusCombatComponent.h"
#include "KrerusCombatStateComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

UKrerusAttributeComponent::UKrerusAttributeComponent() { PrimaryComponentTick.bCanEverTick=false; }
UAbilitySystemComponent* UKrerusAttributeComponent::GetASC() const { return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()); }
const UKrerusAttributeSet* UKrerusAttributeComponent::GetAttributes() const { auto* ASC=GetASC(); return ASC ? ASC->GetSet<UKrerusAttributeSet>() : nullptr; }
void UKrerusAttributeComponent::BeginPlay() { Super::BeginPlay(); InitializeGAS(); }
void UKrerusAttributeComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* ASC=BoundASC.Get())
    {
        const FGameplayAttribute Attributes[] = {
            UKrerusAttributeSet::GetHealthAttribute(),
            UKrerusAttributeSet::GetMaxHealthAttribute(),
            UKrerusAttributeSet::GetManaAttribute(),
            UKrerusAttributeSet::GetMaxManaAttribute(),
            UKrerusAttributeSet::GetEnergyAttribute(),
            UKrerusAttributeSet::GetMaxEnergyAttribute(),
            UKrerusAttributeSet::GetRageAttribute(),
            UKrerusAttributeSet::GetMaxRageAttribute(),
            UKrerusAttributeSet::GetStrengthAttribute(),
            UKrerusAttributeSet::GetAgilityAttribute(),
            UKrerusAttributeSet::GetIntellectAttribute(),
            UKrerusAttributeSet::GetStaminaAttribute(),
            UKrerusAttributeSet::GetAttackPowerAttribute(),
            UKrerusAttributeSet::GetSpellPowerAttribute(),
            UKrerusAttributeSet::GetArmorAttribute(),
            UKrerusAttributeSet::GetCriticalChanceAttribute(),
        };
        for (const auto& A : Attributes) ASC->GetGameplayAttributeValueChangeDelegate(A).RemoveAll(this);
    }
    Super::EndPlay(Reason);
}
void UKrerusAttributeComponent::InitializeGAS()
{
    auto* ASC=GetASC(); if (!ASC || !ASC->GetAvatarActor() || !GetAttributes() || BoundASC.Get()==ASC) return;
    BoundASC=ASC;
    const FGameplayAttribute Attributes[] = {
        UKrerusAttributeSet::GetHealthAttribute(),
        UKrerusAttributeSet::GetMaxHealthAttribute(),
        UKrerusAttributeSet::GetManaAttribute(),
        UKrerusAttributeSet::GetMaxManaAttribute(),
        UKrerusAttributeSet::GetEnergyAttribute(),
        UKrerusAttributeSet::GetMaxEnergyAttribute(),
        UKrerusAttributeSet::GetRageAttribute(),
        UKrerusAttributeSet::GetMaxRageAttribute(),
        UKrerusAttributeSet::GetStrengthAttribute(),
        UKrerusAttributeSet::GetAgilityAttribute(),
        UKrerusAttributeSet::GetIntellectAttribute(),
        UKrerusAttributeSet::GetStaminaAttribute(),
        UKrerusAttributeSet::GetAttackPowerAttribute(),
        UKrerusAttributeSet::GetSpellPowerAttribute(),
        UKrerusAttributeSet::GetArmorAttribute(),
        UKrerusAttributeSet::GetCriticalChanceAttribute(),
    };
    for (const auto& A : Attributes) ASC->GetGameplayAttributeValueChangeDelegate(A).AddUObject(this,&UKrerusAttributeComponent::OnGASChanged);
    if (GetOwner()->HasAuthority())
    {
        FGameplayEffectQuery Q; Q.EffectDefinition=UKrerusDerivedStatsEffect::StaticClass();
        // ASC on PlayerState survives the pawn: do not reset stats/resources on a second initialization.
        if (ASC->GetActiveEffects(Q).IsEmpty())
        {
            auto Init=ASC->MakeOutgoingSpec(UKrerusInitialStatsEffect::StaticClass(),1.f,ASC->MakeEffectContext());
            if (!Init.IsValid()) return;
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("Strength")), FMath::Max(0.f,BaseAttributes.Strength));
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("Agility")), FMath::Max(0.f,BaseAttributes.Agility));
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("Intellect")), FMath::Max(0.f,BaseAttributes.Intellect));
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("Stamina")), FMath::Max(0.f,BaseAttributes.Stamina));
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("AttackPower")), 0.f);
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("SpellPower")), 0.f);
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("Armor")), 0.f);
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("CriticalChance")), 0.f);
            const float EnergyMax=Resource.Type==EKrerusResourceType::Energy ? FMath::Max(0.f,Resource.MaxValue) : 100.f;
            const float RageMax=Resource.Type==EKrerusResourceType::Rage ? FMath::Max(0.f,Resource.MaxValue) : 100.f;
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("MaxEnergy")),EnergyMax);
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("Energy")),EnergyMax);
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("MaxRage")),RageMax);
            Init.Data->SetSetByCallerMagnitude(FName(TEXT("Rage")),0.f);
            ASC->ApplyGameplayEffectSpecToSelf(*Init.Data.Get());
            auto Derived=ASC->MakeOutgoingSpec(UKrerusDerivedStatsEffect::StaticClass(),1.f,ASC->MakeEffectContext());
            if (!Derived.IsValid()) return;
            Derived.Data->SetSetByCallerMagnitude(FName(TEXT("StrengthToAttackPower")),AttributeScaling.StrengthToAttackPower);
            Derived.Data->SetSetByCallerMagnitude(FName(TEXT("AgilityToAttackPower")),AttributeScaling.AgilityToAttackPower);
            Derived.Data->SetSetByCallerMagnitude(FName(TEXT("IntellectToSpellPower")),AttributeScaling.IntellectToSpellPower);
            Derived.Data->SetSetByCallerMagnitude(FName(TEXT("AgilityToCriticalChance")),AttributeScaling.AgilityToCriticalChance);
            Derived.Data->SetSetByCallerMagnitude(FName(TEXT("StaminaToArmor")),AttributeScaling.StaminaToArmor);
            Derived.Data->SetSetByCallerMagnitude(FName(TEXT("StaminaToHealth")),AttributeScaling.StaminaToHealth);
            Derived.Data->SetSetByCallerMagnitude(FName(TEXT("InitialStamina")),FMath::Max(0.f,BaseAttributes.Stamina));
            ASC->ApplyGameplayEffectSpecToSelf(*Derived.Data.Get());
        }
    }
    OnHealthChanged.Broadcast(GetCurrentHealth(),GetMaxHealth());
    OnResourceChanged.Broadcast(Resource.Type,GetCurrentResource(),GetMaxResource());
}
float UKrerusAttributeComponent::GetCurrentHealth() const { auto* A=GetAttributes();return A ? A->GetHealth() : 0.f; }
float UKrerusAttributeComponent::GetMaxHealth() const { auto* A=GetAttributes();return A ? A->GetMaxHealth() : 0.f; }
bool UKrerusAttributeComponent::IsAlive() const { return GetCurrentHealth()>0.f; }
FName UKrerusAttributeComponent::ResourceName() const
{
    switch(Resource.Type) { case EKrerusResourceType::Mana:return FName(TEXT("Mana"));case EKrerusResourceType::Energy:return FName(TEXT("Energy"));case EKrerusResourceType::Rage:return FName(TEXT("Rage"));default:return NAME_None; }
}
float UKrerusAttributeComponent::GetCurrentResource() const
{
    auto* A=GetAttributes();if (!A)return 0.f;
    switch(Resource.Type) { case EKrerusResourceType::Mana:return A->GetMana();case EKrerusResourceType::Energy:return A->GetEnergy();case EKrerusResourceType::Rage:return A->GetRage();default:return 0.f; }
}
float UKrerusAttributeComponent::GetMaxResource() const
{
    auto* A=GetAttributes();if (!A)return 0.f;
    switch(Resource.Type) { case EKrerusResourceType::Mana:return A->GetMaxMana();case EKrerusResourceType::Energy:return A->GetMaxEnergy();case EKrerusResourceType::Rage:return A->GetMaxRage();default:return 0.f; }
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
void UKrerusAttributeComponent::OnGASChanged(const FOnAttributeChangeData& Data)
{
    if(Data.Attribute==UKrerusAttributeSet::GetHealthAttribute() || Data.Attribute==UKrerusAttributeSet::GetMaxHealthAttribute())
        OnHealthChanged.Broadcast(GetCurrentHealth(),GetMaxHealth());
    if(Data.Attribute==UKrerusAttributeSet::GetHealthAttribute() && Data.OldValue>0.f && Data.NewValue<=0.f)
    {
        if(GetOwner()->HasAuthority())
        {
            if(auto* C=GetOwner()->FindComponentByClass<UKrerusCombatComponent>())C->StopAutoAttack();
            if(auto* C=GetOwner()->FindComponentByClass<UKrerusCombatStateComponent>())C->ClearCombatRelations();
            if(auto* ASC=GetASC())ASC->CancelAllAbilities();
        }
        OnDeath.Broadcast();
    }
    OnResourceChanged.Broadcast(Resource.Type,GetCurrentResource(),GetMaxResource());
    OnAttributesChanged.Broadcast(GetFinalAttributes());
    OnDerivedAttributesChanged.Broadcast(GetDerivedAttributes());
}
