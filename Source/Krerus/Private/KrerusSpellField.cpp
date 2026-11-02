// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusSpellField.h"

#include "Components/BoxComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

#include "KrerusCharacter.h"
#include "KrerusEnemy.h"
#include "KrerusAttributeSet.h"
#include "KrerusCombatStateComponent.h"

// Sets default values
AKrerusSpellField::AKrerusSpellField()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(false);

	DetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionBox"));

	SetRootComponent(DetectionBox);

	DetectionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionBox->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
	DetectionBox->SetGenerateOverlapEvents(true);
	DetectionBox->OnComponentBeginOverlap.AddDynamic(this, &AKrerusSpellField::OnDetectionBegin);
	DetectionBox->OnComponentEndOverlap.AddDynamic(this, &AKrerusSpellField::OnDetectionEnd);
}


// Called when the game starts or when spawned
void AKrerusSpellField::BeginPlay()
{
	Super::BeginPlay();
	
	DetectionBox->SetBoxExtent(FVector(Radius, Radius, HalfHeight));

	if (!HasAuthority())
	{
		DetectionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

bool AKrerusSpellField::InitializeField(AKrerusCharacter* InCaster, float InAbilityLevel)
{
	if (!HasAuthority() || bInitialized || !IsValid(InCaster))
	{
		return false;
	}

	if(!FMath::IsFinite(Radius) || Radius <= 0.f ||
		!FMath::IsFinite(HalfHeight) || HalfHeight <= 0.f ||
		!FMath::IsFinite(FieldDuration) || FieldDuration <= 0.f ||
		!FMath::IsFinite(PulseInterval) || PulseInterval < 0.1f ||
		!FMath::IsFinite(ConeAngleDegrees) ||
		ConeAngleDegrees <= 0.f || ConeAngleDegrees > 180.f ||
		!FMath::IsFinite(InAbilityLevel) || InAbilityLevel <= 0.f ||
		!FMath::IsFinite(BaseDamage) || BaseDamage < 0.f ||
		!FMath::IsFinite(SpellPowerCoefficient) ||
		SpellPowerCoefficient < 0.f)
	{
		return false;
	}

	if (!PulseEffectClass && !WhileInsideEffectClass)
	{
		return false;
	}

	if (WhileInsideEffectClass)
	{
		const UGameplayEffect* Effect = WhileInsideEffectClass.GetDefaultObject();

		if (Effect->DurationPolicy != EGameplayEffectDurationType::Infinite || Effect->Period.GetValueAtLevel(InAbilityLevel) > 0.f || Effect->GetStackingType() != EGameplayEffectStackingType::None)
		{
			return false;
		}
	}

	if(PulseEffectClass)
	{
		const UGameplayEffect* Effect = PulseEffectClass.GetDefaultObject();

		if (Effect->DurationPolicy == EGameplayEffectDurationType::Infinite || Effect->Period.GetValueAtLevel(InAbilityLevel) > 0.f)
		{
			return false;
		}
	}
	
	Caster = InCaster;
	SourceASC = Caster->GetAbilitySystemComponent();

	const UKrerusAttributeSet* Attributes = IsValid(SourceASC) ? SourceASC->GetSet<UKrerusAttributeSet>() : nullptr;

	if (!Attributes || Attributes->GetHealth() <= 0.f)
	{
		return false;
	}

	AbilityLevel = InAbilityLevel;

	SnapshotDamage = BaseDamage + SpellPowerCoefficient * Attributes->GetSpellPower();

	if (!FMath::IsFinite(SnapshotDamage))
	{
		return false;
	}

	DetectionBox->SetBoxExtent(FVector(Radius, Radius, HalfHeight));

	bInitialized = true;

	DetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionBox->UpdateOverlaps();

	TArray<AActor*> InitialActors;

	DetectionBox->GetOverlappingActors(InitialActors, AKrerusEnemy::StaticClass());

	for (AActor* Actor : InitialActors)
	{
		if (AKrerusEnemy* Enemy = Cast<AKrerusEnemy>(Actor))
		{
			Candidates.Add(TWeakObjectPtr<AKrerusEnemy>(Enemy));
		}
	}

	StartTime = GetWorld()->GetTimeSeconds();
	NextPulseTime = StartTime + PulseInterval;

	RefreshInsideEffects();
	ApplyPulse();

	if (Application == EKrerusFieldApplication::Once)
	{
		Destroy();
		return true;
	}

	GetWorldTimerManager().SetTimer(UpdateTimer, this, &AKrerusSpellField::UpdateField, 0.1f, true);
	return true;

}

void AKrerusSpellField::OnDetectionBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bInitialized)
	{
		return;
	}

	if (AKrerusEnemy* Enemy = Cast<AKrerusEnemy>(OtherActor))
	{
		Candidates.Add(TWeakObjectPtr<AKrerusEnemy>(Enemy));
	}
}

void AKrerusSpellField::OnDetectionEnd(UPrimitiveComponent* OverlappendComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || !bInitialized)
	{
		return;
	}

	AKrerusEnemy* Enemy = Cast<AKrerusEnemy>(OtherActor);

	if (Enemy && !DetectionBox->IsOverlappingActor(Enemy))
	{
		Candidates.Remove(TWeakObjectPtr<AKrerusEnemy>(Enemy));
		RemoveInsideEffect(Enemy);
	}
}

