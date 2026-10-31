#include "UI/LordSimulatorActivatableWidget.h"

ULordSimulatorActivatableWidget::ULordSimulatorActivatableWidget()
{
	bAutoRestoreFocus = true;
	SetIsFocusable(true);
}

void ULordSimulatorActivatableWidget::ConfigureForLayer(ELordSimulatorUILayer Layer)
{
	ManagedLayer = Layer;
	bAutoActivate = false;
	bIsModal = Layer == ELordSimulatorUILayer::Menu || Layer == ELordSimulatorUILayer::Modal;
	bSupportsActivationFocus = Layer != ELordSimulatorUILayer::PassiveOverlay;
	SetIsFocusable(bSupportsActivationFocus);
}

TOptional<FUIInputConfig> ULordSimulatorActivatableWidget::GetDesiredInputConfig() const
{
	if (ManagedLayer == ELordSimulatorUILayer::Menu || ManagedLayer == ELordSimulatorUILayer::Modal)
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
	if (ManagedLayer == ELordSimulatorUILayer::PassiveOverlay) return {};
	return Super::GetDesiredInputConfig();
}

UWidget* ULordSimulatorActivatableWidget::NativeGetDesiredFocusTarget() const
{
	if (UWidget* Desired = Super::NativeGetDesiredFocusTarget()) return Desired;
	return IsFocusable() ? const_cast<ULordSimulatorActivatableWidget*>(this) : nullptr;
}
