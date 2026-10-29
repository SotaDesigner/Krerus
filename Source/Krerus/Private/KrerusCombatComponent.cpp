#include "KrerusCombatComponent.h"
#include "KrerusCharacter.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"
#include "KrerusAutoAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

UKrerusCombatComponent::UKrerusCombatComponent() { PrimaryComponentTick.bCanEverTick=false;SetIsReplicatedByDefault(true); }
void UKrerusCombatComponent::EndPlay(const EEndPlayReason::Type Reason) { if(GetOwner()->HasAuthority())StopAutoAttack();Super::EndPlay(Reason); }
void UKrerusCombatComponent::SetCombatTarget(AKrerusEnemy* Target)
{
    if(CombatTarget.Get()==Target)return;
    StopAutoAttack();CombatTarget=Target;
}
AKrerusEnemy* UKrerusCombatComponent::GetCombatTarget() const { return CombatTarget.Get(); }
UKrerusAutoAttackAbility* UKrerusCombatComponent::GetAutoAttackAbility() const
{
    auto* C=Cast<AKrerusCharacter>(GetOwner());auto* ASC=C?C->GetAbilitySystemComponent():nullptr;
    if(!ASC)return nullptr;
    for(const auto& Spec:ASC->GetActivatableAbilities())
        if(Spec.Ability && Spec.Ability->IsA(UKrerusAutoAttackAbility::StaticClass()))return Cast<UKrerusAutoAttackAbility>(Spec.GetPrimaryInstance());
    return nullptr;
}
bool UKrerusCombatComponent::StartAutoAttack()
{
    auto* C=Cast<AKrerusCharacter>(GetOwner());if(!C)return false;
    if(!C->HasAuthority()) { if(!IsValid(CombatTarget.Get()) || CombatTarget->IsCorpse())return false;C->ServerStartAutoAttack(CombatTarget.Get());return true; }
    if(bIsAutoAttacking || !CanAttackTarget())return false;
    auto* ASC=C->GetAbilitySystemComponent();if(!ASC)return false;
    FGameplayAbilitySpecHandle H;
    for(const auto& S:ASC->GetActivatableAbilities())if(S.Ability && S.Ability->IsA(UKrerusAutoAttackAbility::StaticClass())) { H=S.Handle;break; }
    return H.IsValid() && ASC->TryActivateAbility(H);
}
void UKrerusCombatComponent::StopAutoAttack()
{
    auto* C=Cast<AKrerusCharacter>(GetOwner());if(!C)return;
    if(!C->HasAuthority()) { if(!C->IsActorBeingDestroyed())C->ServerStopAutoAttack();return; }
    if(auto* A=GetAutoAttackAbility()) A->StopAttack();
    PublishAbilityState(false);
}
void UKrerusCombatComponent::PublishAbilityState(bool bActive)
{
    if(!GetOwner()->HasAuthority() || bIsAutoAttacking==bActive)return;
    bIsAutoAttacking=bActive;
    if(!bActive)bAutoAttackPaused=false;
    OnRep_AutoAttacking();GetOwner()->ForceNetUpdate();
}
bool UKrerusCombatComponent::IsAutoAttacking() const { return bIsAutoAttacking; }
bool UKrerusCombatComponent::IsAutoAttackPaused() const { return bAutoAttackPaused; }
float UKrerusCombatComponent::GetAttackInterval() const { return FMath::IsFinite(AttackInterval)?FMath::Max(0.1f,AttackInterval):2.f; }
float UKrerusCombatComponent::GetAttackDamage() const
{
    auto* C=Cast<AKrerusCharacter>(GetOwner());auto* ASC=C?C->GetAbilitySystemComponent():nullptr;auto* A=ASC?ASC->GetSet<UKrerusAttributeSet>():nullptr;
    return A?FMath::Max(0.f,BaseAttackDamage+A->GetAttackPower()*AttackPowerCoefficient):0.f;
}
bool UKrerusCombatComponent::CanAttackTarget() const
{
    auto* C=Cast<AKrerusCharacter>(GetOwner());auto* T=CombatTarget.Get();auto* ASC=C?C->GetAbilitySystemComponent():nullptr;
    auto* A=ASC?ASC->GetSet<UKrerusAttributeSet>():nullptr;
    return C && A && A->GetHealth()>0.f && IsValid(T) && T->GetWorld()==GetWorld() && !T->IsCorpse() && IsTargetInRange(C,T) && IsTargetInFront(C,T);
}
bool UKrerusCombatComponent::IsTargetInRange(const AKrerusCharacter* C,const AKrerusEnemy* T) const { return C && T && FVector::Dist2D(C->GetActorLocation(),T->GetActorLocation())<=AttackRange; }
bool UKrerusCombatComponent::IsTargetInFront(const AKrerusCharacter* C,const AKrerusEnemy* T) const { return C && T && FVector::DotProduct(C->GetActorForwardVector().GetSafeNormal2D(),(T->GetActorLocation()-C->GetActorLocation()).GetSafeNormal2D())>=0.f; }
void UKrerusCombatComponent::ApplyAutoAttackImpact() { if(GetOwner()->HasAuthority())if(auto* A=GetAutoAttackAbility())A->SendImpactEvent(); }
void UKrerusCombatComponent::CancelPendingAutoAttackImpact() { if(GetOwner()->HasAuthority())if(auto* A=GetAutoAttackAbility())A->InvalidateImpact(); }
void UKrerusCombatComponent::SetAutoAttackPaused(bool bPaused)
{
    auto* C=Cast<AKrerusCharacter>(GetOwner());if(!C)return;
    if(!C->HasAuthority()) {C->ServerSetAutoAttackPaused(bPaused);return;}
    if(bAutoAttackPaused==bPaused)return;
    bAutoAttackPaused=bPaused;
    if(auto* A=GetAutoAttackAbility())A->RefreshPause();
    C->ForceNetUpdate();
}
void UKrerusCombatComponent::OnRep_AutoAttacking() { if(bIsAutoAttacking)OnAutoAttackStarted.Broadcast();else OnAutoAttackStopped.Broadcast(); }
void UKrerusCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKrerusCombatComponent,CombatTarget,COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKrerusCombatComponent,bIsAutoAttacking,COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKrerusCombatComponent,bAutoAttackPaused,COND_OwnerOnly);
}
