#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KrerusPlayerController.generated.h"

class AKrerusNPC;

/**
 * Gestiona cursor, selección de NPC y distinción entre clic y arrastre de cámara.
 * La selección alimenta tanto el autoataque del Character como la habilidad Strike de GAS.
 */
UCLASS()
class KRERUS_API AKrerusPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    void UpdateMouseCapture(bool bIsCameraControlActive, bool bIsFreeCameraActive);

    // Compatibilidad con mapas que todavia usen el PlayerController anterior.
    static void ApplyMouseCapture(APlayerController* PC, bool bIsCameraControlActive, bool bIsFreeCameraActive);

    AKrerusNPC* GetSelectedNPC() const;

    void SelectEnemyAtScreenPosition(const FVector2D& ScreenPosition);
    void BeginLeftMouseClick();
    void EndLeftMouseClick();
    void RegisterLeftMouseMovement(const FVector2D& MouseDelta);
    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void SetSelectedNPC(AKrerusNPC* NewEnemy);
    void ClearSelectedEnemy();
    void ClearSelectionIfTarget(AKrerusNPC* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ToggleAutoAttack();

protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Targeting")
    TWeakObjectPtr<AKrerusNPC> SelectedNPC;

    FVector2D LeftClickScreenPosition = FVector2D::ZeroVector;
    bool bLeftMousePressed = false;
    bool bLeftMouseDragged = false;

    float LeftMouseDragDistance = 0.0f;

    float LeftMouseDragThreshold = 5.0f;
};
