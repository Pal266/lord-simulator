#include "Region/LordSimulatorRegionSubsystem.h"
#include "Region/LordSimulatorRegionCamera.h"
#include "UI/LordSimulatorRegionSelectionWidget.h"
#include "UI/LordSimulatorUISubsystem.h"
#include "UI/LordSimulatorPrimaryUILayout.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Landscape.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

void ULordSimulatorRegionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<ULordSimulatorUISubsystem>();
}

void ULordSimulatorRegionSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(LoadTimeout);
    CloseScreen();
    if (Camera) Camera->Destroy();
    Camera = nullptr;
    for (ULevelStreamingDynamic* Level : {CurrentLevel.Get(), Destination.Get()})
    {
        if (!Level) continue;
        Level->OnLevelLoaded.RemoveAll(this);
        Level->OnLevelShown.RemoveAll(this);
        Level->OnLevelUnloaded.RemoveAll(this);
        Level->SetIsRequestingUnloadAndRemoval(true);
    }
    CurrentLevel = Destination = nullptr;
    Super::Deinitialize();
}

bool ULordSimulatorRegionSubsystem::OpenSelection()
{
    if (Screen) return true;
    ULordSimulatorUISubsystem* UI = GetLocalPlayer()->GetSubsystem<ULordSimulatorUISubsystem>();
    if (!UI || !UI->IsUIAvailable()) return false;
    UClass* Class = LoadClass<ULordSimulatorRegionSelectionWidget>(nullptr,
        TEXT("/Game/UI/Screens/RegionSelection/WBP_RegionSelection.WBP_RegionSelection_C"));
    if (!Class) return false;
    State.Open();
    Screen = Cast<ULordSimulatorRegionSelectionWidget>(UI->AddWidget(ELordSimulatorUILayer::Menu, Class));
    if (!Screen) { State.Dismiss(); return false; }
    UpdateScreen();
    return true;
}

void ULordSimulatorRegionSubsystem::DismissSelection()
{
    if (State.Dismiss()) CloseScreen();
}

void ULordSimulatorRegionSubsystem::CloseScreen()
{
    if (!Screen) return;
    if (ULordSimulatorUISubsystem* UI = GetLocalPlayer()->GetSubsystem<ULordSimulatorUISubsystem>())
        UI->RemoveWidget(ELordSimulatorUILayer::Menu, Screen);
    Screen = nullptr;
}

void ULordSimulatorRegionSubsystem::UpdateScreen()
{
    if (Screen) Screen->Present(State);
}

bool ULordSimulatorRegionSubsystem::CanMoveCamera() const
{
    const auto* UI = GetLocalPlayer()->GetSubsystem<ULordSimulatorUISubsystem>();
    const auto* Root = UI ? UI->GetPrimaryLayout() : nullptr;
    return CurrentLevel && Camera && !State.bOpen && !State.bLoading && Root
        && Root->GetWidgetCount(ELordSimulatorUILayer::Menu) == 0
        && Root->GetWidgetCount(ELordSimulatorUILayer::Modal) == 0;
}

void ULordSimulatorRegionSubsystem::SelectTile(FIntPoint Tile)
{
    const ELordSimulatorPlotRequest Request = State.Select(Tile);
    if (Request == ELordSimulatorPlotRequest::Close) { CloseScreen(); return; }
    if (Request != ELordSimulatorPlotRequest::Load) return;
    UpdateScreen();
    const FString Package = FLordSimulatorRegion::LevelPath(Tile);
    bool bAvailable = FPackageName::DoesPackageExist(Package);
#if WITH_DEV_AUTOMATION_TESTS
    if (PackageAvailabilityForTesting) bAvailable = PackageAvailabilityForTesting(Package);
#endif
    if (!bAvailable) { FailLoad(); return; }
    bool bStarted = false;
    Destination = ULevelStreamingDynamic::LoadLevelInstance(GetWorld(), Package, FVector::ZeroVector, FRotator::ZeroRotator, bStarted);
    if (!Destination || !bStarted) { FailLoad(); return; }
    Destination->SetShouldBeVisible(false);
    Destination->OnLevelLoaded.AddUniqueDynamic(this, &ThisClass::DestinationLoaded);
    // A failed asynchronous load must not leave the screen permanently locked.
    GetWorld()->GetTimerManager().SetTimer(LoadTimeout, this, &ThisClass::FailLoad, 120.0f, false);
}

