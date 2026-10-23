// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusNPC.h"

// Sets default values
AKrerusNPC::AKrerusNPC()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AKrerusNPC::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKrerusNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AKrerusNPC::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AKrerusNPC::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning,TEXT("KRERUS: Interaccion con NPC: %s"),*GetName());
}

