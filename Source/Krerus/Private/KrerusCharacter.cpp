// Fill out your copyright notice in the Description page of Project Settings.


#include "KrerusCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Controller.h"
#include "InputActionValue.h"

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

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 700.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

}

// Called when the game starts or when spawned
void AKrerusCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AKrerusCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AKrerusCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhacedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{

		if (MoveAction)
		{
			EnhacedInput->BindAction(MoveAction,ETriggerEvent::Triggered,this,&AKrerusCharacter::Move);
		}
		if (LookAction)
		{
			EnhacedInput->BindAction(LookAction,ETriggerEvent::Triggered,this,&AKrerusCharacter::Look);
		}
		if (JumpAction)
		{
			EnhacedInput->BindAction(JumpAction,ETriggerEvent::Started,this,&ACharacter::Jump);

			EnhacedInput->BindAction(JumpAction,ETriggerEvent::Completed,this,&ACharacter::StopJumping);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KRERUS: El componente NO es EnhancedInputComponent"));
	}
}

void AKrerusCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

		const FVector ForwardDirection =
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection =
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AKrerusCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AKrerusCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();

	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("KRERUS: LocalPlayer no disponible"));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("KRERUS: Enhanced Input Subsystem no disponible"));
		return;
	}

	if (DefaultMappingContext)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);

		UE_LOG(LogTemp, Warning, TEXT("KRERUS: Mapping Context ACTIVADO"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KRERUS: Mapping Context no asignado"));
	}
}