#include "Region/LordSimulatorRegionCamera.h"
#include "Region/LordSimulatorRegionSubsystem.h"
#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"

ALordSimulatorRegionCamera::ALordSimulatorRegionCamera()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorEnableCollision(false);
    GetCameraComponent()->bConstrainAspectRatio = false;
}

void ALordSimulatorRegionCamera::Initialize(ULordSimulatorRegionSubsystem* InOwner, APlayerController* Controller)
{
    Region = InOwner;
    InputController = Controller;
    EnableInput(Controller);
    InputComponent->BindKey(EKeys::M, IE_Pressed, this, &ThisClass::OpenSelection);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::OpenSelection);
    InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ThisClass::Faster);
    InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ThisClass::Slower);
}

void ALordSimulatorRegionCamera::OpenSelection() { if (Region.IsValid()) Region->OpenSelection(); }
void ALordSimulatorRegionCamera::Faster() { if (Region.IsValid() && Region->CanMoveCamera()) SpeedCentimetres = FMath::Min(100000.0, SpeedCentimetres * 1.25); }
void ALordSimulatorRegionCamera::Slower() { if (Region.IsValid() && Region->CanMoveCamera()) SpeedCentimetres = FMath::Max(1000.0, SpeedCentimetres / 1.25); }

void ALordSimulatorRegionCamera::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APlayerController* Controller = InputController.Get();
    if (!Controller || !Region.IsValid() || !Region->CanMoveCamera()) return;
    FRotator Rotation = GetActorRotation();
    if (Controller->IsInputKeyDown(EKeys::RightMouseButton))
    {
        float X = 0, Y = 0;
        // Read pixels before Unreal's configurable sensitivity/FOV/smoothing transforms.
        if (Controller->PlayerInput)
        {
            X = Controller->PlayerInput->GetRawKeyValue(EKeys::MouseX);
            Y = Controller->PlayerInput->GetRawKeyValue(EKeys::MouseY);
        }
        Rotation.Yaw += X * 0.15;
        Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Y * 0.15, -89.0, 89.0);
        Rotation.Roll = 0;
        SetActorRotation(Rotation);
    }
    const FRotator Heading(0, Rotation.Yaw, 0);
    const FVector Forward = Heading.Vector();
    const FVector Right = FRotationMatrix(Heading).GetUnitAxis(EAxis::Y);
    FVector Movement = FVector::ZeroVector;
    if (Controller->IsInputKeyDown(EKeys::W)) Movement += Forward;
    if (Controller->IsInputKeyDown(EKeys::S)) Movement -= Forward;
    if (Controller->IsInputKeyDown(EKeys::D)) Movement += Right;
    if (Controller->IsInputKeyDown(EKeys::A)) Movement -= Right;
    if (Controller->IsInputKeyDown(EKeys::E)) Movement.Z += 1;
    if (Controller->IsInputKeyDown(EKeys::Q)) Movement.Z -= 1;
    AddActorWorldOffset(Movement.GetClampedToMaxSize(1.0) * SpeedCentimetres * DeltaSeconds, false);
}
