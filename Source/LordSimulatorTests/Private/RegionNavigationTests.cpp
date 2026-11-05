#include "Region/LordSimulatorRegionSubsystem.h"
#include "Region/LordSimulatorRegionCamera.h"
#include "UI/LordSimulatorUISubsystem.h"
#include "UI/LordSimulatorPrimaryUILayout.h"
#include "Components/InputComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "KeyState.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"

struct FLordSimulatorRegionTestAccess
{
    static ALordSimulatorRegionCamera* Camera(ULordSimulatorRegionSubsystem* Region) { return Region->Camera; }
    static ULevelStreamingDynamic* Level(ULordSimulatorRegionSubsystem* Region) { return Region->CurrentLevel; }
    static void MakePackageUnavailable(ULordSimulatorRegionSubsystem* Region, bool bUnavailable)
    {
        Region->PackageAvailabilityForTesting = bUnavailable
            ? TFunction<bool(const FString&)>([](const FString&) { return false; }) : TFunction<bool(const FString&)>();
    }
    static void SimulateLevelWithoutLandscape(ULordSimulatorRegionSubsystem* Region)
    {
        Region->State.Select(FIntPoint(0, 0));
        Region->Destination = NewObject<ULevelStreamingDynamic>(Region->GetWorld());
        Region->DestinationLoaded();
    }
};

