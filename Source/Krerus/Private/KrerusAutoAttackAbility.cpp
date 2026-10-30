#include "KrerusAutoAttackAbility.h"
#include "KrerusCombatComponent.h"
#include "KrerusCombatStateComponent.h"
#include "KrerusCharacter.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "NativeGameplayTags.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_KrerusAutoAttackImpact,"Event.Krerus.AutoAttack.Impact");
UKrerusAutoAttackAbility::UKrerusAutoAttackAbility()
{
    InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy=EGameplayAbilityNetExecutionPolicy::ServerOnly;
}
void UKrerusAutoAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle H,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event)
{
    auto* C=Info?Cast<AKrerusCharacter>(Info->AvatarActor.Get()):nullptr;

    auto* Comp=C?C->GetCombatComponent():nullptr;

    if(!C || !C->HasAuthority() || !Comp || !Comp->CanAttackTarget() || !Comp->GetDamageEffectClass() || !CommitAbility(H,Info,Activation))
    {
        EndAbility(H,Info,Activation,true,true);return;
    }

    Combat=Comp;Target=Comp->GetCombatTarget();TargetASC=Target->GetAbilitySystemComponent();
    
    if(!TargetASC.IsValid()) {EndAbility(H,Info,Activation,true,true);return;}
    
    TargetASC->GetGameplayAttributeValueChangeDelegate(UKrerusAttributeSet::GetHealthAttribute()).AddUObject(this,&UKrerusAutoAttackAbility::OnHealthChanged);
    
    Info->AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UKrerusAttributeSet::GetHealthAttribute()).AddUObject(this,&UKrerusAutoAttackAbility::OnHealthChanged);
    
    auto* Impact=UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,TAG_KrerusAutoAttackImpact,nullptr,false,true);
    
    Impact->EventReceived.AddDynamic(this,&UKrerusAutoAttackAbility::OnImpact);Impact->ReadyForActivation();
    
    Comp->PublishAbilityState(true);
    
    NextSwingTime=FMath::Max(NextSwingTime,double(GetWorld()->GetTimeSeconds())+Comp->GetAttackInterval());
    
    Schedule(float(NextSwingTime-GetWorld()->GetTimeSeconds()));
}

