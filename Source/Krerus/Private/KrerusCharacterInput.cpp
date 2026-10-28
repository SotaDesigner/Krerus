#include "KrerusCharacter.h"
#include "KrerusCameraComponent.h"
#include "KrerusPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "KrerusNPC.h"
#include "KrerusEnemy.h"
#include "KrerusInteractable.h"
#include "AbilitySystemComponent.h"
#include "KrerusStrikeAbility.h"
#include "Abilities/GameplayAbility.h"

// Conecta las acciones de Enhanced Input con cámara, movimiento, interacción y los dos circuitos de combate.
void AKrerusCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhacedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
        {
            EnhacedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AKrerusCharacter::Move);
            EnhacedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &AKrerusCharacter::StopMovementInput);
        }
        if (LookAction)
        {
            EnhacedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AKrerusCharacter::Look);
        }
        if (JumpAction)
        {
            EnhacedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);

            EnhacedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }
        if (SprintAction)
        {
            EnhacedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AKrerusCharacter::StartSprint);

            EnhacedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AKrerusCharacter::StopSprint);
        }
        if (CameraControlAction)
        {
            EnhacedInput->BindAction(
                CameraControlAction, ETriggerEvent::Started, this, &AKrerusCharacter::StartCameraControl);

            EnhacedInput->BindAction(
                CameraControlAction, ETriggerEvent::Completed, this, &AKrerusCharacter::StopCameraControl);
        }
        if (CameraFreeAction)
        {
            EnhacedInput->BindAction(
                CameraFreeAction, ETriggerEvent::Started, this, &AKrerusCharacter::StartFreeCamera);

            EnhacedInput->BindAction(
                CameraFreeAction, ETriggerEvent::Completed, this, &AKrerusCharacter::StopFreeCamera);
        }
        if (CameraZoomAction)
        {
            EnhacedInput->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &AKrerusCharacter::ZoomCamera);
        }
        if (ClearTargetAction)
        {
            EnhacedInput->BindAction(ClearTargetAction, ETriggerEvent::Started, this, &AKrerusCharacter::ClearTarget);
        }
        if (InteractAction)
        {
            EnhacedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AKrerusCharacter::Interact);
        }
        if (AutoAttackAction)
        {
            EnhacedInput->BindAction(AutoAttackAction, ETriggerEvent::Started, this, &AKrerusCharacter::OnAutoAttack);
        }
        if (StrikeAction)
        {
            EnhacedInput->BindAction(StrikeAction, ETriggerEvent::Started, this, &AKrerusCharacter::ActivateStrike);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("KRERUS: El componente NO es EnhancedInputComponent"));
    }
}

UKrerusCombatComponent* AKrerusCharacter::GetCombatComponent() const
{
    return CombatComponent.Get();
}

UKrerusAttributeComponent* AKrerusCharacter::GetAttributeComponent() const
{
    return AttributeComponent.Get();
}

// Instala el contexto de entrada del jugador local y configura cámara y cursor al reiniciar el Pawn.
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

    UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

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

    if (bIsFreeCameraActive)
    {
        if (AKrerusPlayerController* PC = Cast<AKrerusPlayerController>(GetController()))
        {
            PC->RegisterLeftMouseMovement(LookAxisVector);
        }
    }

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
    UE_LOG(LogTemp, Warning, TEXT("KRERUS: Controller actual: %s"), *GetNameSafe(GetController()));

    if (AKrerusPlayerController* PC = Cast<AKrerusPlayerController>(GetController()))
    {
        PC->BeginLeftMouseClick();
    }

    bIsFreeCameraActive = true;
    UpdateMouseCapture();
}

void AKrerusCharacter::StopFreeCamera()
{
    bIsFreeCameraActive = false;
    UpdateMouseCapture();

    if (AKrerusPlayerController* PC = Cast<AKrerusPlayerController>(GetController()))
    {
        PC->EndLeftMouseClick();
    }
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

void AKrerusCharacter::ClearTarget()
{
    if (AKrerusPlayerController* PC = Cast<AKrerusPlayerController>(GetController()))
    {
        PC->ClearSelectedEnemy();
    }
}

// Usa el NPC seleccionado y su alcance horizontal antes de despachar la interfaz de interacción.
void AKrerusCharacter::Interact()
{
    AKrerusPlayerController* PC = Cast<AKrerusPlayerController>(GetController());

    if (!PC)
    {
        return;
    }

    AKrerusNPC* SelectedNPC = PC->GetSelectedNPC();

    if (!IsValid(SelectedNPC))
    {
        return;
    }

    // Calculamos la distancia horizontal entre jugador y NPC
    const float Distance = FVector::Dist2D(GetActorLocation(), SelectedNPC->GetActorLocation());

    // Comprobamos si estamos dentro del alcance
    if (Distance > SelectedNPC->GetInteractionDistance())
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS: NPC demasiado lejos. Distancia: %.0f / Maxima: %.0f"), Distance,
            SelectedNPC->GetInteractionDistance());
        return;
    }

    // Ejecutamos la interacción mediante la interfaz
    if (SelectedNPC->GetClass()->ImplementsInterface(UKrerusInteractable::StaticClass()))
    {
        IKrerusInteractable::Execute_Interact(SelectedNPC, this);
    }
}

void AKrerusCharacter::OnAutoAttack()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("KRERUS: TECLA T DETECTADA"));
    }

    AKrerusPlayerController* KrerusController = Cast<AKrerusPlayerController>(GetController());

    if (!IsValid(KrerusController))
    {
        return;
    }

    KrerusController->ToggleAutoAttack();
}

// Busca la instancia concedida de Strike en el ASC del PlayerState y solicita su activación por Handle.
void AKrerusCharacter::ActivateStrike()
{
    if (AKrerusPlayerController* PC = Cast<AKrerusPlayerController>(GetController()))
    {
        ServerActivateStrike(Cast<AKrerusEnemy>(PC->GetSelectedNPC()));
    }
}
