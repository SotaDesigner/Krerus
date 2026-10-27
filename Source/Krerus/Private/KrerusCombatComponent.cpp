// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusCombatComponent.h"
#include "KrerusEnemy.h"
#include "TimerManager.h"
#include "KrerusAttributeComponent.h"
#include "KrerusCharacter.h"

// Sets default values for this component's properties
UKrerusCombatComponent::UKrerusCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UKrerusCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetOwner());

	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent();

	if (!IsValid(PlayerAttributes))
	{
		return;
	}

	PlayerAttributes->OnDeath.AddUniqueDynamic(this,&UKrerusCombatComponent::OnOwnerDeath);
	
}

void UKrerusCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAutoAttack();

	if (AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetOwner()))
	{
		if (UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent())
		{
			PlayerAttributes->OnDeath.RemoveDynamic(this,&UKrerusCombatComponent::OnOwnerDeath);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UKrerusCombatComponent::SetCombatTarget(AKrerusEnemy* NewTarget)
{
	// Si es el mismo objetivo, no hacemos nada.
	if (CombatTarget.Get() == NewTarget)
	{
		return;
	}

	// Detenemos el combate anterior antes de cambiar de objetivo.
	StopAutoAttack();

	// Guardamos el nuevo objetivo.
	CombatTarget = NewTarget;
}

AKrerusEnemy* UKrerusCombatComponent::GetCombatTarget() const
{
	return CombatTarget.Get();
}

bool UKrerusCombatComponent::StartAutoAttack()
{
	if (!IsValid(CombatTarget.Get()))
	{
		return false;
	}

	if (bIsAutoAttacking)
	{
		return false;
	}

	AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetOwner());

	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent();

	if (!IsValid(PlayerAttributes) || !PlayerAttributes->IsAlive())
	{
		UE_LOG(LogTemp,Warning,TEXT("KRERUS: No se puede iniciar el autoataque. El jugador esta muerto."));

		return false;
	}

	UKrerusAttributeComponent* TargetAttributes = CombatTarget->GetAttributeComponent();

	if (!IsValid(TargetAttributes) || !TargetAttributes->IsAlive())
	{
		return false;
	}

	TargetAttributes->OnDeath.AddUniqueDynamic(this,&UKrerusCombatComponent::OnCombatTargetDeath);

	if (!IsValid(CombatTarget.Get()) || CombatTarget->IsCorpse())
	{
		UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: No se puede atacar. Objetivo invalido o muerto."));

		return false;
	}

	bIsAutoAttacking = true;

	GetWorld()->GetTimerManager().SetTimer(AutoAttackTimerHandle, this, &UKrerusCombatComponent::PerformAutoAttack, GetAttackInterval(), true);
	OnAutoAttackStarted.Broadcast();

	return true;
}

void UKrerusCombatComponent::StopAutoAttack()
{
	const bool bWasAutoAttacking = bIsAutoAttacking;

	if (IsValid(CombatTarget.Get()))
	{
		UKrerusAttributeComponent* TargetAttributes = CombatTarget->GetAttributeComponent();

		if (IsValid(TargetAttributes))
		{
			TargetAttributes->OnDeath.RemoveDynamic(this,&UKrerusCombatComponent::OnCombatTargetDeath);
		}
	}

	bIsAutoAttacking = false;
	bAttackImpactConsumed = true;
	bAutoAttackPaused = false;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
	}

	if (bWasAutoAttacking)
	{
		OnAutoAttackStopped.Broadcast();
	}
}

bool UKrerusCombatComponent::IsAutoAttacking() const
{
	return bIsAutoAttacking;
}

float UKrerusCombatComponent::GetAttackInterval() const
{
	return FMath::Max(0.1f, AttackInterval);
}

void UKrerusCombatComponent::ApplyAutoAttackImpact()
{
	AKrerusCharacter* PlayerCharacter =	Cast<AKrerusCharacter>(GetOwner());

	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsAutoAttackMontagePlaying())
	{
		UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: Impacto descartado. Montage de ataque inactivo."));

		return;
	}

	if (bAttackImpactConsumed)
	{
		return;
	}

	bAttackImpactConsumed = true;

	if (!bIsAutoAttacking)
	{
		return;
	}

	AKrerusEnemy* Target = CombatTarget.Get();

	if (!IsValid(Target))
	{
		StopAutoAttack();
		return;
	}

	UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent();

	UKrerusAttributeComponent* TargetAttributes = Target->GetAttributeComponent();

	if (!IsValid(PlayerAttributes) || !IsValid(TargetAttributes))
	{
		StopAutoAttack();
		return;
	}

	if (!PlayerAttributes->IsAlive() || !TargetAttributes->IsAlive())
	{
		StopAutoAttack();
		return;
	}

	if (!IsTargetInRange(PlayerCharacter,Target))
	{
		return;
	}

	if (!IsTargetInFront(PlayerCharacter,Target))
	{
		UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: Impacto fallido. Objetivo fuera del arco frontal."));

		return;
	}

	const FKrerusDerivedAttributes DerivedAttributes =	PlayerAttributes->GetDerivedAttributes();

	const float Damage = BaseAttackDamage +	DerivedAttributes.AttackPower * AttackPowerCoefficient;

	TargetAttributes->ApplyDamage(Damage);

	UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: Impacto aplicado a %s | Daño: %.1f | Vida restante: %.1f"),*Target->GetName(),Damage,TargetAttributes->GetCurrentHealth());

	OnAutoAttackHit.Broadcast(Target, Damage);
}