void UKrerusAutoAttackAbility::Schedule(float Delay)
{
    if(!IsActive())return;
    if(DelayTask){DelayTask->EndTask();DelayTask=nullptr;}
    DelayTask=UAbilityTask_WaitDelay::WaitDelay(this,FMath::Max(0.001f,Delay));
    DelayTask->OnFinish.AddDynamic(this,&UKrerusAutoAttackAbility::TrySwing);DelayTask->ReadyForActivation();
}
void UKrerusAutoAttackAbility::TrySwing()
{
    DelayTask=nullptr;
    auto* Comp=Combat.Get();auto* C=Cast<AKrerusCharacter>(GetAvatarActorFromActorInfo());
    auto* ASC=GetAbilitySystemComponentFromActorInfo();auto* A=ASC?ASC->GetSet<UKrerusAttributeSet>():nullptr;
    if(!IsActive() || !Comp || !C || !A || A->GetHealth()<=0.f || !Target.IsValid() || Target->IsCorpse() || Comp->GetCombatTarget()!=Target.Get()) {StopAttack();return;}
    if(Comp->IsAutoAttackPaused())return; // Resume explicitly starts a new task.
    const double Remaining=NextSwingTime-GetWorld()->GetTimeSeconds();
    if(Remaining>0.0) {Schedule(float(Remaining));return;}
    if(!Comp->CanAttackTarget()) {Schedule(0.05f);return;}
    if(C->IsAutoAttackMontagePlaying()) {Schedule(0.01f);return;}
    bImpactConsumed=true;
    C->MulticastAutoAttackMontage(true,Comp->GetAttackInterval());
    if(!C->IsAutoAttackMontagePlaying()) {StopAttack();return;}
    bImpactConsumed=false;
    NextSwingTime=double(GetWorld()->GetTimeSeconds())+Comp->GetAttackInterval();
    UE_LOG(LogTemp,Log,TEXT("KRERUS GAS AUTO: Swing Owner=%s Game=%.6f Interval=%.3f"),*C->GetName(),double(GetWorld()->GetTimeSeconds()),Comp->GetAttackInterval());
    Schedule(Comp->GetAttackInterval());
}
void UKrerusAutoAttackAbility::SendImpactEvent()
{
    if(!IsActive())return;
    FGameplayEventData Event;Event.EventTag=TAG_KrerusAutoAttackImpact;Event.Instigator=GetAvatarActorFromActorInfo();Event.Target=Target.Get();
    if(auto* ASC=GetAbilitySystemComponentFromActorInfo())ASC->HandleGameplayEvent(TAG_KrerusAutoAttackImpact,&Event);
}
void UKrerusAutoAttackAbility::OnImpact(FGameplayEventData Payload)
{
    auto* Comp=Combat.Get();auto* C=Cast<AKrerusCharacter>(GetAvatarActorFromActorInfo());
    if(!IsActive() || bImpactConsumed || !Comp || !C || !C->HasAuthority() || !C->IsAutoAttackMontagePlaying())return;
    bImpactConsumed=true;
    if(Comp->IsAutoAttackPaused() || !Comp->CanAttackTarget() || Comp->GetCombatTarget()!=Target.Get())return;
    auto* ASC=GetAbilitySystemComponentFromActorInfo();auto* EnemyASC=TargetASC.Get();auto* Enemy=Target.Get();
    const float Damage=Comp->GetAttackDamage();
    if(!ASC || !EnemyASC || !Enemy || !FMath::IsFinite(Damage) || Damage<=0.f)return;
    if (OnHitResourceEffectClass)
    {
        FGameplayEffectSpecHandle ResourceSpec =
            ASC->MakeOutgoingSpec(
                OnHitResourceEffectClass,
                GetAbilityLevel(),
                ASC->MakeEffectContext()
            );

        if (ResourceSpec.IsValid())
        {
            const FActiveGameplayEffectHandle ResourceHandle =
                ASC->ApplyGameplayEffectSpecToSelf(*ResourceSpec.Data.Get());

            const UKrerusAttributeSet* Attributes =
                ASC->GetSet<UKrerusAttributeSet>();

            if (Attributes)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("KRERUS RAGE: Jugador=%s | Ira actual=%.1f | Efecto aplicado=%s"),
                    *GetNameSafe(C),
                    Attributes->GetRage(),
                    ResourceHandle.WasSuccessfullyApplied() ? TEXT("SI") : TEXT("NO"));
            }
        }
    }
    auto Context=ASC->MakeEffectContext();Context.AddSourceObject(C);
    auto Spec=ASC->MakeOutgoingSpec(Comp->GetDamageEffectClass(),GetAbilityLevel(),Context);if(!Spec.IsValid())return;
    Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName(TEXT("Data.Damage"))),-Damage);
    // Hold local weak references: target death can synchronously end this ability during effect application.
    if(!ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(),EnemyASC).WasSuccessfullyApplied())return;
    if(!Enemy->IsCorpse())if(auto* State=C->GetCombatStateComponent())State->AddCombatRelation(Enemy);
    Comp->OnAutoAttackHit.Broadcast(Enemy,Damage);
    UE_LOG(LogTemp,Log,TEXT("KRERUS GAS AUTO: Impact Owner=%s Target=%s Game=%.6f Damage=%.3f"),*C->GetName(),*Enemy->GetName(),double(C->GetWorld()->GetTimeSeconds()),Damage);
    if(Enemy->IsCorpse())StopAttack();
}
void UKrerusAutoAttackAbility::RefreshPause()
{
    if(!IsActive() || !Combat.IsValid())return;
    if(Combat->IsAutoAttackPaused())
    {
        bImpactConsumed=true;
        if(DelayTask){DelayTask->EndTask();DelayTask=nullptr;}
        if(auto* C=Cast<AKrerusCharacter>(GetAvatarActorFromActorInfo()))C->MulticastAutoAttackMontage(false,Combat->GetAttackInterval());
    }
    else Schedule(float(FMath::Max(0.001,NextSwingTime-GetWorld()->GetTimeSeconds())));
}
void UKrerusAutoAttackAbility::OnHealthChanged(const FOnAttributeChangeData& Data) { if(Data.NewValue<=0.f)StopAttack(); }
void UKrerusAutoAttackAbility::StopAttack() { if(IsActive())EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true); }
void UKrerusAutoAttackAbility::EndAbility(const FGameplayAbilitySpecHandle H,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo Activation,bool bReplicate,bool bCancelled)
{
    bImpactConsumed=true;
    if(auto* ASC=TargetASC.Get())ASC->GetGameplayAttributeValueChangeDelegate(UKrerusAttributeSet::GetHealthAttribute()).RemoveAll(this);
    if(Info && Info->AbilitySystemComponent.IsValid())Info->AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UKrerusAttributeSet::GetHealthAttribute()).RemoveAll(this);
    if(auto* C=Info?Cast<AKrerusCharacter>(Info->AvatarActor.Get()):nullptr)
        if(C->HasAuthority() && Combat.IsValid())C->MulticastAutoAttackMontage(false,Combat->GetAttackInterval());
    if(auto* Comp=Combat.Get())Comp->PublishAbilityState(false);
    Combat.Reset();Target.Reset();TargetASC.Reset();DelayTask=nullptr;
    Super::EndAbility(H,Info,Activation,bReplicate,bCancelled);
}
