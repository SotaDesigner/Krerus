#include "KrerusAutoAttackAbility.h"
#include "KrerusCombatComponent.h"
#include "KrerusCombatStateComponent.h"
#include "KrerusCharacter.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"
#include "KrerusProjectile.h"
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
    auto* Comp = Combat.Get();
    auto* C = Cast<AKrerusCharacter>(GetAvatarActorFromActorInfo());

    if (!IsActive() || bImpactConsumed || !Comp || !C || !C->HasAuthority() || !C->IsAutoAttackMontagePlaying())
    {
        return;
    }

    bImpactConsumed = true;

    if (Comp->IsAutoAttackPaused() || !Comp->CanAttackTarget() || Comp->GetCombatTarget() != Target.Get()) 
    {
        return;
    }

    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

    UAbilitySystemComponent* EnemyASC = TargetASC.Get();

    AKrerusEnemy* Enemy = Target.Get();

    const float Damage = Comp->GetAttackDamage();

    TSubclassOf<UGameplayEffect> DamageEffectClass = Comp->GetDamageEffectClass();

    if (!IsValid(SourceASC) || !IsValid(EnemyASC) || !IsValid(Enemy) || !DamageEffectClass || !FMath::IsFinite(Damage) || Damage <= 0.f)
    {
        return;
    }

    // -------------------------------------------------
    // ATAQUE A DISTANCIA
    // -------------------------------------------------

    if (Comp->UsesProjectileAttack())
    {
        if (!ProjectileClass)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("KRERUS AUTO: ProjectileClass no configurada")
            );

            return;
        }

        const FVector SpawnLocation = C->GetActorLocation() + C->GetActorForwardVector() * 100.f + FVector(0.f, 0.f, 60.f);

        const FRotator SpawnRotation = (Enemy->GetActorLocation() - SpawnLocation).Rotation();

        FTransform SpawnTransform(SpawnRotation, SpawnLocation);

        AKrerusProjectile* Projectile = GetWorld()->SpawnActorDeferred<AKrerusProjectile>(ProjectileClass,SpawnTransform,C,C,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

        if (!IsValid(Projectile))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("KRERUS AUTO: No se pudo crear el proyectil")
            );

            return;
        }

        Projectile->InitializeProjectile(Enemy, Damage, SourceASC, DamageEffectClass, OnHitResourceEffectClass, GetAbilityLevel());

        Projectile->FinishSpawning(SpawnTransform);

        UE_LOG(
            LogTemp,
            Log,
            TEXT("KRERUS AUTO: Disparo Owner=%s Target=%s Damage=%.3f"),
            *GetNameSafe(C),
            *GetNameSafe(Enemy),
            Damage
        );

        return;
    }

    // -------------------------------------------------
    // ATAQUE CUERPO A CUERPO
    // -------------------------------------------------

    Comp->ResolveAutoAttackHit(Enemy, Damage, SourceASC, DamageEffectClass,
        OnHitResourceEffectClass, GetAbilityLevel());
    // A lethal hit can already have ended this ability through its health delegate.
    if (IsValid(Enemy) && Enemy->IsCorpse()) StopAttack();
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
