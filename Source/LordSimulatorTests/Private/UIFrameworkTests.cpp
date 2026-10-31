#include "UITestWidgets.h"
#include "UI/LordSimulatorUISubsystem.h"
#include "UI/LordSimulatorPrimaryUILayout.h"
#include "Localization/LordSimulatorLocalization.h"
#include "CommonGameViewportClient.h"
#include "CommonInputSettings.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Components/Overlay.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Editor.h"
#include "Tests/AutomationEditorCommon.h"
#include "Misc/AutomationTest.h"
#include "Layout/ArrangedChildren.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Widgets/SViewport.h"
#include "Input/HittestGrid.h"
#include "UObject/UObjectIterator.h"

struct FLordSimulatorUITestAccess
{
	static bool Initialize(ULordSimulatorUISubsystem* UI, bool bReady) { return UI->TryInitialize(bReady); }
	static void Release(ULordSimulatorUISubsystem* UI) { UI->ReleaseRoot(); }
	static UWidget* Layer(const ULordSimulatorPrimaryUILayout* Root, ELordSimulatorUILayer Layer)
	{
		if (auto* Panel = Root->GetPanel(Layer)) return Panel;
		return Root->GetStack(Layer);
	}
};

namespace LordSimulatorUITests
{
// Automation may consume several completed latent commands in one tick.
// CommonUI intentionally builds and routes its tree after Slate paints.
class FWaitForUIFrames : public IAutomationLatentCommand
{
	uint64 FirstFrame = MAX_uint64;
	virtual bool Update() override
	{
		if (FirstFrame == MAX_uint64) FirstFrame = GFrameCounter;
		return GFrameCounter >= FirstFrame + 3;
	}
};

static ULordSimulatorUISubsystem* GetUI()
{
	APlayerController* Controller = GEditor && GEditor->PlayWorld ? GEditor->PlayWorld->GetFirstPlayerController() : nullptr;
	ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	return Player ? Player->GetSubsystem<ULordSimulatorUISubsystem>() : nullptr;
}

static int32 CountRoots(ULocalPlayer* Player)
{
	int32 Count = 0;
	for (TObjectIterator<ULordSimulatorPrimaryUILayout> It; It; ++It)
		if (It->GetOwningLocalPlayer() == Player && It->IsInViewport()) ++Count;
	return Count;
}

// Query Slate's painted hit-test grid, including enabled filtering, rather than
// inferring interaction solely from UMG properties. Fixtures contain full-size buttons.
static FWidgetPath HitTestRoot(const ULordSimulatorPrimaryUILayout* Root)
{
	const auto Widget = Root->GetCachedWidget();
	const auto Window = FSlateApplication::Get().FindWidgetWindow(Widget.ToSharedRef());
	if (!Window) return {};
	const FGeometry& Geometry = Root->GetCachedGeometry();
	return FWidgetPath(Window->GetHittestGrid().GetBubblePath(
		Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f), 0.f, false, 0));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordSimulatorUIFrameworkTest, "LordSimulator.UI.Framework",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLordSimulatorUIFrameworkTest::RunTest(const FString&)
{
	using namespace LordSimulatorUITests;
	if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run UI tests in an idle editor.")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	const double Deadline = FPlatformTime::Seconds() + 60;
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Deadline]()
	{
		auto* UI = GetUI();
		if (!UI)
		{
			if (FPlatformTime::Seconds() < Deadline) return false;
			AddError(TEXT("PIE local-player UI subsystem did not become available."));
			return true;
		}
		TestTrue(TEXT("Production lifecycle creates root"), UI->IsUIAvailable());
		if (!UI->IsUIAvailable()) return true;
		auto* Player = UI->GetLocalPlayer();
		TestNotNull(TEXT("CommonUI viewport"), Cast<UCommonGameViewportClient>(Player->ViewportClient));
		TestFalse(TEXT("Enhanced Input integration excluded"), GetDefault<UCommonInputSettings>()->GetEnableEnhancedInputSupport());
		const auto* Root = UI->GetPrimaryLayout();
		TestTrue(TEXT("Canonical root asset"), Root->GetClass()->GetPathName().Equals(TEXT("/Game/UI/Framework/WBP_LordSimulatorPrimaryUILayout.WBP_LordSimulatorPrimaryUILayout_C")));
		TestTrue(TEXT("Canonical four layers and order"), Root->HasValidLayers());
		for (int32 Attempt = 0; Attempt < 4; ++Attempt) TestTrue(TEXT("Repeated initialization succeeds"), UI->InitializeUI());
		TestEqual(TEXT("Same root retained"), UI->GetPrimaryLayout(), Root);
		TestEqual(TEXT("Exactly one root for player"), CountRoots(Player), 1);
		TestEqual(TEXT("Baseline allows game and UI"), Root->GetDesiredInputConfig()->GetInputMode(), ECommonInputMode::All);

		FLordSimulatorUITestAccess::Release(UI);
		TestFalse(TEXT("No root after release"), UI->IsUIAvailable());
		TestFalse(TEXT("Localization prerequisite rejects creation"), FLordSimulatorUITestAccess::Initialize(UI, false));
		TestEqual(TEXT("No root was attached before prerequisite"), CountRoots(Player), 0);
		TestNull(TEXT("Layer operation fails without root"), UI->AddWidget(ELordSimulatorUILayer::Game, ULordSimulatorUITestWidget::StaticClass()));
		TestFalse(TEXT("Pop without root fails"), UI->PopWidget(ELordSimulatorUILayer::Menu));
		TestTrue(TEXT("Actual completed localization permits initialization"), UI->InitializeUI());
		Root = UI->GetPrimaryLayout();
		if (!Root) return true;

		TestNull(TEXT("Null class rejected"), UI->AddWidget(ELordSimulatorUILayer::Game, nullptr));
		TestNull(TEXT("Ordinary widget rejected by Menu"), UI->AddWidget(ELordSimulatorUILayer::Menu, ULordSimulatorUITestWidget::StaticClass()));
		TestNull(TEXT("Ordinary widget rejected by Modal"), UI->AddWidget(ELordSimulatorUILayer::Modal, ULordSimulatorUITestWidget::StaticClass()));
		TestNull(TEXT("Unrelated user widget rejected"), UI->AddWidget(ELordSimulatorUILayer::Game, UUserWidget::StaticClass()));
		TestNull(TEXT("Invalid enum rejected"), UI->AddWidget(static_cast<ELordSimulatorUILayer>(255), ULordSimulatorUITestWidget::StaticClass()));
		TestNull(TEXT("Nested viewport root rejected"), UI->AddWidget(ELordSimulatorUILayer::Game, Root->GetClass()));
		TestFalse(TEXT("Empty pop fails"), UI->PopWidget(ELordSimulatorUILayer::Menu));
		TestFalse(TEXT("Null removal fails"), UI->RemoveWidget(ELordSimulatorUILayer::Game, nullptr));

		const bool bWasPaused = GEditor->PlayWorld->IsPaused();
		for (const auto Layer : {ELordSimulatorUILayer::Game, ELordSimulatorUILayer::PassiveOverlay})
		{
			auto* First = UI->AddWidget(Layer, ULordSimulatorUITestWidget::StaticClass());
			auto* Second = UI->AddWidget(Layer, ULordSimulatorUITestWidget::StaticClass());
			TestNotNull(TEXT("First ordinary widget"), First);
			TestNotNull(TEXT("Second ordinary widget"), Second);
			TestTrue(TEXT("Distinct widgets coexist"), First && Second && First != Second);
			TestEqual(TEXT("Multiple widgets in layer"), Root->GetWidgetCount(Layer), 2);
			TestTrue(TEXT("Remove first"), UI->RemoveWidget(Layer, First));
			TestFalse(TEXT("Removed reference absent"), Root->ContainsWidget(Layer, First));
			TestFalse(TEXT("Duplicate removal fails"), UI->RemoveWidget(Layer, First));
			TestTrue(TEXT("Unrelated widget retained"), Root->ContainsWidget(Layer, Second));
			TestTrue(TEXT("Remove second"), UI->RemoveWidget(Layer, Second));
		}
		UWidget* Passive = FLordSimulatorUITestAccess::Layer(Root, ELordSimulatorUILayer::PassiveOverlay);
		TestEqual(TEXT("Passive self and descendants are hit-test transparent"), Passive->GetVisibility(), ESlateVisibility::HitTestInvisible);
		TestTrue(TEXT("Passive retains enabled presentation"), Passive->GetIsEnabled());

		for (const auto Layer : {ELordSimulatorUILayer::Menu, ELordSimulatorUILayer::Modal})
		{
			auto* First = Cast<ULordSimulatorUITestScreen>(UI->AddWidget(Layer, ULordSimulatorUITestScreen::StaticClass()));
			auto* Second = Cast<ULordSimulatorUITestScreen>(UI->AddWidget(Layer, ULordSimulatorUITestScreen::StaticClass()));
			TestNotNull(TEXT("First screen created"), First);
			TestNotNull(TEXT("Second screen created"), Second);
			if (!First || !Second) continue;
			TestFalse(TEXT("Lower screen deactivated"), First->IsActivated());
			TestTrue(TEXT("Top screen active"), Second->IsActivated());
			TestEqual(TEXT("Top screen displayed"), Root->GetTopWidget(Layer), static_cast<UUserWidget*>(Second));
			TestEqual(TEXT("Screen blocks gameplay through CommonUI"), Second->GetDesiredInputConfig()->GetInputMode(), ECommonInputMode::Menu);
			TestTrue(TEXT("Pop succeeds"), UI->PopWidget(Layer));
			TestFalse(TEXT("Popped reference absent"), Root->ContainsWidget(Layer, Second));
			TestTrue(TEXT("Underlying screen restored"), First->IsActivated());
			TestEqual(TEXT("Underlying instance preserved"), Root->GetTopWidget(Layer), static_cast<UUserWidget*>(First));
			First->DeactivateWidget();
			TestEqual(TEXT("Direct deactivation removes screen"), Root->GetWidgetCount(Layer), 0);
		}
		TestEqual(TEXT("Layer changes preserve pause state"), GEditor->PlayWorld->IsPaused(), bWasPaused);
		UI->AddWidget(ELordSimulatorUILayer::Game, ULordSimulatorUITestWidget::StaticClass());
		UI->AddWidget(ELordSimulatorUILayer::PassiveOverlay, ULordSimulatorUITestWidget::StaticClass());
		// CommonUI deliberately does not steal focus from editor tools during PIE.
		FSlateApplication::Get().SetUserFocus(0, Player->ViewportClient->GetGameViewportWidget());
		UI->AddWidget(ELordSimulatorUILayer::Menu, ULordSimulatorUITestScreen::StaticClass());
		return true;
	}));
	// Give CommonUI its normal input-tree/paint update before checking the routed mode.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForUIFrames());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		auto* UI = GetUI();
		if (!UI || !UI->IsUIAvailable()) return true;
		auto* Router = UI->GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>();
		TestEqual(TEXT("Menu is actual routed input mode"), Router->GetActiveInputMode(), ECommonInputMode::Menu);
		const auto* Root = UI->GetPrimaryLayout();
		const FWidgetPath Hit = HitTestRoot(Root);
		AddInfo(FString::Printf(TEXT("Menu pointer path: %s; root geometry: %s"), *Hit.ToString(), *Root->GetCachedGeometry().ToString()));
		TestTrue(TEXT("Pointer reaches menu through passive overlay"), Hit.ContainsWidget(Root->GetTopWidget(ELordSimulatorUILayer::Menu)->GetRootWidget()->GetCachedWidget().Get()));
		TestFalse(TEXT("Passive layer absent from pointer path"), Hit.ContainsWidget(FLordSimulatorUITestAccess::Layer(Root, ELordSimulatorUILayer::PassiveOverlay)->GetCachedWidget().Get()));
		UI->AddWidget(ELordSimulatorUILayer::Modal, ULordSimulatorUITestScreen::StaticClass());
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForUIFrames());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		auto* UI = GetUI();
		if (!UI || !UI->IsUIAvailable()) return true;
		const auto* Root = UI->GetPrimaryLayout();
		auto* Modal = Cast<UCommonActivatableWidget>(Root->GetTopWidget(ELordSimulatorUILayer::Modal));
		TestNotNull(TEXT("Modal displayed above menu"), Modal);
		TestTrue(TEXT("Modal is independent CommonUI routing root"), Modal && Modal->IsModal());
		const FWidgetPath Hit = HitTestRoot(Root);
		TestTrue(TEXT("Pointer reaches modal content"), Modal && Hit.ContainsWidget(Modal->GetRootWidget()->GetCachedWidget().Get()));
		for (const auto Layer : {ELordSimulatorUILayer::Game, ELordSimulatorUILayer::Menu, ELordSimulatorUILayer::PassiveOverlay})
		{
			UWidget* Container = FLordSimulatorUITestAccess::Layer(Root, Layer);
			TestFalse(TEXT("Blocked layer absent from pointer path"), Hit.ContainsWidget(Container->GetCachedWidget().Get()));
			TestTrue(TEXT("Blocked layer retains enabled presentation"), Container->GetIsEnabled());
		}
		TestTrue(TEXT("CommonUI gives keyboard focus to modal"), Modal && Modal->HasUserFocus(UI->GetLocalPlayer()->GetPlayerController(GEditor->PlayWorld)));
		TestEqual(TEXT("Modal actual input mode"), UI->GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>()->GetActiveInputMode(), ECommonInputMode::Menu);

		// Arrange the actual root hierarchy at the five required shapes and two DPI scales.
		// All four layers are visible here, so this checks the full technical layout/order.
		const TSharedRef<SWidget> Layout = Root->GetRootWidget()->TakeWidget();
		for (const FVector2D Size : {FVector2D(1280,720), FVector2D(1920,1080), FVector2D(2560,1440), FVector2D(3440,1440), FVector2D(3840,2160)})
		{
			for (const float Scale : {1.f, 1.5f})
			{
				Layout->SlatePrepass(Scale);
				FArrangedChildren Children(EVisibility::Visible);
				Layout->ArrangeChildren(FGeometry::MakeRoot(Size / Scale, FSlateLayoutTransform(Scale)), Children);
				TestEqual(TEXT("Four arranged layers"), Children.Num(), 4);
				for (int32 Index = 0; Index < Children.Num(); ++Index)
				{
					TestTrue(TEXT("Layer fills viewport at resolution/DPI"), Children[Index].Geometry.GetLocalSize().Equals(Size / Scale, 0.1f));
					UWidget* Expected = FLordSimulatorUITestAccess::Layer(Root, static_cast<ELordSimulatorUILayer>(Index));
					TestTrue(TEXT("Layer arrangement preserves visual priority"), Children[Index].Widget == Expected->GetCachedWidget());
				}
			}
		}
		TestTrue(TEXT("Remove modal"), UI->PopWidget(ELordSimulatorUILayer::Modal));
		TestEqual(TEXT("Game pointer eligibility restored"), FLordSimulatorUITestAccess::Layer(Root, ELordSimulatorUILayer::Game)->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		TestEqual(TEXT("Menu pointer eligibility restored"), FLordSimulatorUITestAccess::Layer(Root, ELordSimulatorUILayer::Menu)->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		TestTrue(TEXT("Remove menu"), UI->PopWidget(ELordSimulatorUILayer::Menu));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForUIFrames());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		auto* UI = GetUI();
		if (!UI || !UI->IsUIAvailable()) return true;
		TestEqual(TEXT("CommonUI restores combined baseline"), UI->GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>()->GetActiveInputMode(), ECommonInputMode::All);
		const auto* Root = UI->GetPrimaryLayout();
		auto* GameLayer = Cast<UOverlay>(FLordSimulatorUITestAccess::Layer(Root, ELordSimulatorUILayer::Game));
		auto* GameWidget = Cast<UUserWidget>(GameLayer->GetChildAt(0));
		TestTrue(TEXT("Pointer reaches game after closing screens through passive overlay"), HitTestRoot(Root).ContainsWidget(GameWidget->GetRootWidget()->GetCachedWidget().Get()));
		auto* Player = UI->GetLocalPlayer();
		FLordSimulatorUITestAccess::Release(UI);
		TestEqual(TEXT("Release detaches the root"), CountRoots(Player), 0);
		TestTrue(TEXT("Reinitialization after cleanup"), UI->InitializeUI());
		TestEqual(TEXT("Reinitialization creates only one root"), CountRoots(Player), 1);
		FString CreationError;
		UGameInstance* Instance = Player->GetGameInstance();
		ULocalPlayer* Second = Instance->CreateLocalPlayer(-1, CreationError, true);
		TestNotNull(TEXT("Second local player created"), Second);
		if (Second)
		{
			auto* SecondUI = Second->GetSubsystem<ULordSimulatorUISubsystem>();
			TestNotNull(TEXT("Separate subsystem for second player"), SecondUI);
			if (SecondUI)
			{
				TestTrue(TEXT("Second player UI available"), SecondUI->InitializeUI());
				TestTrue(TEXT("Second player repeat init"), SecondUI->InitializeUI());
				TestEqual(TEXT("One root for second player"), CountRoots(Second), 1);
				TestTrue(TEXT("Players have distinct roots"), UI->GetPrimaryLayout() != SecondUI->GetPrimaryLayout());
				TestEqual(TEXT("First root unaffected"), CountRoots(Player), 1);
			}
			Instance->RemoveLocalPlayer(Second);
			TestEqual(TEXT("Removing local player cleans up its root"), CountRoots(Second), 0);
		}
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
