#include "UI/LordSimulatorMainMenuWidget.h"
#include "UI/LordSimulatorMainMenuButton.h"
#include "UI/LordSimulatorPrimaryUILayout.h"
#include "UI/LordSimulatorUISubsystem.h"

#include "CommonGameViewportClient.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/UObjectIterator.h"

#include "Editor.h"

struct FLordSimulatorMainMenuTestAccess
{
	static UButton* NewGame(const ULordSimulatorMainMenuWidget* Menu) { return Menu->NewGameButton; }
	static UButton* LoadGame(const ULordSimulatorMainMenuWidget* Menu) { return Menu->LoadGameButton; }
	static UButton* Settings(const ULordSimulatorMainMenuWidget* Menu) { return Menu->SettingsButton; }
	static UButton* Quit(const ULordSimulatorMainMenuWidget* Menu) { return Menu->QuitButton; }
	static UTextBlock* NewGameLabel(const ULordSimulatorMainMenuWidget* Menu) { return Menu->NewGameLabel; }
	static UTextBlock* LoadGameLabel(const ULordSimulatorMainMenuWidget* Menu) { return Menu->LoadGameLabel; }
	static UTextBlock* SettingsLabel(const ULordSimulatorMainMenuWidget* Menu) { return Menu->SettingsLabel; }
	static UTextBlock* QuitLabel(const ULordSimulatorMainMenuWidget* Menu) { return Menu->QuitLabel; }
	static const FButtonStyle& Style(const ULordSimulatorMainMenuWidget* Menu) { return Menu->MainMenuButtonStyle; }
	static UWidget* DesiredFocus(const ULordSimulatorMainMenuWidget* Menu) { return Menu->NativeGetDesiredFocusTarget(); }
};

