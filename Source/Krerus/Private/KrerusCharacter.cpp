// Fill out your copyright notice in the Description page of Project Settings.


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

	CombatComponent = CreateDefaultSubobject<UKrerusCombatComponent>(TEXT("CombatComponent"));
}

// Called when the game starts or when spawned
void AKrerusCharacter::BeginPlay()
{
	// Un Blueprint guardado tras Hot Reload puede conservar nula esta referencia.
	// Recuperamos el subobjeto existente antes del BeginPlay del Blueprint.
	if (!IsValid(CombatComponent))
	{
		CombatComponent = FindComponentByClass<UKrerusCombatComponent>();
		UE_LOG(LogTemp, Warning, TEXT("KRERUS COMBAT: Referencia de combate recuperada: %s"), *GetNameSafe(CombatComponent.Get()));
	}

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
		GEngine->AddOnScreenDebugMessage(10,0.0f,FColor::Yellow,FString::Printf(TEXT("Camara - Deseada: %.0f | Real: %.0f"),DesiredDistance,ActualDistance));
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(11,0.0f,FColor::Cyan,FString::Printf(TEXT("SpringArm Collision: %s"),	CameraBoom->IsCollisionFixApplied()?TEXT("SI") : TEXT("NO")));
	}
}

void AKrerusCharacter::OnAutoAttackImpact()
{
	if (!IsValid(CombatComponent))
	{
		return;
	}

	CombatComponent->ApplyAutoAttackImpact();
}

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

	const float PlayedDuration = AnimInstance->Montage_Play(AutoAttackMontage,PlayRate);

	if (PlayedDuration <= 0.0f)
	{
		return;
	}

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this,&AKrerusCharacter::OnAutoAttackMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,AutoAttackMontage);
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

void AKrerusCharacter::OnAutoAttackMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (Montage != AutoAttackMontage)
	{
		return;
	}

	if (bInterrupted && IsValid(CombatComponent))
	{
		CombatComponent->CancelPendingAutoAttackImpact();
	}

	UE_LOG(LogTemp,Warning,TEXT("KRERUS COMBAT: Montage de autoataque finalizado. Interrumpido: %s"),bInterrupted ? TEXT("SI") : TEXT("NO"));
}