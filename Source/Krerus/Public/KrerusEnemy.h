// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KrerusNPC.h"
#include "KrerusEnemy.generated.h"

UCLASS()
class KRERUS_API AKrerusEnemy : public AKrerusNPC
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AKrerusEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting")
	void SetSelectionRingVisible(bool bVisible);

};