namespace
{
ULordSimulatorRegionSubsystem* RegionInPIE()
{
    APlayerController* Controller = GEditor && GEditor->PlayWorld ? GEditor->PlayWorld->GetFirstPlayerController() : nullptr;
    ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
    return Player ? Player->GetSubsystem<ULordSimulatorRegionSubsystem>() : nullptr;
}
struct FNavigationFixture
{
    FTransform CameraTransform;
    TWeakObjectPtr<ALordSimulatorRegionCamera> Camera;
    TWeakObjectPtr<ULevelStreamingDynamic> Level;
    double Deadline = FPlatformTime::Seconds() + 300;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordSimulatorRegionNavigationTest, "LordSimulator.Region.Navigation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLordSimulatorRegionNavigationTest::RunTest(const FString&)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run navigation tests in an idle editor.")); return false; }
    // Keep the editor's last imported landscape out of the test's persistent world.
    FAutomationEditorCommonUtils::CreateNewMap();
    TSharedRef<FNavigationFixture> Fixture = MakeShared<FNavigationFixture>();
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Fixture]()
    {
        ULordSimulatorRegionSubsystem* Region = RegionInPIE();
        if (!Region || !Region->OpenSelection())
        {
            if (FPlatformTime::Seconds() < Fixture->Deadline) return false;
            AddError(TEXT("Region selection UI did not become available."));
            return true;
        }
        TestTrue(TEXT("New Game opens overview"), Region->GetSelectionState().bOpen);
        Region->DismissSelection();
        TestFalse(TEXT("Initial Back dismisses overview"), Region->GetSelectionState().bOpen);
        Region->OpenSelection();
        Region->SelectTile(FIntPoint(9, 0));
        return true;
    }));
    ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Fixture]()
    {
        ULordSimulatorRegionSubsystem* Region = RegionInPIE();
        if (!Region) return true;
        if (Region->GetSelectionState().bLoading && FPlatformTime::Seconds() < Fixture->Deadline) return false;
        TestEqual(TEXT("Real landscape loaded for selected cell"), Region->GetSelectionState().Current, FIntPoint(9, 0));
        auto* Camera = FLordSimulatorRegionTestAccess::Camera(Region);
        TestNotNull(TEXT("Independent camera exists"), Camera);
        if (!Camera) return true;
        // Exercise the live camera against deterministic engine input state, without
        // advancing the world or relying on the duration of OS keyboard events.
        APlayerController* Controller = GEditor->PlayWorld->GetFirstPlayerController();
        UPlayerInput* Input = Controller->PlayerInput;
        const FTransform InitialTransform = Camera->GetActorTransform();
        const TArray<FKey> Keys = {EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::Q, EKeys::E,
            EKeys::RightMouseButton, EKeys::MouseX, EKeys::MouseY};
        TMap<FKey, FKeyState> SavedKeys;
        for (FKey Key : Keys)
        {
            const FKeyState* Previous = Input->GetKeyState(Key);
            SavedKeys.Add(Key, Previous ? *Previous : FKeyState());
            Input->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), Key, IE_Pressed, uint64(0)));
            FKeyState* KeyState = Input->GetKeyState(Key);
            *KeyState = FKeyState();
        }
        auto Move = [&](FKey Key, FVector Expected)
        {
            Camera->SetActorTransform(InitialTransform);
            Input->GetKeyState(Key)->bDown = true;
            Camera->Tick(0.25f);
            TestTrue(TEXT("Camera movement follows heading and elapsed time"),
                (Camera->GetActorLocation() - InitialTransform.GetLocation()).Equals(Expected, 0.1));
            Input->GetKeyState(Key)->bDown = false;
        };
        Move(EKeys::W, FVector(0, -2500, 0));
        Move(EKeys::S, FVector(0, 2500, 0));
        Move(EKeys::A, FVector(-2500, 0, 0));
        Move(EKeys::D, FVector(2500, 0, 0));
        Move(EKeys::Q, FVector(0, 0, -2500));
        Move(EKeys::E, FVector(0, 0, 2500));
        auto Wheel = [&](FKey Key)
        {
            for (const FInputKeyBinding& Binding : Camera->InputComponent->KeyBindings)
                if (Binding.Chord.Key == Key) Binding.KeyDelegate.Execute(Key);
        };
        Wheel(EKeys::MouseScrollUp);
        Move(EKeys::W, FVector(0, -3125, 0));
        Wheel(EKeys::MouseScrollDown);
        Move(EKeys::W, FVector(0, -2500, 0));
        for (int32 Index = 0; Index < 100; ++Index) Wheel(EKeys::MouseScrollUp);
        Move(EKeys::W, FVector(0, -25000, 0));
        for (int32 Index = 0; Index < 100; ++Index) Wheel(EKeys::MouseScrollDown);
        Move(EKeys::W, FVector(0, -250, 0));
        Camera->SetActorTransform(InitialTransform);
        Input->GetKeyState(EKeys::RightMouseButton)->bDown = true;
        Input->GetKeyState(EKeys::MouseX)->RawValue.X = 100;
        Input->GetKeyState(EKeys::MouseY)->RawValue.X = 100;
        Camera->Tick(0.25f);
        TestTrue(TEXT("Raw pixels rotate yaw at 0.15 degrees per pixel"), FMath::IsNearlyEqual(Camera->GetActorRotation().Yaw, -75.0, 0.01));
        TestTrue(TEXT("Raw pixels rotate pitch at 0.15 degrees per pixel"), FMath::IsNearlyEqual(Camera->GetActorRotation().Pitch, -30.0, 0.01));
        Input->GetKeyState(EKeys::MouseY)->RawValue.X = 10000;
        Camera->Tick(0.25f);
        TestTrue(TEXT("Pitch clamps at 89 degrees"), FMath::IsNearlyEqual(Camera->GetActorRotation().Pitch, 89.0, 0.01));
        for (FKey Key : Keys) *Input->GetKeyState(Key) = SavedKeys.FindChecked(Key);
        Camera->SetActorTransform(InitialTransform);
        Fixture->Camera = Camera;
        Fixture->CameraTransform = Camera->GetActorTransform();
        Fixture->Level = FLordSimulatorRegionTestAccess::Level(Region);
        bool bFoundM = false;
        for (const FInputKeyBinding& Binding : Camera->InputComponent->KeyBindings)
            if (Binding.Chord.Key == EKeys::M && Binding.KeyEvent == IE_Pressed)
            {
                bFoundM = true;
                Binding.KeyDelegate.Execute(EKeys::M);
            }
        TestTrue(TEXT("M has a production input binding"), bFoundM);
        TestTrue(TEXT("M opens the overview from a loaded plot"), Region->GetSelectionState().bOpen);
        Region->DismissSelection();
        TestTrue(TEXT("Overview dismissal preserves camera"), Fixture->CameraTransform.Equals(Camera->GetActorTransform()));
        TestEqual(TEXT("Dismissal preserves loaded level instance"), FLordSimulatorRegionTestAccess::Level(Region), Fixture->Level.Get());
        Region->OpenSelection();
        Region->SelectTile(FIntPoint(9, 0));
        TestFalse(TEXT("Reselecting current plot closes overview"), Region->GetSelectionState().bOpen);
        TestEqual(TEXT("Reselecting does not reload"), FLordSimulatorRegionTestAccess::Level(Region), Fixture->Level.Get());
        TestTrue(TEXT("Reselecting preserves camera"), Fixture->CameraTransform.Equals(Camera->GetActorTransform()));
        Region->OpenSelection();
        FLordSimulatorRegionTestAccess::MakePackageUnavailable(Region, true);
        Region->SelectTile(FIntPoint(9, 1));
        FLordSimulatorRegionTestAccess::MakePackageUnavailable(Region, false);
        TestTrue(TEXT("Unavailable landscape shows failure and keeps overview open"), Region->GetSelectionState().bOpen && Region->GetSelectionState().bLoadFailed);
        TestEqual(TEXT("Failed switch preserves actual level"), FLordSimulatorRegionTestAccess::Level(Region), Fixture->Level.Get());
        FLordSimulatorRegionTestAccess::SimulateLevelWithoutLandscape(Region);
        TestTrue(TEXT("Level without landscape is rejected"), Region->GetSelectionState().bLoadFailed);
        TestTrue(TEXT("Failure preserves camera transform"), Fixture->CameraTransform.Equals(Camera->GetActorTransform()));
        Region->SelectTile(FIntPoint(9, 1));
        return true;
    }));
    ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Fixture]()
    {
        ULordSimulatorRegionSubsystem* Region = RegionInPIE();
        if (!Region) return true;
        if (Region->GetSelectionState().bLoading && FPlatformTime::Seconds() < Fixture->Deadline) return false;
        TestEqual(TEXT("Plot-to-plot transition commits selected destination"), Region->GetSelectionState().Current, FIntPoint(9, 1));
        TestTrue(TEXT("Previous landscape unloaded"), !Fixture->Level.IsValid() || !Fixture->Level->IsLevelLoaded());
        TestEqual(TEXT("Switch uses the same independent camera actor"), FLordSimulatorRegionTestAccess::Camera(Region), Fixture->Camera.Get());
        TestFalse(TEXT("Successful switch removes overview"), Region->GetSelectionState().bOpen);
        return true;
    }));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
