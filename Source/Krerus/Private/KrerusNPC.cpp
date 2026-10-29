#include "KrerusNPC.h"
#include "KrerusAttributeComponent.h"
#include "AbilitySystemComponent.h"
#include "KrerusAttributeSet.h"
#include "KrerusAbilitySystemComponent.h"

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
