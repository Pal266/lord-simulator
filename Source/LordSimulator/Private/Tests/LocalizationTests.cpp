#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Localization/LocalizationState.h"
#include "Localization/LordSimulatorLocalization.h"
#include "Settings/LordSimulatorSettings.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/TextLocalizationResource.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Misc/ConfigCacheIni.h"
#if WITH_EDITOR
#include "LocalizationSettings.h"
#include "LocalizationTargetTypes.h"
#endif

namespace LordSimulatorLocalizationTests
{
using namespace LordSimulatorLocalization;
struct FFixture
{
	FString Configured = TEXT("en");
	TArray<FString> Attempts;
	TArray<FString> Logs;
	TArray<FString> Cultures = {TEXT("en"), TEXT("pt-BR")};
	TSet<FString> Failures;
	bool bDiscovery = true;
	bool bResources = true;
	int32 DiscoveryCalls = 0;
	int32 Writes = 0;
	FState State;
	FFixture() : State({
		[this](TArray<FLordSimulatorLanguage>& Out)
		{
			++DiscoveryCalls;
			if (!bDiscovery || !FState::ValidateCultures(Cultures)) return false;
			for (const FString& Culture : Cultures) Out.Add({Culture, Culture});
			return true;
		},
		[this] { return bResources; },
		[this](const FString& Culture) { Attempts.Add(Culture); return !Failures.Contains(Culture); },
		[this] { return Configured; },
		[this](const FString& Culture) { Configured = Culture; ++Writes; },
		[this](const FString& Message) { Logs.Add(Message); }}) {}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordLocalizationStateTest, "LordSimulator.Localization.StateMachine", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLordLocalizationStateTest::RunTest(const FString&)
{
	using namespace LordSimulatorLocalizationTests;
	FFixture F;
	TestTrue(TEXT("Initialize"), F.State.Initialize());
	TestTrue(TEXT("Single attempt"), F.State.Initialize());
	TestEqual(TEXT("One discovery"), F.DiscoveryCalls, 1);
	TestEqual(TEXT("One lifecycle sequence"), F.Logs.Num(), 2);
	TestEqual(TEXT("Start log"), F.Logs[0], FString(TEXT("Localization initialization started.")));
	TestEqual(TEXT("Complete log"), F.Logs[1], FString(TEXT("Localization initialization completed.")));
	TestTrue(TEXT("Regional culture exact match"), F.State.IsSupported(TEXT("pt-BR")));
	for (const FString& Unsupported : {FString(TEXT("EN")), FString(TEXT("pt")), FString(TEXT("pt-br")), FString(TEXT("en-US"))})
		TestFalse(TEXT("No case/parent/child mapping"), F.State.IsSupported(Unsupported));
	TestFalse(TEXT("Initially unavailable"), F.State.GetActive().IsSet());
	F.State.ApplyStartup();
	TestEqual(TEXT("Startup active"), F.State.GetActive().GetValue(), FString(TEXT("en")));
	TestEqual(TEXT("Applied log"), F.Logs.Last(), FString(TEXT("Language 'en' was applied.")));
	F.State.CompleteStartup();
	TestTrue(TEXT("Startup completes"), F.State.IsStartupComplete());
	const int32 LogsBeforeNoOp = F.Logs.Num();
	TestTrue(TEXT("No-op"), F.State.Change(TEXT("en")));
	TestEqual(TEXT("No-op no engine call"), F.Attempts.Num(), 1);
	TestEqual(TEXT("No-op no log"), F.Logs.Num(), LogsBeforeNoOp);
	TestTrue(TEXT("Runtime supported change"), F.State.Change(TEXT("pt-BR")));
	TestEqual(TEXT("In-memory setting updated"), F.Configured, FString(TEXT("pt-BR")));
	TestEqual(TEXT("One settings mutation"), F.Writes, 1);
	TestEqual(TEXT("Active updated"), F.State.GetActive().GetValue(), F.Configured);
	TestFalse(TEXT("Unsupported rejection"), F.State.Change(TEXT("pt")));
	TestEqual(TEXT("Unsupported no engine call"), F.Attempts.Num(), 2);
	TestEqual(TEXT("Unsupported active log"), F.Logs.Last(), FString(TEXT("Language 'pt' is not supported. Language 'pt-BR' will remain active.")));
	F.Failures.Add(TEXT("en"));
	TestFalse(TEXT("Failed runtime change"), F.State.Change(TEXT("en")));
	TestEqual(TEXT("Configured retained"), F.Configured, FString(TEXT("pt-BR")));
	TestEqual(TEXT("Active retained"), F.State.GetActive().GetValue(), F.Configured);
	TestEqual(TEXT("Failure active log"), F.Logs.Last(), FString(TEXT("Language 'en' could not be applied. Language 'pt-BR' will remain active.")));
	F.State.Initialize();
	TestEqual(TEXT("Reinit retains state"), F.State.GetActive().GetValue(), F.Configured);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordLocalizationFailuresTest, "LordSimulator.Localization.FailuresAndRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLordLocalizationFailuresTest::RunTest(const FString&)
{
	using namespace LordSimulatorLocalizationTests;
	for (bool bNativeFails : {false, true})
	{
		FFixture F;
		F.Configured = TEXT("pt-BR");
		F.Failures.Add(TEXT("pt-BR"));
		if (bNativeFails) F.Failures.Add(TEXT("en"));
		F.State.Initialize();
		F.State.ApplyStartup();
		TestEqual(TEXT("Configured retained after fallback"), F.Configured, FString(TEXT("pt-BR")));
		TestEqual(TEXT("Two attempts"), F.Attempts.Num(), 2);
		TestEqual(TEXT("Native fallback last"), F.Attempts.Last(), FString(TEXT("en")));
		TestEqual(TEXT("Single startup result log"), F.Logs.Num(), 3);
		TestEqual(TEXT("Startup failure log"), F.Logs.Last(), bNativeFails
			? FString(TEXT("Language 'pt-BR' could not be applied and native language 'en' could not be activated. Localization is unavailable for the current session."))
			: FString(TEXT("Language 'pt-BR' could not be applied. Native language 'en' will be used for the current session.")));
		TestEqual(TEXT("Unavailable iff fallback fails"), F.State.GetActive().IsSet(), !bNativeFails);
		if (bNativeFails)
		{
			TestFalse(TEXT("Unsupported without active"), F.State.Change(TEXT("xx")));
			TestEqual(TEXT("Unsupported unavailable log"), F.Logs.Last(), FString(TEXT("Language 'xx' is not supported. No language is currently active.")));
			TestFalse(TEXT("Failed runtime without active"), F.State.Change(TEXT("en")));
			TestEqual(TEXT("Unavailable failure log"), F.Logs.Last(), FString(TEXT("Language 'en' could not be applied. No language is currently active.")));
		}
		F.Failures.Reset();
		TestTrue(TEXT("Recovery to configured but inactive culture applies"), F.State.Change(TEXT("pt-BR")));
		TestEqual(TEXT("Recovered active"), F.State.GetActive().GetValue(), FString(TEXT("pt-BR")));
		F.State.CompleteStartup();
		TestTrue(TEXT("Application failure does not block startup"), F.State.IsStartupComplete());
	}
	FFixture Native;
	Native.Failures.Add(TEXT("en"));
	Native.State.Initialize();
	Native.State.ApplyStartup();
	TestEqual(TEXT("Failed native not retried"), Native.Attempts.Num(), 1);
	TestFalse(TEXT("Native unavailable"), Native.State.GetActive().IsSet());
	TestEqual(TEXT("Native failure log"), Native.Logs.Last(), FString(TEXT("Language 'en' could not be applied and native language 'en' could not be activated. Localization is unavailable for the current session.")));
	for (bool bDiscoveryFails : {false, true})
	{
		FFixture F;
		F.bDiscovery = !bDiscoveryFails;
		F.bResources = false;
		TestFalse(TEXT("Failed init"), F.State.Initialize());
		TestFalse(TEXT("Failed init no retry"), F.State.Initialize());
		TestEqual(TEXT("Failed single attempt"), F.DiscoveryCalls, 1);
		TestEqual(TEXT("Failed sequence"), F.Logs.Num(), 2);
		TestEqual(TEXT("Init failure log"), F.Logs.Last(), FString(TEXT("Localization initialization failed. Native language 'en' will be used where available.")));
		TestTrue(TEXT("Native setting always valid"), F.State.IsValidSetting(TEXT("en")));
		TestEqual(TEXT("Discovered metadata retained after resource failure"), F.State.IsValidSetting(TEXT("pt-BR")), !bDiscoveryFails);
		F.State.ApplyStartup();
		TestEqual(TEXT("Failed init no startup application"), F.Attempts.Num(), 0);
		TestFalse(TEXT("Failed init unsupported request rejected"), F.State.Change(TEXT("xx")));
		TestEqual(TEXT("Failure precedence log"), F.Logs.Last(), FString(TEXT("Language 'xx' could not be applied. No language is currently active.")));
		TestEqual(TEXT("No engine calls after failure"), F.Attempts.Num(), 0);
		TestEqual(TEXT("No setting mutation after failure"), F.Writes, 0);
		F.State.CompleteStartup();
		TestTrue(TEXT("Failed initialization startup completes"), F.State.IsStartupComplete());
	}
	TestFalse(TEXT("Duplicate discovery rejected"), FState::ValidateCultures({TEXT("en"), TEXT("en")}));
	TestFalse(TEXT("Missing native rejected"), FState::ValidateCultures({TEXT("fr")}));
	TestFalse(TEXT("Target mismatch rejected"), FState::SameCultures({TEXT("en")}, {TEXT("en"), TEXT("fr")}));
	TestTrue(TEXT("Culture order immaterial"), FState::SameCultures({TEXT("en"), TEXT("fr")}, {TEXT("fr"), TEXT("en")}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordLocalizationProjectTest, "LordSimulator.Localization.ProjectResources", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLordLocalizationProjectTest::RunTest(const FString&)
{
	TArray<FString> Configured;
	GConfig->GetArray(TEXT("LordSimulator.Localization"), TEXT("SupportedCultures"), Configured, GGameIni);
	TestTrue(TEXT("Production contains only en"), Configured == TArray<FString>{TEXT("en")});
#if WITH_EDITOR
	const ULocalizationTargetSet* Targets = ULocalizationSettings::GetGameTargetSet();
	TestEqual(TEXT("One canonical target"), Targets->TargetObjects.Num(), 1);
	const ULocalizationTarget* Target = nullptr;
	for (const ULocalizationTarget* Candidate : Targets->TargetObjects)
		if (Candidate && Candidate->Settings.Name == TEXT("LordSimulator")) Target = Candidate;
	TestNotNull(TEXT("Canonical target in editor configuration"), Target);
	if (Target)
	{
		TArray<FString> TargetCultures;
		for (const auto& Culture : Target->Settings.SupportedCulturesStatistics) TargetCultures.Add(Culture.CultureName);
		TestTrue(TEXT("Target consistent with authoritative configuration"), LordSimulatorLocalization::FState::SameCultures(Configured, TargetCultures));
		TestEqual(TEXT("Native culture index"), Target->Settings.NativeCultureIndex, 0);
		if (TargetCultures.Num()) TestEqual(TEXT("Target native culture"), TargetCultures[0], FString(TEXT("en")));
	}
#endif
	TestTrue(TEXT("Framework resource initialization"), FLordSimulatorLocalization::Initialize());
	TestTrue(TEXT("Startup completion"), FLordSimulatorLocalization::IsStartupComplete());
	const auto& Languages = FLordSimulatorLocalization::GetSupportedLanguages();
	TestEqual(TEXT("One production language"), Languages.Num(), 1);
	if (Languages.Num()) TestEqual(TEXT("Unreal native name"), Languages[0].NativeDisplayName, FString(TEXT("English")));
	const UStringTable* Table = LoadObject<UStringTable>(nullptr, TEXT("/Game/Localization/ST_LordSimulator.ST_LordSimulator"));
	TestNotNull(TEXT("Asset table"), Table);
	if (Table)
	{
		TestEqual(TEXT("Asset assigned ID"), Table->GetStringTableId(), FName(TEXT("/Game/Localization/ST_LordSimulator.ST_LordSimulator")));
		int32 Entries = 0;
		Table->GetStringTable()->EnumerateSourceStrings([&](const FString&, const FString&) { ++Entries; return true; });
		TestEqual(TEXT("No placeholder entries"), Entries, 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordLocalizationTextTest, "LordSimulator.Localization.LiveFText", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLordLocalizationTextTest::RunTest(const FString&)
{
	// Isolated transient fixture, never part of production content or supported cultures.
	const FString Namespace = TEXT("LordSimulator.Automation.") + FGuid::NewGuid().ToString();
	const FName TableId(*Namespace);
	const FStringTableRef Fixture = FStringTable::NewStringTable();
	Fixture->SetNamespace(FTextKey(Namespace));
	Fixture->SetSourceString(FTextKey(TEXT("Fixture")), TEXT("Source"), FString());
	FStringTableRegistry::Get().RegisterStringTable(TableId, Fixture);
	const FText Text = FText::FromStringTable(TableId, TEXT("Fixture"));
	TestEqual(TEXT("Native source"), Text.ToString(), FString(TEXT("Source")));
	FTextLocalizationResource Translation;
	Translation.AddEntry(FTextKey(Namespace), FTextKey(TEXT("Fixture")), FString(TEXT("Source")), FString(TEXT("Translated fixture")), 0);
	FTextLocalizationManager::Get().UpdateFromLocalizationResource(Translation);
	TestEqual(TEXT("Retained FText resolves changed Unreal resource"), Text.ToString(), FString(TEXT("Translated fixture")));
	FTextLocalizationResource Native;
	Native.AddEntry(FTextKey(Namespace), FTextKey(TEXT("Fixture")), FString(TEXT("Source")), FString(TEXT("Source")), 0);
	FTextLocalizationManager::Get().UpdateFromLocalizationResource(Native);
	TestEqual(TEXT("Return to native source"), Text.ToString(), FString(TEXT("Source")));
	FStringTableRegistry::Get().UnregisterStringTable(TableId);
	return true;
}
#endif
