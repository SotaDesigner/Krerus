// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusEnemy.h"

// Sets default values
AKrerusEnemy::AKrerusEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AKrerusEnemy::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKrerusEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AKrerusEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