bool ULordSimulatorRegionSubsystem::CheckDestination() const
{
    if (!Destination || !Destination->GetLoadedLevel()) return false;
    int32 Landscapes = 0;
    for (AActor* Actor : Destination->GetLoadedLevel()->Actors)
        if (const ALandscape* Landscape = Cast<ALandscape>(Actor))
        {
            if (!Landscape->ActorHasTag(FName(*FLordSimulatorRegion::TileId(State.Pending)))) return false;
            ++Landscapes;
        }
    return Landscapes == 1;
}

void ULordSimulatorRegionSubsystem::DestinationLoaded()
{
    if (!State.bLoading || !CheckDestination()) { FailLoad(); return; }
    Destination->OnLevelShown.AddUniqueDynamic(this, &ThisClass::DestinationShown);
    Destination->SetShouldBeVisible(true);
}

void ULordSimulatorRegionSubsystem::DestinationShown()
{
    if (!State.bLoading || !CheckDestination()) { FailLoad(); return; }
    ALandscape* Landscape = nullptr;
    for (AActor* Actor : Destination->GetLoadedLevel()->Actors)
        if (auto* Found = Cast<ALandscape>(Actor)) Landscape = Found;
    const TOptional<float> Height = Landscape->GetHeightAtLocation(FVector(500000, 500000, 0));
    if (!Height.IsSet()) { FailLoad(); return; }
    DestinationCameraLocation = FVector(500000, 500000, Height.GetValue() + 50000);
    APlayerController* Controller = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (!Controller) { FailLoad(); return; }
    if (!Camera)
    {
        Camera = GetWorld()->SpawnActor<ALordSimulatorRegionCamera>();
        if (!Camera) { FailLoad(); return; }
        Camera->Initialize(this, Controller);
    }
    if (CurrentLevel)
    {
        // Retain the old content until the destination is verified, then unload it before dismissal.
        CurrentLevel->OnLevelUnloaded.AddUniqueDynamic(this, &ThisClass::PreviousUnloaded);
        CurrentLevel->SetShouldBeVisible(false);
        CurrentLevel->SetShouldBeLoaded(false);
        // Keep the streaming entry registered until its unload notification has fired.
        // Unreal broadcasts OnLevelUnloaded by looking up registered streaming entries.
    }
    else PreviousUnloaded();
}

void ULordSimulatorRegionSubsystem::PreviousUnloaded()
{
    if (!State.bLoading || !Destination) return;
    APlayerController* Controller = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (!Controller) { FailLoad(); return; }
    if (CurrentLevel)
    {
        CurrentLevel->OnLevelUnloaded.RemoveAll(this);
        CurrentLevel->SetIsRequestingUnloadAndRemoval(true);
    }
    Camera->SetActorLocation(DestinationCameraLocation);
    Camera->SetActorRotation(FRotator(-45, -90, 0)); // X=east, Y=south: north is negative Y.
    Controller->SetViewTarget(Camera);
    CurrentLevel = Destination;
    Destination->OnLevelLoaded.RemoveAll(this);
    Destination->OnLevelShown.RemoveAll(this);
    Destination = nullptr;
    GetWorld()->GetTimerManager().ClearTimer(LoadTimeout);
    State.Complete(true);
    if (auto* UI = GetLocalPlayer()->GetSubsystem<ULordSimulatorUISubsystem>()) UI->DismissInitialMainMenuForPlot();
    CloseScreen();
}

void ULordSimulatorRegionSubsystem::FailLoad()
{
    if (!State.bLoading) return;
    GetWorld()->GetTimerManager().ClearTimer(LoadTimeout);
    if (Destination)
    {
        Destination->OnLevelLoaded.RemoveAll(this);
        Destination->OnLevelShown.RemoveAll(this);
        Destination->SetShouldBeVisible(false);
        Destination->SetShouldBeLoaded(false);
        Destination->SetIsRequestingUnloadAndRemoval(true);
        Destination = nullptr;
    }
    if (CurrentLevel)
    {
        CurrentLevel->OnLevelUnloaded.RemoveAll(this);
        if (!GetWorld()->GetStreamingLevels().Contains(CurrentLevel)) GetWorld()->AddStreamingLevel(CurrentLevel);
        CurrentLevel->SetIsRequestingUnloadAndRemoval(false);
        CurrentLevel->SetShouldBeLoaded(true);
        CurrentLevel->SetShouldBeVisible(true);
    }
    State.Complete(false);
    UpdateScreen();
}
