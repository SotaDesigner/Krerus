#include "KrerusPlayerController.h"
#include "KrerusNPC.h"
#include "KrerusCharacter.h"
#include "KrerusEnemy.h"
#include "KrerusCombatComponent.h"

void AKrerusPlayerController::UpdateMouseCapture(bool bIsCameraControlActive, bool bIsFreeCameraActive)
{
	ApplyMouseCapture(this, bIsCameraControlActive, bIsFreeCameraActive);
}

void AKrerusPlayerController::ApplyMouseCapture(APlayerController* PC, bool bIsCameraControlActive, bool bIsFreeCameraActive)
{
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (bIsCameraControlActive || bIsFreeCameraActive)
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);

		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void AKrerusPlayerController::SelectEnemyAtScreenPosition(const FVector2D& ScreenPosition)
{
	FHitResult Hit;

	const bool bHit = GetHitResultAtScreenPosition(ScreenPosition,ECC_Visibility,true,Hit);

	if (!bHit)
	{
		return;
	}

	AKrerusNPC* NPC = Cast<AKrerusNPC>(Hit.GetActor());
	
	if (NPC) 
	{
		SetSelectedNPC(NPC);
	}
}

void AKrerusPlayerController::BeginLeftMouseClick()
{
	bLeftMousePressed = true;
	bLeftMouseDragged = false;
	LeftMouseDragDistance = 0.0f;

	float MouseX = 0.0f;
	float MouseY = 0.0f;

	if (GetMousePosition(MouseX, MouseY))
	{
		LeftClickScreenPosition = FVector2D(MouseX, MouseY);
	}

	UE_LOG(LogTemp, Warning, TEXT("KRERUS: Inicio clic izquierdo"));
}

void AKrerusPlayerController::EndLeftMouseClick()
{
	if (!bLeftMousePressed)
	{
		return;
	}

	bLeftMousePressed = false;

	if (!bLeftMouseDragged)
	{
		SelectEnemyAtScreenPosition(LeftClickScreenPosition);
	}

	UE_LOG(LogTemp, Warning,TEXT("KRERUS: Fin clic izquierdo"));
}

void AKrerusPlayerController::RegisterLeftMouseMovement(
	const FVector2D& MouseDelta)
{
	if (!bLeftMousePressed || bLeftMouseDragged)
	{
		return;
	}

	LeftMouseDragDistance += MouseDelta.Size();

	if (LeftMouseDragDistance >= LeftMouseDragThreshold)
	{
		bLeftMouseDragged = true;

		UE_LOG(LogTemp, Warning,
			TEXT("KRERUS: Arrastre de cámara detectado"));
	}
}

void AKrerusPlayerController::SetSelectedNPC(AKrerusNPC* NewEnemy)
{
	if (SelectedNPC.Get() == NewEnemy)
	{
		return;
	}

	AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetPawn());

	if (IsValid(PlayerCharacter))
	{
		UKrerusCombatComponent* Combat = PlayerCharacter->GetCombatComponent();

		if (IsValid(Combat))
		{
			Combat->SetCombatTarget(nullptr);
		}
	}

	if (SelectedNPC.IsValid() && SelectedNPC != NewEnemy)
	{
		SelectedNPC->SetSelectionRingVisible(false);
	}

	SelectedNPC = NewEnemy;

	if (SelectedNPC.IsValid())
	{
		SelectedNPC->SetSelectionRingVisible(true);
		UE_LOG(LogTemp, Warning,TEXT("KRERUS: Nuevo objetivo: %s"),*SelectedNPC->GetName());
	}
}

void AKrerusPlayerController::ClearSelectedEnemy()
{
	if (SelectedNPC.IsValid())
	{
		UE_LOG(LogTemp, Warning,TEXT("KRERUS: Objetivo cancelado"));
	}

	SetSelectedNPC(nullptr);

	AKrerusCharacter* PlayerCharacter = Cast<AKrerusCharacter>(GetPawn());

	if (IsValid(PlayerCharacter))
	{
		UKrerusCombatComponent* Combat = PlayerCharacter->GetCombatComponent();

		if (IsValid(Combat))
		{
			Combat->SetCombatTarget(nullptr);
		}
	}
}

void AKrerusPlayerController::ClearSelectionIfTarget(AKrerusNPC* Target)
{
	if (SelectedNPC.Get() != Target)
	{
		return;
	}

	ClearSelectedEnemy();
}

AKrerusNPC* AKrerusPlayerController::GetSelectedNPC() const
{
	return SelectedNPC.Get();
}

void AKrerusPlayerController::ToggleAutoAttack()
{
	AKrerusCharacter* PlayerCharacter =	Cast<AKrerusCharacter>(GetPawn());

	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	UKrerusCombatComponent* Combat = PlayerCharacter->GetCombatComponent();

	if (!IsValid(Combat))
	{
		return;
	}

	if (Combat->IsAutoAttacking())
	{
		Combat->StopAutoAttack();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,3.0f,FColor::Red,TEXT("KRERUS: Autoataque DESACTIVADO"));
		}
		return;
	}

	AKrerusEnemy* Enemy = Cast<AKrerusEnemy>(SelectedNPC.Get());

	if (!IsValid(Enemy))
	{
		return;
	}

	Combat->SetCombatTarget(Enemy);
	const bool bStarted = Combat->StartAutoAttack();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,5.0f,bStarted ? FColor::Green : FColor::Red,bStarted? TEXT("KRERUS: Autoataque INICIADO correctamente") : TEXT("KRERUS: ERROR al iniciar autoataque"));
	}
}