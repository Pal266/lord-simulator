#include "UI/LordSimulatorRegionSelectionWidget.h"
#include "UI/LordSimulatorRegionOverview.h"
#include "Region/LordSimulatorRegionSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

ULordSimulatorRegionSelectionWidget::ULordSimulatorRegionSelectionWidget() { SetIsFocusable(true); }

void ULordSimulatorRegionSelectionWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    const FName Table(TEXT("/Game/Localization/ST_LordSimulator.ST_LordSimulator"));
    RegionTitle->SetText(FText::FromStringTable(Table, TEXT("NewGame.RegionSelection.Title")));
    BackLabel->SetText(FText::FromStringTable(Table, TEXT("NewGame.RegionSelection.Back")));
    LoadFailedLabel->SetText(FText::FromStringTable(Table, TEXT("NewGame.RegionSelection.LoadFailed")));
    BackButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Back);
    Overview->OnCellSelected.AddUniqueDynamic(this, &ThisClass::SelectCell);
    FailurePanel->SetVisibility(ESlateVisibility::Collapsed);
}

void ULordSimulatorRegionSelectionWidget::Present(const FLordSimulatorRegionSelectionState& State)
{
    Overview->Present(State.Current, !State.bLoading);
    BackButton->SetIsEnabled(!State.bLoading);
    FailurePanel->SetVisibility(State.bLoadFailed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void ULordSimulatorRegionSelectionWidget::Back()
{
    if (ULocalPlayer* Player = GetOwningLocalPlayer()) Player->GetSubsystem<ULordSimulatorRegionSubsystem>()->DismissSelection();
}

void ULordSimulatorRegionSelectionWidget::SelectCell(int32 Column, int32 Row)
{
    if (ULocalPlayer* Player = GetOwningLocalPlayer()) Player->GetSubsystem<ULordSimulatorRegionSubsystem>()->SelectTile(FIntPoint(Column, Row));
}

FReply ULordSimulatorRegionSelectionWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape) { Back(); return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry, Event);
}

bool ULordSimulatorRegionSelectionWidget::NativeOnHandleBackAction() { Back(); return true; }
UWidget* ULordSimulatorRegionSelectionWidget::NativeGetDesiredFocusTarget() const { return BackButton; }
