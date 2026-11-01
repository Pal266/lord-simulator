#include "UI/LordSimulatorUISubsystem.h"
#include "UI/LordSimulatorMainMenuWidget.h"
#include "UI/LordSimulatorPrimaryUILayout.h"
#include "Localization/LordSimulatorLocalization.h"
#include "Input/CommonUIActionRouterBase.h"
#include "CommonGameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void ULordSimulatorUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UCommonUIActionRouterBase>();
	ViewportHandle = UGameViewportClient::OnViewportCreated().AddUObject(this, &ThisClass::OnViewportReady);
	CleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::OnWorldCleanup);
	InitializeUI();
}

void ULordSimulatorUISubsystem::Deinitialize()
{
	UGameViewportClient::OnViewportCreated().Remove(ViewportHandle);
	FWorldDelegates::OnWorldCleanup.Remove(CleanupHandle);
	ReleaseRoot();
	Super::Deinitialize();
}

void ULordSimulatorUISubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	if (PrimaryLayout && PrimaryLayout->GetOwningPlayer() != NewPlayerController) ReleaseRoot();
	if (NewPlayerController) InitializeUI();
}

void ULordSimulatorUISubsystem::OnViewportReady() { InitializeUI(); }

void ULordSimulatorUISubsystem::OnWorldCleanup(UWorld* World, bool, bool)
{
	if (PrimaryLayout && PrimaryLayout->GetWorld() == World) ReleaseRoot();
}

bool ULordSimulatorUISubsystem::InitializeUI()
{
	return TryInitialize(FLordSimulatorLocalization::IsStartupComplete());
}

bool ULordSimulatorUISubsystem::TryInitialize(bool bLocalizationStartupComplete)
{
	// Gate precedes even class loading/CreateWidget, where feature initialization could run.
	if (!bLocalizationStartupComplete) return false;
	if (IsUIAvailable()) return EnsureInitialMainMenu();
	ReleaseRoot();
	ULocalPlayer* Player = GetLocalPlayer();
	APlayerController* Controller = Player ? Player->GetPlayerController(GetWorld()) : nullptr;
	if (!Controller || !Controller->GetWorld()->IsGameWorld()
		|| !Cast<UCommonGameViewportClient>(Player->ViewportClient)) return false;
	UClass* LayoutClass = LoadClass<ULordSimulatorPrimaryUILayout>(nullptr,
		TEXT("/Game/UI/Framework/WBP_LordSimulatorPrimaryUILayout.WBP_LordSimulatorPrimaryUILayout_C"));
	if (!LayoutClass || LayoutClass->HasAnyClassFlags(CLASS_Abstract)) return false;
	auto* Candidate = CreateWidget<ULordSimulatorPrimaryUILayout>(Controller, LayoutClass);
	if (!Candidate || !Candidate->HasValidLayers()) return false;
	if (!Candidate->AddToPlayerScreen()) return false;
	PrimaryLayout = Candidate;
	PrimaryLayout->ActivateWidget();
	return EnsureInitialMainMenu();
}

bool ULordSimulatorUISubsystem::EnsureInitialMainMenu()
{
	if (!IsUIAvailable()) return false;
	if (bInitialMainMenuPresented) return true;
	if (MainMenu && PrimaryLayout->ContainsWidget(ELordSimulatorUILayer::Menu, MainMenu))
	{
		bInitialMainMenuPresented = true;
		return true;
	}
	MainMenu = nullptr;

	UClass* MainMenuClass = LoadClass<ULordSimulatorMainMenuWidget>(nullptr,
		TEXT("/Game/UI/Screens/MainMenu/WBP_MainMenu.WBP_MainMenu_C"));
	if (!MainMenuClass || MainMenuClass->HasAnyClassFlags(CLASS_Abstract)) return false;

	MainMenu = Cast<ULordSimulatorMainMenuWidget>(PrimaryLayout->AddToLayer(ELordSimulatorUILayer::Menu, MainMenuClass));
	bInitialMainMenuPresented = MainMenu != nullptr;
	return bInitialMainMenuPresented;
}

bool ULordSimulatorUISubsystem::IsUIAvailable() const
{
	return IsValid(PrimaryLayout) && PrimaryLayout->IsInViewport();
}

const ULordSimulatorPrimaryUILayout* ULordSimulatorUISubsystem::GetPrimaryLayout() const
{
	return IsUIAvailable() ? PrimaryLayout.Get() : nullptr;
}

void ULordSimulatorUISubsystem::ReleaseRoot()
{
	MainMenu = nullptr;
	bInitialMainMenuPresented = false;
	if (PrimaryLayout)
	{
		PrimaryLayout->ClearLayers();
		PrimaryLayout->DeactivateWidget();
		PrimaryLayout->RemoveFromParent();
		PrimaryLayout = nullptr;
	}
}

UUserWidget* ULordSimulatorUISubsystem::AddWidget(ELordSimulatorUILayer Layer, TSubclassOf<UUserWidget> Class)
{
	return IsUIAvailable() ? PrimaryLayout->AddToLayer(Layer, Class) : nullptr;
}

bool ULordSimulatorUISubsystem::RemoveWidget(ELordSimulatorUILayer Layer, UUserWidget* Widget)
{
	if (!IsUIAvailable()) return false;
	const bool bRemoved = PrimaryLayout->RemoveFromLayer(Layer, Widget);
	if (bRemoved && Widget == MainMenu) MainMenu = nullptr;
	return bRemoved;
}

bool ULordSimulatorUISubsystem::PopWidget(ELordSimulatorUILayer Layer)
{
	if (!IsUIAvailable()) return false;
	return RemoveWidget(Layer, PrimaryLayout->GetTopWidget(Layer));
}