namespace LordSimulatorMainMenuTests
{
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

static FString ResourcePath(const FSlateBrush& Brush)
{
	const UObject* Resource = Brush.GetResourceObject();
	return Resource ? Resource->GetPathName() : FString();
}

static void CheckLabel(FAutomationTestBase& Test, UTextBlock* Label, const TCHAR* Key, const TCHAR* Expected)
{
	Test.TestNotNull(TEXT("Bound label"), Label);
	if (!Label) return;
	const FName TableId(TEXT("/Game/Localization/ST_LordSimulator.ST_LordSimulator"));
	const FText ExpectedText = FText::FromStringTable(TableId, FString(Key));
	Test.TestEqual(TEXT("String Table entry resolves expected native text"), ExpectedText.ToString(), FString(Expected));
	Test.TestEqual(TEXT("Visible label uses the String Table value"), Label->GetText().ToString(), ExpectedText.ToString());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordSimulatorMainMenuTest, "LordSimulator.UI.MainMenu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLordSimulatorMainMenuTest::RunTest(const FString&)
{
	using namespace LordSimulatorMainMenuTests;
	if (!GEditor || GEditor->PlayWorld)
	{
		AddError(TEXT("Run main-menu tests in an idle editor."));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	const double Deadline = FPlatformTime::Seconds() + 60.0;
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Deadline]()
	{
		ULordSimulatorUISubsystem* UI = GetUI();
		if (!UI || !UI->IsUIAvailable())
		{
			if (FPlatformTime::Seconds() < Deadline) return false;
			AddError(TEXT("PIE main UI did not become available."));
			return true;
		}

		const ULordSimulatorPrimaryUILayout* Root = UI->GetPrimaryLayout();
		TestNotNull(TEXT("Primary root"), Root);
		if (!Root) return true;

		TestEqual(TEXT("Exactly one startup menu screen"), Root->GetWidgetCount(ELordSimulatorUILayer::Menu), 1);
		auto* Menu = Cast<ULordSimulatorMainMenuWidget>(Root->GetTopWidget(ELordSimulatorUILayer::Menu));
		TestNotNull(TEXT("Startup screen is main menu"), Menu);
		if (!Menu) return true;

		TestTrue(TEXT("Canonical main-menu asset"), Menu->GetClass()->GetPathName().Equals(
			TEXT("/Game/UI/Screens/MainMenu/WBP_MainMenu.WBP_MainMenu_C")));
		TestTrue(TEXT("Main menu is compatible with canonical Menu layer"),
			ULordSimulatorPrimaryUILayout::IsCompatible(ELordSimulatorUILayer::Menu, Menu->GetClass()));

		for (int32 Attempt = 0; Attempt < 4; ++Attempt)
		{
			TestTrue(TEXT("Repeated startup/UI initialization succeeds"), UI->InitializeUI());
		}
		TestEqual(TEXT("Repeated initialization preserves exactly one main menu"),
			Root->GetWidgetCount(ELordSimulatorUILayer::Menu), 1);
		TestEqual(TEXT("Repeated initialization preserves the same main-menu instance"),
			Root->GetTopWidget(ELordSimulatorUILayer::Menu), static_cast<UUserWidget*>(Menu));

		for (UButton* Disabled : {FLordSimulatorMainMenuTestAccess::NewGame(Menu),
			FLordSimulatorMainMenuTestAccess::LoadGame(Menu), FLordSimulatorMainMenuTestAccess::Settings(Menu)})
		{
			TestNotNull(TEXT("Disabled button bound"), Disabled);
			if (!Disabled) continue;
			TestFalse(TEXT("Unavailable button disabled"), Disabled->GetIsEnabled());
			TestFalse(TEXT("Unavailable button non-focusable"), Disabled->GetIsFocusable());
		}

		UButton* Quit = FLordSimulatorMainMenuTestAccess::Quit(Menu);
		TestNotNull(TEXT("Quit button bound"), Quit);
		if (Quit)
		{
			TestTrue(TEXT("Quit enabled"), Quit->GetIsEnabled());
			TestTrue(TEXT("Quit focusable"), Quit->GetIsFocusable());
			TestEqual(TEXT("Main-menu desired focus resolves to Quit"),
				FLordSimulatorMainMenuTestAccess::DesiredFocus(Menu), static_cast<UWidget*>(Quit));
		}

		CheckLabel(*this, FLordSimulatorMainMenuTestAccess::NewGameLabel(Menu), TEXT("MainMenu.NewGame"), TEXT("NEW GAME"));
		CheckLabel(*this, FLordSimulatorMainMenuTestAccess::LoadGameLabel(Menu), TEXT("MainMenu.LoadGame"), TEXT("LOAD GAME"));
		CheckLabel(*this, FLordSimulatorMainMenuTestAccess::SettingsLabel(Menu), TEXT("MainMenu.Settings"), TEXT("SETTINGS"));
		CheckLabel(*this, FLordSimulatorMainMenuTestAccess::QuitLabel(Menu), TEXT("MainMenu.Quit"), TEXT("QUIT"));

		const FButtonStyle& Style = FLordSimulatorMainMenuTestAccess::Style(Menu);
		TestEqual(TEXT("Normal button texture"), ResourcePath(Style.Normal),
			FString(TEXT("/Game/UI/Textures/MainMenu/T_MainMenuButtonNormal.T_MainMenuButtonNormal")));
		TestEqual(TEXT("Hover/focus button texture"), ResourcePath(Style.Hovered),
			FString(TEXT("/Game/UI/Textures/MainMenu/T_MainMenuButtonHover.T_MainMenuButtonHover")));
		TestEqual(TEXT("Pressed button texture"), ResourcePath(Style.Pressed),
			FString(TEXT("/Game/UI/Textures/MainMenu/T_MainMenuButtonPressed.T_MainMenuButtonPressed")));
		TestEqual(TEXT("Disabled button texture"), ResourcePath(Style.Disabled),
			FString(TEXT("/Game/UI/Textures/MainMenu/T_MainMenuButtonDisabled.T_MainMenuButtonDisabled")));

		APlayerController* Controller = GEditor->PlayWorld->GetFirstPlayerController();
		ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
		TestNotNull(TEXT("PIE player controller for focus verification"), Controller);
		if (!Player || !Player->ViewportClient)
		{
			AddError(TEXT("PIE game viewport unavailable for focus verification."));
			return true;
		}

		// PIE deliberately leaves focus with editor tools. Give the game viewport user focus,
		// then activate the production menu class through the canonical stack so CommonUI's
		// real activation-focus path is exercised instead of only inspecting DesiredFocusTarget.
		FSlateApplication::Get().SetUserFocus(0, Player->ViewportClient->GetGameViewportWidget());
		UClass* MainMenuClass = Menu->GetClass();
		TestTrue(TEXT("Remove startup main menu for activation-focus verification"),
			UI->PopWidget(ELordSimulatorUILayer::Menu));
		auto* FocusMenu = Cast<ULordSimulatorMainMenuWidget>(UI->AddWidget(ELordSimulatorUILayer::Menu, MainMenuClass));
		TestNotNull(TEXT("Production main menu reactivated for runtime focus verification"), FocusMenu);

		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForUIFrames());
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		ULordSimulatorUISubsystem* UI = GetUI();
		if (!UI || !UI->IsUIAvailable())
		{
			AddError(TEXT("PIE main UI unavailable during focus verification."));
			return true;
		}

		const ULordSimulatorPrimaryUILayout* Root = UI->GetPrimaryLayout();
		auto* Menu = Root ? Cast<ULordSimulatorMainMenuWidget>(Root->GetTopWidget(ELordSimulatorUILayer::Menu)) : nullptr;
		UButton* Quit = Menu ? FLordSimulatorMainMenuTestAccess::Quit(Menu) : nullptr;
		APlayerController* Controller = GEditor && GEditor->PlayWorld ? GEditor->PlayWorld->GetFirstPlayerController() : nullptr;
		TestTrue(TEXT("Quit receives actual CommonUI user focus after main-menu activation"),
			Quit && Controller && Quit->HasUserFocus(Controller));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
