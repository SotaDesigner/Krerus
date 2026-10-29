// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrerusCombatStateComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRERUS_API UKrerusCombatStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFUNCTION(BlueprintPure, Category = "Combat|State")
	bool IsInCombat();
	// Sets default values for this component's properties
	UKrerusCombatStateComponent();

	bool AddCombatRelation(AActor* OtherActor);

	bool RemoveCombatRelation(AActor* OtherActor);
	
	void ClearCombatRelations();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_IsInCombat, VisibleInstanceOnly,BlueprintReadOnly, Category = "Combat|State")
	bool bIsInCombat = false;

	UFUNCTION()
	void OnRep_IsInCombat();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	TSet<TWeakObjectPtr<AActor>> ActiveCombatRelations;

	bool AddRelationLocal(AActor* OtherActor);

	void UpdateCombatState();

	bool RemoveRelationLocal(AActor* OtherActor);
		
};
