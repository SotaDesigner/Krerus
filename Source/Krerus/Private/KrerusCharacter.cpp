#include "KrerusCharacter.h"
#include "KrerusCameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KrerusAttributeComponent.h"
#include "KrerusCombatComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "KrerusPlayerState.h"
#include "AbilitySystemComponent.h"

AKrerusCharacter::AKrerusCharacter()
{
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

    CombatComponent = CreateDefaultSubobject<UKrerusCombatComponent>(TEXT("CombatComponent"));
}

void AKrerusCharacter::BeginPlay()
{
    // Un Blueprint guardado tras Hot Reload puede conservar nula esta referencia.
    // Recuperamos el subobjeto existente antes del BeginPlay del Blueprint.
    if (!IsValid(CombatComponent))
    {
        CombatComponent = FindComponentByClass<UKrerusCombatComponent>();
        UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Referencia de combate recuperada: %s"),
            *GetNameSafe(CombatComponent.Get()));
    }

    Super::BeginPlay();

    CameraSystem->InitializeCamera(CameraBoom);
}

void AKrerusCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateSprint(DeltaTime);
    CameraSystem->UpdateCamera(CameraBoom, DeltaTime, CameraZoomInterpSpeed);

    const float DesiredDistance = CameraBoom->TargetArmLength;
    const float ActualDistance =
        FVector::Distance(CameraBoom->GetComponentLocation(), FollowCamera->GetComponentLocation());

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(10, 0.0f, FColor::Yellow,
            FString::Printf(TEXT("Camara - Deseada: %.0f | Real: %.0f"), DesiredDistance, ActualDistance));
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(11, 0.0f, FColor::Cyan,
            FString::Printf(
                TEXT("SpringArm Collision: %s"), CameraBoom->IsCollisionFixApplied() ? TEXT("SI") : TEXT("NO")));
    }
}

// Punto de entrada del aviso de impacto de la animación; el componente valida y consume el golpe.
void AKrerusCharacter::OnAutoAttackImpact()
{
    if (!IsValid(CombatComponent))
    {
        return;
    }

    CombatComponent->ApplyAutoAttackImpact();
}

// Ajusta la duración del montaje a la cadencia de combate y enlaza su finalización.
void AKrerusCharacter::PlayAutoAttackMontage(float AttackInterval)
{
    if (!IsValid(AutoAttackMontage))
    {
        return;
    }

    const float MontageDuration = AutoAttackMontage->GetPlayLength();

    const float SafeAttackInterval = FMath::Max(1.0f, AttackInterval);

    const float AnimationDuration = SafeAttackInterval * 0.9f;

    const float PlayRate = MontageDuration / AnimationDuration;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    if (!IsValid(AnimInstance))
    {
        return;
    }

    const float PlayedDuration = AnimInstance->Montage_Play(AutoAttackMontage, PlayRate);

    if (PlayedDuration <= 0.0f)
    {
        return;
    }

    FOnMontageEnded MontageEndedDelegate;

    MontageEndedDelegate.BindUObject(this, &AKrerusCharacter::OnAutoAttackMontageEnded);

    AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AutoAttackMontage);
}

bool AKrerusCharacter::IsAutoAttackMontagePlaying() const
{
    if (!IsValid(AutoAttackMontage))
    {
        return false;
    }

    const USkeletalMeshComponent* CharacterMesh = GetMesh();

    if (!IsValid(CharacterMesh))
    {
        return false;
    }

    UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();

    if (!IsValid(AnimInstance))
    {
        return false;
    }

    return AnimInstance->Montage_IsPlaying(AutoAttackMontage);
}

// Una interrupción invalida el impacto pendiente para impedir daño de un montaje cancelado.
void AKrerusCharacter::OnAutoAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != AutoAttackMontage)
    {
        return;
    }

    if (bInterrupted && IsValid(CombatComponent))
    {
        CombatComponent->CancelPendingAutoAttackImpact();
    }

    UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Montage de autoataque finalizado. Interrumpido: %s"),
        bInterrupted ? TEXT("SI") : TEXT("NO"));
}

UAbilitySystemComponent* AKrerusCharacter::GetAbilitySystemComponent() const
{
    const AKrerusPlayerState* KrerusPS = GetPlayerState<AKrerusPlayerState>();

    return IsValid(KrerusPS) ? KrerusPS->GetAbilitySystemComponent() : nullptr;
}

void AKrerusCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    InitializeAbilitySystem();
}

void AKrerusCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    InitializeAbilitySystem();
}

// Compartido por PossessedBy y OnRep_PlayerState: vincula Owner (PlayerState) y Avatar (este Character).
void AKrerusCharacter::InitializeAbilitySystem()
{
    AKrerusPlayerState* KrerusPS = GetPlayerState<AKrerusPlayerState>();

    if (!IsValid(KrerusPS))
    {
        UE_LOG(LogTemp, Warning, TEXT("KRERUS GAS: PlayerState no disponible"));
        return;
    }

    UAbilitySystemComponent* ASC = KrerusPS->GetAbilitySystemComponent();

    if (!IsValid(ASC))
    {
        UE_LOG(LogTemp, Error, TEXT("KRERUS GAS: ASC no disponible"));
        return;
    }

    ASC->InitAbilityActorInfo(KrerusPS, this);

    UE_LOG(LogTemp, Display, TEXT("KRERUS GAS: Inicializado | Owner: %s | Avatar: %s"),
        *GetNameSafe(ASC->GetOwnerActor()), *GetNameSafe(ASC->GetAvatarActor()));
}