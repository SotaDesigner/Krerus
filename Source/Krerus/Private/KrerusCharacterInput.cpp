#include "KrerusCharacter.h"
#include "KrerusCameraComponent.h"
#include "KrerusPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"

void AKrerusCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhacedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{

		if (MoveAction)
		{
			EnhacedInput->BindAction(MoveAction,ETriggerEvent::Triggered,this,&AKrerusCharacter::Move);
			EnhacedInput->BindAction(MoveAction,ETriggerEvent::Completed,this,&AKrerusCharacter::StopMovementInput);
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
		if (SprintAction)
		{
			EnhacedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AKrerusCharacter::StartSprint);

			EnhacedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AKrerusCharacter::StopSprint);
		}
		if (CameraControlAction)
		{
			EnhacedInput->BindAction(CameraControlAction,ETriggerEvent::Started,this,&AKrerusCharacter::StartCameraControl);

			EnhacedInput->BindAction(CameraControlAction,ETriggerEvent::Completed,this,&AKrerusCharacter::StopCameraControl);
		}
		if (CameraFreeAction)
		{
			EnhacedInput->BindAction(CameraFreeAction,ETriggerEvent::Started,this,&AKrerusCharacter::StartFreeCamera);

			EnhacedInput->BindAction(CameraFreeAction,ETriggerEvent::Completed,this,&AKrerusCharacter::StopFreeCamera);
		}
		if (CameraZoomAction)
		{
			EnhacedInput->BindAction(CameraZoomAction,ETriggerEvent::Triggered,this,&AKrerusCharacter::ZoomCamera);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KRERUS: El componente NO es EnhancedInputComponent"));
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

	CameraSystem->SetupCameraLimits(PC);

	AKrerusPlayerController::ApplyMouseCapture(PC, false, false);
}

void AKrerusCharacter::Look(const FInputActionValue& Value)
{
	if (!Controller || (!bIsCameraControlActive && !bIsFreeCameraActive))
	{
		return;
	}

	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AKrerusCharacter::StartCameraControl()
{
	bIsCameraControlActive = true;
	UpdateMouseCapture();
}

void AKrerusCharacter::StopCameraControl()
{
	bIsCameraControlActive = false;
	bWasMoving = false;
	UpdateMouseCapture();
}

void AKrerusCharacter::StartFreeCamera()
{
	bIsFreeCameraActive = true;
	UpdateMouseCapture();
}

void AKrerusCharacter::StopFreeCamera()
{
	bIsFreeCameraActive = false;
	UpdateMouseCapture();
}

void AKrerusCharacter::UpdateMouseCapture()
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (AKrerusPlayerController* KrerusPC = Cast<AKrerusPlayerController>(PC))
	{
		KrerusPC->UpdateMouseCapture(bIsCameraControlActive, bIsFreeCameraActive);
	}
	else
	{
		AKrerusPlayerController::ApplyMouseCapture(PC, bIsCameraControlActive, bIsFreeCameraActive);
	}
}

void AKrerusCharacter::ZoomCamera(const FInputActionValue& Value)
{
	const float ZoomInput = Value.Get<float>();

	CameraSystem->ZoomCamera(ZoomInput, CameraZoomStep, MinCameraDistance, MaxCameraDistance);
}
