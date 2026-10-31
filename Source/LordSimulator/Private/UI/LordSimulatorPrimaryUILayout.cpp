#include "UI/LordSimulatorPrimaryUILayout.h"
#include "UI/LordSimulatorUserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

ULordSimulatorPrimaryUILayout::ULordSimulatorPrimaryUILayout()
{
	bAutoActivate = true;
}

TOptional<FUIInputConfig> ULordSimulatorPrimaryUILayout::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture, false);
}

bool ULordSimulatorPrimaryUILayout::HasValidLayers() const
{
	const UOverlay* Root = Cast<UOverlay>(GetRootWidget());
	return Root && Root->GetChildrenCount() == 4 && Game && Menu && Modal && PassiveOverlay
		&& Root->GetChildAt(0) == Game && Root->GetChildAt(1) == Menu
		&& Root->GetChildAt(2) == Modal && Root->GetChildAt(3) == PassiveOverlay;
}

void ULordSimulatorPrimaryUILayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!HasValidLayers()) return;
	// No production animation is specified. Stack changes complete immediately.
	Menu->SetTransitionDuration(0);
	Modal->SetTransitionDuration(0);
	Menu->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::RefreshLayerInteraction);
	Modal->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::RefreshLayerInteraction);
	PassiveOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	PassiveOverlay->SetIsEnabled(false);
	RefreshLayerInteraction();
}

UCommonActivatableWidgetStack* ULordSimulatorPrimaryUILayout::GetStack(ELordSimulatorUILayer Layer) const
{
	return Layer == ELordSimulatorUILayer::Menu ? Menu.Get()
		: Layer == ELordSimulatorUILayer::Modal ? Modal.Get() : nullptr;
}

UOverlay* ULordSimulatorPrimaryUILayout::GetPanel(ELordSimulatorUILayer Layer) const
{
	return Layer == ELordSimulatorUILayer::Game ? Game.Get()
		: Layer == ELordSimulatorUILayer::PassiveOverlay ? PassiveOverlay.Get() : nullptr;
}

bool ULordSimulatorPrimaryUILayout::IsCompatible(ELordSimulatorUILayer Layer, const UClass* Class)
{
	if (!Class || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
		|| Class->IsChildOf(StaticClass())) return false;
	const bool bActivatable = Class->IsChildOf(ULordSimulatorActivatableWidget::StaticClass());
	switch (Layer)
	{
	case ELordSimulatorUILayer::Game:
	case ELordSimulatorUILayer::PassiveOverlay:
		return bActivatable || Class->IsChildOf(ULordSimulatorUserWidget::StaticClass());
	case ELordSimulatorUILayer::Menu:
	case ELordSimulatorUILayer::Modal:
		return bActivatable;
	default:
		return false;
	}
}

UUserWidget* ULordSimulatorPrimaryUILayout::AddToLayer(ELordSimulatorUILayer Layer, TSubclassOf<UUserWidget> Class)
{
	if (!HasValidLayers() || !IsCompatible(Layer, Class) || !GetOwningPlayer()) return nullptr;
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetOwningPlayer(), Class);
	if (!Widget) return nullptr;
	if (auto* Activatable = Cast<ULordSimulatorActivatableWidget>(Widget)) Activatable->ConfigureForLayer(Layer);
	if (UCommonActivatableWidgetStack* Stack = GetStack(Layer))
	{
		// Instance insertion intentionally avoids retaining removed feature widgets in a pool.
		Stack->AddWidgetInstance(*CastChecked<ULordSimulatorActivatableWidget>(Widget));
	}
	else if (UOverlay* Panel = GetPanel(Layer))
	{
		UOverlaySlot* LayerSlot = Panel->AddChildToOverlay(Widget);
		if (!LayerSlot) return nullptr;
		LayerSlot->SetHorizontalAlignment(HAlign_Fill);
		LayerSlot->SetVerticalAlignment(VAlign_Fill);
	}
	else return nullptr;
	RefreshLayerInteraction();
	return Widget;
}

bool ULordSimulatorPrimaryUILayout::ContainsWidget(ELordSimulatorUILayer Layer, const UUserWidget* Widget) const
{
	if (!Widget) return false;
	if (const auto* Stack = GetStack(Layer)) return Stack->GetWidgetList().Contains(Widget);
	if (const auto* Panel = GetPanel(Layer)) return Panel->GetAllChildren().Contains(Widget);
	return false;
}

int32 ULordSimulatorPrimaryUILayout::GetWidgetCount(ELordSimulatorUILayer Layer) const
{
	if (const auto* Stack = GetStack(Layer)) return Stack->GetNumWidgets();
	if (const auto* Panel = GetPanel(Layer)) return Panel->GetChildrenCount();
	return 0;
}

UUserWidget* ULordSimulatorPrimaryUILayout::GetTopWidget(ELordSimulatorUILayer Layer) const
{
	if (const auto* Stack = GetStack(Layer)) return Stack->GetActiveWidget();
	return nullptr;
}

bool ULordSimulatorPrimaryUILayout::RemoveFromLayer(ELordSimulatorUILayer Layer, UUserWidget* Widget)
{
	if (!ContainsWidget(Layer, Widget)) return false;
	if (auto* Stack = GetStack(Layer)) Stack->RemoveWidget(*CastChecked<UCommonActivatableWidget>(Widget));
	else if (auto* Panel = GetPanel(Layer)) Panel->RemoveChild(Widget);
	RefreshLayerInteraction();
	return !ContainsWidget(Layer, Widget);
}

void ULordSimulatorPrimaryUILayout::ClearLayers()
{
	if (Game) Game->ClearChildren();
	if (PassiveOverlay) PassiveOverlay->ClearChildren();
	if (Menu) Menu->ClearWidgets();
	if (Modal) Modal->ClearWidgets();
}

void ULordSimulatorPrimaryUILayout::RefreshLayerInteraction(UCommonActivatableWidget*)
{
	if (!HasValidLayers()) return;
	const bool bModal = Modal->GetActiveWidget() != nullptr;
	// CommonUI owns focus/input routing. This separately closes the pointer hit-test path
	// through lower UMG layers, including empty space around a modal's visual contents.
	Game->SetIsEnabled(!bModal);
	Menu->SetIsEnabled(!bModal);
	Game->SetVisibility(bModal ? ESlateVisibility::HitTestInvisible : ESlateVisibility::SelfHitTestInvisible);
	Menu->SetVisibility(Menu->GetActiveWidget()
		? (bModal ? ESlateVisibility::HitTestInvisible : ESlateVisibility::SelfHitTestInvisible)
		: ESlateVisibility::Collapsed);
	SetVisibility(bModal || Menu->GetActiveWidget() ? ESlateVisibility::Visible : ESlateVisibility::SelfHitTestInvisible);
}

FReply ULordSimulatorPrimaryUILayout::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (GetTopWidget(ELordSimulatorUILayer::Modal) || GetTopWidget(ELordSimulatorUILayer::Menu)) return FReply::Handled();
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}
