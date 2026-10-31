#pragma once

#include "Styling/SlateTypes.h"
#include "UI/LordSimulatorActivatableWidget.h"
#include "LordSimulatorMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
struct FLordSimulatorMainMenuTestAccess;

/** Production main-menu behavior. The Widget Blueprint owns layout and production asset references. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class LORDSIMULATOR_API ULordSimulatorMainMenuWidget : public ULordSimulatorActivatableWidget
{
	GENERATED_BODY()
public:
	ULordSimulatorMainMenuWidget();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

private:
	friend struct FLordSimulatorMainMenuTestAccess;

	void ConfigureButton(UButton* Button, bool bEnabled);
	void ConfigureLabel(UTextBlock* Label, const TCHAR* StringTableKey, const FLinearColor& Color);
	void UpdateQuitPresentation();

	UFUNCTION()
	void HandleQuitClicked();
	UFUNCTION()
	void HandleQuitHovered();
	UFUNCTION()
	void HandleQuitUnhovered();
	UFUNCTION()
	void HandleQuitPressed();
	UFUNCTION()
	void HandleQuitReleased();

	/** Set once on WBP_MainMenu so every button shares the same approved state artwork. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lord Simulator|Main Menu", meta = (AllowPrivateAccess = "true"))
	FButtonStyle MainMenuButtonStyle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NewGameButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LoadGameButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NewGameLabel;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LoadGameLabel;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SettingsLabel;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> QuitLabel;

	bool bQuitPressed = false;
	bool bQuitFocusPath = false;
};
