#include "UITestWidgets.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"

void ULordSimulatorUITestWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	WidgetTree->RootWidget = WidgetTree->ConstructWidget<UButton>();
}

void ULordSimulatorUITestScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	WidgetTree->RootWidget = WidgetTree->ConstructWidget<UButton>();
}
