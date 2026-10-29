// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusCombatStateComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UKrerusCombatStateComponent::UKrerusCombatStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}


bool UKrerusCombatStateComponent::IsInCombat()
{
	return bIsInCombat;
}

void UKrerusCombatStateComponent::OnRep_IsInCombat()
{
	UE_LOG(LogTemp,Log,TEXT("KRERUS COMBAT STATE: %s | %s"),*GetOwner()->GetName(),bIsInCombat ? TEXT("EN COMBATE") : TEXT("FUERA DE COMBATE"));
}

void UKrerusCombatStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKrerusCombatStateComponent,bIsInCombat);
}

bool UKrerusCombatStateComponent::AddCombatRelation(AActor* OtherActor)
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return false;
	}

	if (!IsValid(OtherActor) || OtherActor == Owner)
	{
		return false;
	}

	UKrerusCombatStateComponent* OtherCombatState =	OtherActor->FindComponentByClass<UKrerusCombatStateComponent>();

	if (!IsValid(OtherCombatState))
	{
		return false;
	}

	const bool bAddedToOwner = AddRelationLocal(OtherActor);

	const bool bAddedToOther = OtherCombatState->AddRelationLocal(Owner);

	if (bAddedToOwner || bAddedToOther)
	{
		UpdateCombatState();
		OtherCombatState->UpdateCombatState();
	}

	return bAddedToOwner || bAddedToOther;
}

bool UKrerusCombatStateComponent::AddRelationLocal(AActor* OtherActor)
{
	if (!IsValid(OtherActor))
	{
		return false;
	}

	const int32 PreviousCount = ActiveCombatRelations.Num();

	ActiveCombatRelations.Add(OtherActor);

	return ActiveCombatRelations.Num() > PreviousCount;
}

void UKrerusCombatStateComponent::UpdateCombatState()
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	const bool bNewCombatState = !ActiveCombatRelations.IsEmpty();

	if (bIsInCombat == bNewCombatState)
	{
		return;
	}

	bIsInCombat = bNewCombatState;

	OnRep_IsInCombat();
}

bool UKrerusCombatStateComponent::RemoveRelationLocal(AActor* OtherActor)
{
	const int32 RemovedCount = ActiveCombatRelations.Remove(OtherActor);

	return RemovedCount > 0;
}

bool UKrerusCombatStateComponent::RemoveCombatRelation(AActor* OtherActor)
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return false;
	}

	if (!IsValid(OtherActor) || OtherActor == Owner)
	{
		return false;
	}

	UKrerusCombatStateComponent* OtherCombatState =	OtherActor->FindComponentByClass<UKrerusCombatStateComponent>();

	if (!IsValid(OtherCombatState))
	{
		return false;
	}

	const bool bRemovedFromOwner = RemoveRelationLocal(OtherActor);

	const bool bRemovedFromOther = OtherCombatState->RemoveRelationLocal(Owner);

	if (bRemovedFromOwner || bRemovedFromOther)
	{
		UpdateCombatState();
		OtherCombatState->UpdateCombatState();
	}

	return bRemovedFromOwner || bRemovedFromOther;
}

void UKrerusCombatStateComponent::ClearCombatRelations()
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	TArray<TWeakObjectPtr<AActor>> RelationsToRemove = ActiveCombatRelations.Array();

	for (const TWeakObjectPtr<AActor>& Relation : RelationsToRemove)
	{
		AActor* OtherActor = Relation.Get();

		if (!IsValid(OtherActor))
		{
			continue;
		}

		UKrerusCombatStateComponent* OtherCombatState = OtherActor->FindComponentByClass<UKrerusCombatStateComponent>();

		if (!IsValid(OtherCombatState))
		{
			continue;
		}

		OtherCombatState->RemoveRelationLocal(Owner);
		OtherCombatState->UpdateCombatState();
	}

	ActiveCombatRelations.Empty();

	UpdateCombatState();
}

void UKrerusCombatStateComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ClearCombatRelations();
	}

	Super::EndPlay(EndPlayReason);
}