void UKrerusCombatComponent::OnCombatTargetDeath()
{
	UE_LOG(LogTemp,Warning,TEXT("KRERUS: El objetivo ha muerto. Deteniendo autoataque."));

	StopAutoAttack();
}

void UKrerusCombatComponent::OnOwnerDeath()
{
	UE_LOG(LogTemp,Warning,TEXT("KRERUS: El jugador ha muerto. Deteniendo autoataque."));

	StopAutoAttack();
}

void UKrerusCombatComponent::PerformAutoAttack()
{

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("KRERUS DEBUG: AutoAttack=%s | Paused=%s | Target=%s"),
		bIsAutoAttacking ? TEXT("TRUE") : TEXT("FALSE"),
		bAutoAttackPaused ? TEXT("TRUE") : TEXT("FALSE"),
		IsValid(CombatTarget.Get()) ? *CombatTarget->GetName() : TEXT("NONE")
	);

	if (bAutoAttackPaused)
	{
		return;
	}

    AActor* Owner = GetOwner();
    AKrerusEnemy* Target = CombatTarget.Get();

    if (!IsValid(Owner) || !IsValid(Target))
    {
        StopAutoAttack();
        return;
    }

	AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(Owner);

	if (!IsValid(PlayerCharacter))
	{
		StopAutoAttack();
		return;
	}

	UKrerusAttributeComponent* PlayerAttributes = PlayerCharacter->GetAttributeComponent();

	if (!IsValid(PlayerAttributes) || !PlayerAttributes->IsAlive())
	{
		UE_LOG(LogTemp,Warning,TEXT("KRERUS: El jugador esta muerto. Autoataque detenido."));

		StopAutoAttack();
		return;
	}

	UKrerusAttributeComponent* TargetAttributes = Target->GetAttributeComponent();

	if (!IsValid(TargetAttributes) || !TargetAttributes->IsAlive())
	{
		UE_LOG(LogTemp,Warning,TEXT("KRERUS: Objetivo muerto o sin atributos. Autoataque detenido."));

		StopAutoAttack();
		return;
	}

    if (!IsTargetInRange(PlayerCharacter,Target))
    {
        UE_LOG(LogTemp,Warning,TEXT("KRERUS: Objetivo fuera de alcance"));

        return;
    }

	if (!IsTargetInFront(PlayerCharacter, Target))
	{
		UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: Objetivo fuera del arco frontal"));

		return;
	}

	if (PlayerCharacter->IsAutoAttackMontagePlaying())
	{
		UE_LOG(LogTemp,Verbose,TEXT("KRERUS COMBAT: Ataque omitido. El Montage anterior sigue activo."));

		return;
	}

	bAttackImpactConsumed = false;
	
	PlayerCharacter->PlayAutoAttackMontage(GetAttackInterval());
}

bool UKrerusCombatComponent::IsTargetInRange(const AKrerusCharacter* PlayerCharacter,const AKrerusEnemy* Target) const
{
	if (!IsValid(PlayerCharacter) || !IsValid(Target))
	{
		return false;
	}

	const float Distance = FVector::Dist2D(PlayerCharacter->GetActorLocation(),Target->GetActorLocation());

	return Distance <= AttackRange;
}

bool UKrerusCombatComponent::IsTargetInFront(const AKrerusCharacter* PlayerCharacter,const AKrerusEnemy* Target) const
{
	if (!IsValid(PlayerCharacter) || !IsValid(Target))
	{
		return false;
	}

	const FVector DirectionToTarget = (Target->GetActorLocation() - PlayerCharacter->GetActorLocation()).GetSafeNormal2D();

	const FVector ForwardDirection = PlayerCharacter->GetActorForwardVector().GetSafeNormal2D();

	const float FacingDot =	FVector::DotProduct(ForwardDirection, DirectionToTarget);

	return FacingDot >= 0.0f;
}

void UKrerusCombatComponent::CancelPendingAutoAttackImpact()
{
	bAttackImpactConsumed = true;

	UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: Impacto pendiente cancelado."));
}

void UKrerusCombatComponent::SetAutoAttackPaused(bool bPaused)
{
	// Evitar procesar dos veces el mismo estado.
	if (bAutoAttackPaused == bPaused)
	{
		return;
	}

	bAutoAttackPaused = bPaused;

	UE_LOG(LogTemp,Log,TEXT("KRERUS COMBAT: Autoataque %s"),bAutoAttackPaused ? TEXT("PAUSADO") : TEXT("REANUDADO"));

	if (!bAutoAttackPaused && bIsAutoAttacking)
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);

		PerformAutoAttack();

		if (bIsAutoAttacking)
		{
			GetWorld()->GetTimerManager().SetTimer(AutoAttackTimerHandle,this,&UKrerusCombatComponent::PerformAutoAttack,GetAttackInterval(),true);
		}
	}
}

bool UKrerusCombatComponent::IsAutoAttackPaused() const
{
	return bAutoAttackPaused;
}