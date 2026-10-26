// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusCharacter.h"
#include "KrerusCameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KrerusAttributeComponent.h"

// Sets default values
AKrerusCharacter::AKrerusCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;

	// Mantenemos la rotación directa del ratón
	CameraBoom->bEnableCameraRotationLag = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CameraSystem = CreateDefaultSubobject<UKrerusCameraComponent>(TEXT("CameraSystem"));

	AttributeComponent = CreateDefaultSubobject<UKrerusAttributeComponent>(TEXT("AttributeComponent"));
}

// Called when the game starts or when spawned
void AKrerusCharacter::BeginPlay()
{
	Super::BeginPlay();

	CameraSystem->InitializeCamera(CameraBoom);
}

// Called every frame
void AKrerusCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateSprint(DeltaTime);
	CameraSystem->UpdateCamera(CameraBoom, DeltaTime, CameraZoomInterpSpeed);

	const float DesiredDistance = CameraBoom->TargetArmLength;
	const float ActualDistance = FVector::Distance(
		CameraBoom->GetComponentLocation(),
		FollowCamera->GetComponentLocation()
	);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			10,
			0.0f,
			FColor::Yellow,
			FString::Printf(
				TEXT("Camara - Deseada: %.0f | Real: %.0f"),
				DesiredDistance,
				ActualDistance
			)
		);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			11,
			0.0f,
			FColor::Cyan,
			FString::Printf(
				TEXT("SpringArm Collision: %s"),
				CameraBoom->IsCollisionFixApplied()
				? TEXT("SI")
				: TEXT("NO")
			)
		);
	}
}
