#include "KrerusNPC.h"
#include "KrerusAttributeComponent.h"
#include "AbilitySystemComponent.h"
#include "KrerusAttributeSet.h"
#include "KrerusAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AKrerusNPC::AKrerusNPC()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    AbilitySystemComponent = CreateDefaultSubobject<UKrerusAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);
    GASAttributeSet = CreateDefaultSubobject<UKrerusAttributeSet>(TEXT("GASAttributeSet"));
    AttributeComponent = CreateDefaultSubobject<UKrerusAttributeComponent>(TEXT("AttributeComponent"));
}

void AKrerusNPC::BeginPlay()
{
    Super::BeginPlay();
    AbilitySystemComponent->InitAbilityActorInfo(this,this);
    
    if (HasAuthority())
    {
        UKrerusAbilitySystemComponent* KrerusASC = Cast<UKrerusAbilitySystemComponent>(AbilitySystemComponent);
        if (!KrerusASC || !KrerusASC->InitializeAttributes(InitialAttributes))
        {
            UE_LOG(LogTemp, Error, TEXT("[Krerus GAS] Error inicializando atributos del NPC %s"), *GetName());
        }
    }

    UnaffectedWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

    const FGameplayTag SlowTag = FGameplayTag::RequestGameplayTag(FName(TEXT("State.Movement.Slowed")));
    const FGameplayTag RootTag = FGameplayTag::RequestGameplayTag(FName(TEXT("State.Movement.Rooted")));

    SlowTagDelegate = AbilitySystemComponent->RegisterGameplayTagEvent(SlowTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AKrerusNPC::OnMovementStatusChanged);
    RootTagDelegate = AbilitySystemComponent->RegisterGameplayTagEvent(RootTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AKrerusNPC::OnMovementStatusChanged);

    RefreshMovementStatus();
}

void AKrerusNPC::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AKrerusNPC::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AKrerusNPC::Interact_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Warning, TEXT("KRERUS: Interaccion con NPC: %s"), *GetName());
}

UAbilitySystemComponent* AKrerusNPC::GetAbilitySystemComponent() const { return AbilitySystemComponent; }

void AKrerusNPC::OnMovementStatusChanged(
    const FGameplayTag Tag,
    int32 NewCount)
{
    RefreshMovementStatus();
}

void AKrerusNPC::RefreshMovementStatus()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    UCharacterMovementComponent* Movement =
        GetCharacterMovement();

    if (!Movement)
    {
        return;
    }

    const FGameplayTag SlowTag =
        FGameplayTag::RequestGameplayTag(
            FName(TEXT("State.Movement.Slowed")));

    const FGameplayTag RootTag =
        FGameplayTag::RequestGameplayTag(
            FName(TEXT("State.Movement.Rooted")));

    if (AbilitySystemComponent->HasMatchingGameplayTag(RootTag))
    {
        Movement->MaxWalkSpeed = 0.f;
        Movement->StopMovementImmediately();
        return;
    }

    const bool bSlowed =
        AbilitySystemComponent->HasMatchingGameplayTag(SlowTag);

    Movement->MaxWalkSpeed =
        UnaffectedWalkSpeed * (bSlowed ? 0.6f : 1.f);
}

void AKrerusNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AbilitySystemComponent)
    {
        const FGameplayTag SlowTag =
            FGameplayTag::RequestGameplayTag(
                FName(TEXT("State.Movement.Slowed")));

        const FGameplayTag RootTag =
            FGameplayTag::RequestGameplayTag(
                FName(TEXT("State.Movement.Rooted")));

        AbilitySystemComponent->RegisterGameplayTagEvent(
            SlowTag, EGameplayTagEventType::NewOrRemoved)
            .Remove(SlowTagDelegate);

        AbilitySystemComponent->RegisterGameplayTagEvent(
            RootTag, EGameplayTagEventType::NewOrRemoved)
            .Remove(RootTagDelegate);
    }

    Super::EndPlay(EndPlayReason);
}