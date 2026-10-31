#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/LordSimulatorUILayer.h"
#include "LordSimulatorUISubsystem.generated.h"

class ULordSimulatorPrimaryUILayout;
class UUserWidget;
struct FLordSimulatorUITestAccess;

/** Sole local-player owner/access point for production screen-space UI. */
UCLASS()
class LORDSIMULATOR_API ULordSimulatorUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	UFUNCTION(BlueprintCallable, Category = "Lord Simulator|UI")
	bool InitializeUI();
	UFUNCTION(BlueprintPure, Category = "Lord Simulator|UI")
	bool IsUIAvailable() const;
	UFUNCTION(BlueprintCallable, Category = "Lord Simulator|UI")
	UUserWidget* AddWidget(ELordSimulatorUILayer Layer, TSubclassOf<UUserWidget> WidgetClass);
	UFUNCTION(BlueprintCallable, Category = "Lord Simulator|UI")
	bool RemoveWidget(ELordSimulatorUILayer Layer, UUserWidget* Widget);
	UFUNCTION(BlueprintCallable, Category = "Lord Simulator|UI")
	bool PopWidget(ELordSimulatorUILayer Layer);

	const ULordSimulatorPrimaryUILayout* GetPrimaryLayout() const;
private:
	friend struct FLordSimulatorUITestAccess;
	bool TryInitialize(bool bLocalizationStartupComplete);
	void ReleaseRoot();
	void OnViewportReady();
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	UPROPERTY(Transient)
	TObjectPtr<ULordSimulatorPrimaryUILayout> PrimaryLayout;
	FDelegateHandle ViewportHandle;
	FDelegateHandle CleanupHandle;
};
