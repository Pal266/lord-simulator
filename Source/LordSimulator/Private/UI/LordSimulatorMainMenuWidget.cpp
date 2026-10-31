#include "UI/LordSimulatorMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"

namespace LordSimulatorMainMenu
{
static const FName StringTableId(TEXT("/Game/Localization/ST_LordSimulator.ST_LordSimulator"));

static FLinearColor FromHex(const TCHAR* Hex)
{
	return FLinearColor(FColor::FromHex(Hex));
}

static const FLinearColor EnabledNormalColor = FromHex(TEXT("E7D9B8"));
static const FLinearColor EnabledHoverColor = FromHex(TEXT("FFF0C7"));
static const FLinearColor EnabledPressedColor = FromHex(TEXT("D8B56C"));
static const FLinearColor DisabledColor = FromHex(TEXT("777777"));
}

ULordSimulatorMainMenuWidget::ULordSimulatorMainMenuWidget()
{
	SetIsFocusable(true);
}

void ULordSimulatorMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ConfigureButton(NewGameButton, false);
	ConfigureButton(LoadGameButton, false);
	ConfigureButton(SettingsButton, false);
	ConfigureButton(QuitButton, true);

	ConfigureLabel(NewGameLabel, TEXT("MainMenu.NewGame"), LordSimulatorMainMenu::DisabledColor);
	ConfigureLabel(LoadGameLabel, TEXT("MainMenu.LoadGame"), LordSimulatorMainMenu::DisabledColor);
	ConfigureLabel(SettingsLabel, TEXT("MainMenu.Settings"), LordSimulatorMainMenu::DisabledColor);
	ConfigureLabel(QuitLabel, TEXT("MainMenu.Quit"), LordSimulatorMainMenu::EnabledNormalColor);

	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
		QuitButton->OnHovered.AddUniqueDynamic(this, &ThisClass::HandleQuitHovered);
		QuitButton->OnUnhovered.AddUniqueDynamic(this, &ThisClass::HandleQuitUnhovered);
		QuitButton->OnPressed.AddUniqueDynamic(this, &ThisClass::HandleQuitPressed);
		QuitButton->OnReleased.AddUniqueDynamic(this, &ThisClass::HandleQuitReleased);
	}

	UpdateQuitPresentation();
}

void ULordSimulatorMainMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	bQuitPressed = false;
	UpdateQuitPresentation();
}

UWidget* ULordSimulatorMainMenuWidget::NativeGetDesiredFocusTarget() const
{
	if (QuitButton && QuitButton->GetIsEnabled() && QuitButton->GetIsFocusable()) return QuitButton;
	return Super::NativeGetDesiredFocusTarget();
}

void ULordSimulatorMainMenuWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bQuitFocusPath = true;
	UpdateQuitPresentation();
}

void ULordSimulatorMainMenuWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bQuitFocusPath = false;
	UpdateQuitPresentation();
}

void ULordSimulatorMainMenuWidget::ConfigureButton(UButton* Button, bool bEnabled)
{
	if (!Button) return;
	Button->SetStyle(MainMenuButtonStyle);
	Button->SetIsEnabled(bEnabled);
}

void ULordSimulatorMainMenuWidget::ConfigureLabel(UTextBlock* Label, const TCHAR* StringTableKey, const FLinearColor& Color)
{
	if (!Label) return;

	Label->SetText(FText::FromStringTable(LordSimulatorMainMenu::StringTableId, FString(StringTableKey)));
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 32);
	Font.OutlineSettings.OutlineSize = 0;
	Label->SetFont(Font);
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(Color);
	Label->SetShadowOffset(FVector2D::ZeroVector);
	Label->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Label->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULordSimulatorMainMenuWidget::UpdateQuitPresentation()
{
	if (!QuitButton || !QuitLabel) return;

	FButtonStyle Style = MainMenuButtonStyle;
	const bool bHoverOrFocus = QuitButton->IsHovered() || bQuitFocusPath;
	if (bHoverOrFocus && !bQuitPressed)
	{
		Style.SetNormal(MainMenuButtonStyle.Hovered);
	}
	QuitButton->SetStyle(Style);

	QuitLabel->SetColorAndOpacity(bQuitPressed
		? LordSimulatorMainMenu::EnabledPressedColor
		: bHoverOrFocus ? LordSimulatorMainMenu::EnabledHoverColor : LordSimulatorMainMenu::EnabledNormalColor);
}

void ULordSimulatorMainMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void ULordSimulatorMainMenuWidget::HandleQuitHovered()
{
	UpdateQuitPresentation();
}

void ULordSimulatorMainMenuWidget::HandleQuitUnhovered()
{
	UpdateQuitPresentation();
}

void ULordSimulatorMainMenuWidget::HandleQuitPressed()
{
	bQuitPressed = true;
	UpdateQuitPresentation();
}

void ULordSimulatorMainMenuWidget::HandleQuitReleased()
{
	bQuitPressed = false;
	UpdateQuitPresentation();
}
