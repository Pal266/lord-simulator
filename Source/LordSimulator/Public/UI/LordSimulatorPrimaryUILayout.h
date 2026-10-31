#pragma once

#include "UI/LordSimulatorActivatableWidget.h"
#include "LordSimulatorPrimaryUILayout.generated.h"

class UOverlay;
class UCommonActivatableWidgetStack;
struct FLordSimulatorUITestAccess;

/** Technical layout only. The canonical Widget Blueprint owns its four layer containers. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class LORDSIMULATOR_API ULordSimulatorPrimaryUILayout : public ULordSimulatorActivatableWidget
{
	GENERATED_BODY()
public:
	ULordSimulatorPrimaryUILayout();
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	bool HasValidLayers() const;
	int32 GetWidgetCount(ELordSimulatorUILayer Layer) const;
	UUserWidget* GetTopWidget(ELordSimulatorUILayer Layer) const;
	bool ContainsWidget(ELordSimulatorUILayer Layer, const UUserWidget* Widget) const;
	static bool IsCompatible(ELordSimulatorUILayer Layer, const UClass* WidgetClass);
protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
	friend class ULordSimulatorUISubsystem;
	friend struct FLordSimulatorUITestAccess;
	UUserWidget* AddToLayer(ELordSimulatorUILayer Layer, TSubclassOf<UUserWidget> WidgetClass);
	bool RemoveFromLayer(ELordSimulatorUILayer Layer, UUserWidget* Widget);
	void ClearLayers();
	void RefreshLayerInteraction(UCommonActivatableWidget* Unused = nullptr);
	UCommonActivatableWidgetStack* GetStack(ELordSimulatorUILayer Layer) const;
	UOverlay* GetPanel(ELordSimulatorUILayer Layer) const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Game;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> Menu;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> Modal;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> PassiveOverlay;
};
