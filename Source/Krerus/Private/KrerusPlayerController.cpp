#include "KrerusPlayerController.h"
#include "KrerusEnemy.h"

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

	AKrerusEnemy* Enemy = Cast<AKrerusEnemy>(Hit.GetActor());
	
	if (Enemy) 
	{
		SetSelectedEnemy(Enemy);
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

void AKrerusPlayerController::SetSelectedEnemy(AKrerusEnemy* NewEnemy)
{
	if (SelectedEnemy.Get() == NewEnemy)
	{
		return;
	}

	if (SelectedEnemy.IsValid() && SelectedEnemy != NewEnemy)
	{
		SelectedEnemy->SetSelectionRingVisible(false);
	}

	SelectedEnemy = NewEnemy;

	if (SelectedEnemy.IsValid())
	{
		SelectedEnemy->SetSelectionRingVisible(true);
		UE_LOG(LogTemp, Warning,TEXT("KRERUS: Nuevo objetivo: %s"),*SelectedEnemy->GetName());
	}
}

void AKrerusPlayerController::ClearSelectedEnemy()
{
	if (SelectedEnemy.IsValid())
	{
		UE_LOG(LogTemp, Warning,TEXT("KRERUS: Objetivo cancelado"));
	}
	SetSelectedEnemy(nullptr);
}