bool AKrerusSpellField::IsValidTarget(AKrerusEnemy* Enemy) const
{
	if (!IsValid(Enemy) || Enemy->IsCorpse() || Enemy->GetWorld() != GetWorld())
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = Enemy->GetAbilitySystemComponent();

	const UKrerusAttributeSet* Attributes = IsValid(TargetASC) ? TargetASC->GetSet<UKrerusAttributeSet>() : nullptr;

	return Attributes && Attributes->GetHealth() > 0.f;
}

bool AKrerusSpellField::IsInsideShape(AKrerusEnemy* Enemy) const
{
	if (!IsValidTarget(Enemy))
	{
		return false;
	}

	FVector Delta = Enemy->GetActorLocation() - GetActorLocation();

	if (FMath::Abs(Delta.Z) > HalfHeight)
	{
		return false;
	}

	Delta.Z = 0.f;

	if (Delta.SizeSquared() > FMath::Square(Radius))
	{
		return false;
	}

	if (Shape == EKrerusFieldShape::Circle || Delta.IsNearlyZero())
	{
		return true;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();

	const float Dot = FVector::DotProduct(Forward, Delta.GetSafeNormal());

	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(ConeAngleDegrees * 0.5f));

	return Dot >= MinimumDot;
}

FActiveGameplayEffectHandle AKrerusSpellField::ApplyEffect(AKrerusEnemy* Enemy,
	TSubclassOf<UGameplayEffect> EffectClass, bool bSetDamage)
{
	if (!HasAuthority() || !IsValid(SourceASC) || !EffectClass || !IsValidTarget(Enemy))
	{
		return FActiveGameplayEffectHandle();
	}

	UAbilitySystemComponent* TargetASC = Enemy->GetAbilitySystemComponent();

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();

	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, AbilityLevel, Context);

	if (!Spec.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	if (bSetDamage)
	{
		const FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Data.Damage")));

		Spec.Data->SetSetByCallerMagnitude(DamageTag, -SnapshotDamage);
	}

	const FActiveGameplayEffectHandle Handle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	if (Handle.WasSuccessfullyApplied() && IsValid(Caster) && IsValidTarget(Enemy))
	{
		if (UKrerusCombatStateComponent* Combat = Caster->GetCombatStateComponent())
		{
			Combat->AddCombatRelation(Enemy);
		}
	}

	return Handle;

}

void AKrerusSpellField::RemoveInsideEffect(AKrerusEnemy* Enemy)
{
	const TWeakObjectPtr<AKrerusEnemy> Key(Enemy);

	if (FActiveGameplayEffectHandle* Handle = InsideEffects.Find(Key))
	{
		if (IsValid(Enemy))
		{
			if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
			{
				ASC->RemoveActiveGameplayEffect(*Handle);
			}
		}

		InsideEffects.Remove(Key);
	}
}

void AKrerusSpellField::RefreshInsideEffects()
{
	for (auto It = InsideEffects.CreateIterator(); It; ++It)
	{
		AKrerusEnemy* Enemy = It.Key().Get();

		if (!Enemy || !Candidates.Contains(It.Key()) || !IsInsideShape(Enemy))
		{
			if (IsValid(Enemy))
			{
				if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
				{
					ASC->RemoveActiveGameplayEffect(It.Value());
				}
			}

			It.RemoveCurrent();
		}
	}

	if (!WhileInsideEffectClass)
	{
		return;
	}

	const TArray<TWeakObjectPtr<AKrerusEnemy>> Snapshot = Candidates.Array();

	for (const auto& Key : Snapshot)
	{
		AKrerusEnemy* Enemy = Key.Get();

		if (IsInsideShape(Enemy) && !InsideEffects.Contains(Key))
		{
			const FActiveGameplayEffectHandle Handle = ApplyEffect(Enemy, WhileInsideEffectClass, false);

			if (Handle.IsValid())
			{
				InsideEffects.Add(Key, Handle);
			}
		}
	}
}

void AKrerusSpellField::ApplyPulse()
{
	if (!PulseEffectClass)
	{
		return;
	}

	const TArray<TWeakObjectPtr<AKrerusEnemy>> SnapShot = Candidates.Array();

	for (const auto& Key : SnapShot)
	{
		AKrerusEnemy* Enemy = Key.Get();
	
		if (IsInsideShape(Enemy))
		{
			ApplyEffect(Enemy, PulseEffectClass, true);
		}
	}
}

void AKrerusSpellField::UpdateField()
{
	if (!HasAuthority() || !bInitialized)
	{
		return;
	}

	const UKrerusAttributeSet* Attributes = IsValid(SourceASC) ? SourceASC->GetSet<UKrerusAttributeSet>() : nullptr;

	const double Now = GetWorld()->GetTimeSeconds();

	if (!IsValid(Caster) || !Attributes || (bDestroyWhenCasterDies && Attributes->GetHealth() <= 0.f) || Now - StartTime >= FieldDuration)
	{
		Destroy();
		return;
	}

	for (auto It = Candidates.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	RefreshInsideEffects();

	if (Now >= NextPulseTime)
	{
		ApplyPulse();

		NextPulseTime = Now + PulseInterval;
	}
}

void AKrerusSpellField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(UpdateTimer);

	if (HasAuthority())
	{
		for (const auto& Entry : InsideEffects)
		{
			if (AKrerusEnemy* Enemy = Entry.Key.Get())
			{
				if (UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
				{
					ASC->RemoveActiveGameplayEffect(Entry.Value);
				}
			}
		}
	}

	InsideEffects.Empty();
	Candidates.Empty();

	Super::EndPlay(EndPlayReason);
	
}

