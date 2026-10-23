// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KrerusInteractable.h"
#include "KrerusNPC.generated.h"

UCLASS()
class KRERUS_API AKrerusNPC : public ACharacter, public IKrerusInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AKrerusNPC();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Interacion", meta = (ClampMin = "0.0"))
	float InteractionDistance = 300.0f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting")
	void SetSelectionRingVisible(bool bVisible);

	float GetInteractionDistance() const { return InteractionDistance; }
};
