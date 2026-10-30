// Editor-only integration probe. Run in a fresh, two-player, single-process PIE session.
#if WITH_EDITOR

#include "KrerusCharacter.h"
#include "KrerusEnemy.h"
#include "KrerusCombatComponent.h"
#include "KrerusAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogKrerusValidation, Log, All);

namespace KrerusValidation
{
struct FWarriorProbe
{
    TWeakObjectPtr<AKrerusCharacter> ServerPawn, ClientPawn;
    TWeakObjectPtr<AKrerusEnemy> ServerEnemy, ClientEnemy;
    int32 Stage = 0;
    int32 Failures = 0;
    double Started = FPlatformTime::Seconds();
    double StageTime = 0;
    double FirstStrikeTime = 0;
    double SecondStrikeTime = 0;
    float HealthBefore = 0;
    float RageBefore = 0;
    float AutoDamage = 0;
    FString Report = TEXT("# Warrior multiplayer integration probe\n\nFresh single-process listen-server PIE, two players. RPC requests originate from the owning client. Fixture changes affect PIE worlds only; stop PIE after running.\n\n");

    UAbilitySystemComponent* Source(bool Client = false) const
    { return (Client ? ClientPawn : ServerPawn)->GetAbilitySystemComponent(); }
    UAbilitySystemComponent* Target(bool Client = false) const
    { return (Client ? ClientEnemy : ServerEnemy)->GetAbilitySystemComponent(); }
    float Read(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute) const
    { return ASC->GetNumericAttribute(Attribute); }
    float Rage(bool Client = false) const { return Read(Source(Client), UKrerusAttributeSet::GetRageAttribute()); }
    float Health(bool Client = false) const { return Read(Target(Client), UKrerusAttributeSet::GetHealthAttribute()); }
    float Armor(bool Client = false) const { return Read(Target(Client), UKrerusAttributeSet::GetArmorAttribute()); }
    bool Near(float A, float B) const { return FMath::IsNearlyEqual(A, B, 0.05f); }
    bool Synced() const
    { return Near(Rage(), Rage(true)) && Near(Health(), Health(true)) && Near(Armor(), Armor(true)); }
    void Check(bool Passed, const FString& Label)
    {
        Failures += Passed ? 0 : 1;
        const FString Line = FString::Printf(TEXT("- %s: %s\n"), Passed ? TEXT("PASS") : TEXT("FAIL"), *Label);
        Report += Line;
        UE_LOG(LogKrerusValidation, Display, TEXT("%s"), *Line.TrimEnd());
    }
    bool Finish(const FString& Reason = FString())
    {
        if (!Reason.IsEmpty()) Check(false, Reason);
        if (ClientPawn.IsValid()) ClientPawn->ServerStopAutoAttack();
        Report += FString::Printf(TEXT("\nResult: %s (%d failures).\n"), Failures ? TEXT("FAIL") : TEXT("PASS"), Failures);
        const FString Directory = FPaths::ProjectSavedDir() / TEXT("Validation");
        IFileManager::Get().MakeDirectory(*Directory, true);
        FFileHelper::SaveStringToFile(Report, *(Directory / TEXT("WarriorMultiplayer.md")));
        UE_LOG(LogKrerusValidation, Display, TEXT("COMPLETE: %s (%d failures). Report: %s/WarriorMultiplayer.md"), Failures ? TEXT("FAIL") : TEXT("PASS"), Failures, *Directory);
        return false;
    }
    bool FindActors()
    {
        UWorld* ServerWorld = nullptr;
        UWorld* ClientWorld = nullptr;
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            UWorld* World = Context.World();
            if (!World || World->WorldType != EWorldType::PIE) continue;
            if (World->GetNetMode() == NM_ListenServer) ServerWorld = World;
            if (World->GetNetMode() == NM_Client) ClientWorld = World;
        }
        if (!ServerWorld || !ClientWorld) return false;
        for (TActorIterator<AKrerusCharacter> It(ClientWorld); It; ++It)
            if (It->IsLocallyControlled() && It->GetPlayerState()) ClientPawn = *It;
        if (!ClientPawn.IsValid()) return false;
        for (TActorIterator<AKrerusCharacter> It(ServerWorld); It; ++It)
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == ClientPawn->GetPlayerState()->GetPlayerId()) ServerPawn = *It;
        for (TActorIterator<AKrerusEnemy> It(ServerWorld); It; ++It)
            if (!It->IsCorpse() && It->GetName().Contains(TEXT("EnemyRage"))) { ServerEnemy = *It; break; }
        if (!ServerEnemy.IsValid()) return false;
        for (TActorIterator<AKrerusEnemy> It(ClientWorld); It; ++It)
            if (It->GetFName() == ServerEnemy->GetFName()) ClientEnemy = *It;
        return ServerPawn.IsValid() && ClientEnemy.IsValid() && Source() && Source(true) && Target() && Target(true)
            && Source()->GetSet<UKrerusAttributeSet>() && Source(true)->GetSet<UKrerusAttributeSet>()
            && Target()->GetSet<UKrerusAttributeSet>() && Target(true)->GetSet<UKrerusAttributeSet>();
    }
    void Next(int32 Value, double Now) { Stage = Value; StageTime = Now; }
    void Strike() { ClientPawn->ServerActivateAbilitySlot(1, ClientEnemy.Get()); }
    bool Tick(float)
    {
        if (FPlatformTime::Seconds() - Started > 120) return Finish(TEXT("Wall-clock timeout: probe did not complete"));
        if (Stage == 0)
        {
            if (!FindActors()) return true;
            Check(ServerPawn->HasAuthority() && !ClientPawn->HasAuthority() && ClientPawn->IsLocallyControlled(), TEXT("Distinct authoritative server pawn and owning client pawn"));
            Check(ServerEnemy->HasAuthority() && !ClientEnemy->HasAuthority(), TEXT("Distinct replicated enemy instances"));
            // Seed a durable target and resources in the server fixture only. No damage or impact is injected.
            Target()->SetNumericAttributeBase(UKrerusAttributeSet::GetMaxHealthAttribute(), 10000.f);
            Target()->SetNumericAttributeBase(UKrerusAttributeSet::GetHealthAttribute(), 10000.f);
            Target()->SetNumericAttributeBase(UKrerusAttributeSet::GetArmorAttribute(), 50.f);
            Source()->SetNumericAttributeBase(UKrerusAttributeSet::GetRageAttribute(), 0.f);
            const FVector Position = ServerEnemy->GetActorLocation() + FVector(-130.f, 0.f, 0.f);
            ServerPawn->SetActorLocationAndRotation(Position, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
            ServerPawn->ForceNetUpdate(); ServerEnemy->ForceNetUpdate();
            AutoDamage = ServerPawn->GetCombatComponent()->GetAttackDamage() * 100.f / 150.f;
            Next(1, ServerPawn->GetWorld()->GetTimeSeconds());
            return true;
        }
        if (!ServerPawn.IsValid() || !ClientPawn.IsValid() || !ServerEnemy.IsValid() || !ClientEnemy.IsValid()) return Finish(TEXT("PIE ended or fixture actor disappeared"));
        const double Now = ServerPawn->GetWorld()->GetTimeSeconds();
        const double Elapsed = Now - StageTime;
        if (Elapsed > 30) return Finish(FString::Printf(TEXT("Stage %d timed out; server rage %.2f, client rage %.2f, server HP %.2f, client HP %.2f"), Stage, Rage(), Rage(true), Health(), Health(true)));
        switch (Stage)
        {
        case 1:
            if (Elapsed < 1 || !Synced()) break;
            HealthBefore = Health();
            ClientPawn->ServerStartAutoAttack(ClientEnemy.Get());
            Next(2, Now);
            break;
        case 2:
            if (Rage() < 15.f) break;
            ClientPawn->ServerStopAutoAttack();
            Next(3, Now);
            break;
        case 3:
            if (Elapsed < 1 || !Synced()) break;
            Check(Near(Rage(), 15.f), FString::Printf(TEXT("Three real montage impacts yield exactly +15 rage; server/client %.2f/%.2f"), Rage(), Rage(true)));
            Check(Near(HealthBefore - Health(), AutoDamage * 3.f), FString::Printf(TEXT("Autoattack damage applied once per impact; delta %.3f, expected %.3f"), HealthBefore - Health(), AutoDamage * 3.f));
            Check(Synced(), TEXT("Autoattack rage, enemy health and armor replicate to owner"));
            // Two strikes without waiting for further autoattacks: explicitly seed the fixture to 30 rage.
            Source()->SetNumericAttributeBase(UKrerusAttributeSet::GetRageAttribute(), 30.f);
            Next(4, Now);
            break;
        case 4:
            if (Elapsed < 0.5 || !Synced()) break;
            HealthBefore = Health(); Strike(); Next(5, Now);
            break;
        case 5:
            if (Near(Armor(), 50.f) || !Synced()) break;
            FirstStrikeTime = Now;
            Check(Near(Rage(), 15.f), TEXT("First client strike consumes exactly 15 rage"));
            Check(Near(HealthBefore - Health(), 10.f), FString::Printf(TEXT("First strike deals 10 damage against armor 50; delta %.3f"), HealthBefore - Health()));
            Check(Near(Armor(), 42.5f), TEXT("Fracture reduces armor from 50 to 42.5"));
            Check(Synced(), TEXT("Strike cost, health and fractured armor replicate to owner"));
            Next(6, Now);
            break;
        case 6:
            if (Now - FirstStrikeTime < 5.f) break;
            HealthBefore = Health(); Strike(); Next(7, Now);
            break;
        case 7:
            if (Rage() > 0.05f || !Synced()) break;
            SecondStrikeTime = Now;
            Check(Near(Armor(), 42.5f), TEXT("Reapplication does not stack armor reduction"));
            Check(Near(HealthBefore - Health(), 15.f * 100.f / 142.5f), TEXT("Second strike uses current fractured armor for mitigation"));
            Check(Synced(), TEXT("Reapplication and second cost replicate to owner"));
            Next(8, Now);
            break;
        case 8:
            if (Now - FirstStrikeTime < 11.f) break;
            Check(Near(Armor(), 42.5f) && Near(Armor(true), 42.5f), TEXT("Fracture remains active past original 10-second expiry on server and client"));
            Next(9, Now);
            break;
        case 9:
            if (Now - SecondStrikeTime < 10.5f || !Synced()) break;
            Check(Near(Armor(), 50.f) && Near(Armor(true), 50.f), TEXT("Armor returns to 50 after refreshed duration on server and client"));
            HealthBefore = Health(); Strike(); Next(10, Now);
            break;
        case 10:
            if (Elapsed < 1 || !Synced()) break;
            Check(Near(Rage(), 0.f) && Near(Health(), HealthBefore) && Near(Armor(), 50.f), TEXT("Server rejects client strike with insufficient rage without damage/debuff"));
            Source()->SetNumericAttributeBase(UKrerusAttributeSet::GetRageAttribute(), 15.f);
            ServerPawn->SetActorLocation(ServerEnemy->GetActorLocation() + FVector(-2000.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
            ServerPawn->ForceNetUpdate(); Next(11, Now);
            break;
        case 11:
            if (Elapsed < 1 || !Synced()) break;
            RageBefore = Rage(); HealthBefore = Health(); Strike(); Next(12, Now);
            break;
        case 12:
            if (Elapsed < 1 || !Synced()) break;
            Check(Near(Rage(), RageBefore) && Near(Health(), HealthBefore) && Near(Armor(), 50.f), TEXT("Server rejects out-of-range client strike without resource cost"));
            return Finish();
        }
        return true;
    }
};

static TWeakPtr<FWarriorProbe> Running;
static FAutoConsoleCommand RunProbe(
    TEXT("Krerus.Test.WarriorMultiplayer"),
    TEXT("EDITOR ONLY. Mutates transient fixture in fresh two-player single-process listen-server PIE. Stop PIE after run. Writes Saved/Validation/WarriorMultiplayer.md."),
    FConsoleCommandDelegate::CreateLambda([]
    {
        if (Running.IsValid()) { UE_LOG(LogKrerusValidation, Warning, TEXT("Probe already running")); return; }
        TSharedRef<FWarriorProbe> Probe = MakeShared<FWarriorProbe>();
        Running = Probe;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Probe](float Delta) { return Probe->Tick(Delta); }));
        UE_LOG(LogKrerusValidation, Display, TEXT("Probe armed; waiting for two-player listen-server PIE (120-second timeout)."));
    }));
}
#endif
