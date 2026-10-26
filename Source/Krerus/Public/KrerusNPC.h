// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KrerusInteractable.h"
#include "KrerusNPC.generated.h"

class UKrerusAttributeComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	TObjectPtr<UKrerusAttributeComponent> AttributeComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting")
	void SetSelectionRingVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Attributes")
	UKrerusAttributeComponent* GetAttributeComponent() const { return AttributeComponent.Get(); }

	float GetInteractionDistance() const { return InteractionDistance; }
};
