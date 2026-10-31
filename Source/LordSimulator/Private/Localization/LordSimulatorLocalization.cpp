#include "Localization/LordSimulatorLocalization.h"
#include "Localization/LocalizationState.h"
#include "Settings/LordSimulatorSettings.h"
#include "Logging/LordSimulatorLog.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/TextLocalizationResource.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "LocalizationSettings.h"
#include "LocalizationTargetTypes.h"
#endif

namespace LordSimulatorLocalization
{
static const TCHAR* Section = TEXT("LordSimulator.Localization");
static TStrongObjectPtr<UStringTable> CanonicalTable;

static bool Discover(TArray<FLordSimulatorLanguage>& Out)
{
	TArray<FString> Cultures;
	if (!GConfig || !GConfig->GetArray(Section, TEXT("SupportedCultures"), Cultures, GGameIni)
		|| !FState::ValidateCultures(Cultures)) return false;
	TArray<FLordSimulatorLanguage> Found;
	for (const FString& Culture : Cultures)
	{
		const FCulturePtr Metadata = FInternationalization::Get().GetCulture(Culture);
		if (!Metadata.IsValid() || Metadata->GetNativeName().IsEmpty()) return false;
		Found.Add({Culture, Metadata->GetNativeName()});
	}
	Out = MoveTemp(Found);
	return true;
}

static bool ValidateResources()
{
	FString Native;
	TArray<FString> Configured;
	GConfig->GetArray(Section, TEXT("SupportedCultures"), Configured, GGameIni);
	if (!GConfig->GetString(Section, TEXT("NativeCulture"), Native, GGameIni) || Native != TEXT("en")) return false;
#if WITH_EDITOR
	const ULocalizationTargetSet* Targets = ULocalizationSettings::GetGameTargetSet();
	if (Targets->TargetObjects.Num() != 1) return false;
	const ULocalizationTarget* Target = Targets->TargetObjects[0];
	if (!Target || Target->Settings.Name != TEXT("LordSimulator")) return false;
	TArray<FString> TargetCultures;
	for (const auto& Culture : Target->Settings.SupportedCulturesStatistics) TargetCultures.Add(Culture.CultureName);
	if (!FState::SameCultures(Configured, TargetCultures)
		|| !TargetCultures.IsValidIndex(Target->Settings.NativeCultureIndex)
		|| TargetCultures[Target->Settings.NativeCultureIndex] != TEXT("en")) return false;
#endif
	const FString Directory = FPaths::ProjectContentDir() / TEXT("Localization/LordSimulator");
	FTextLocalizationMetaDataResource Meta;
	if (!Meta.LoadFromFile(Directory / TEXT("LordSimulator.locmeta")) || Meta.NativeCulture != TEXT("en")
		|| Meta.NativeLocRes != TEXT("en/LordSimulator.locres")
		|| !FState::SameCultures(Configured, Meta.CompiledCultures)) return false;
	for (const FString& Culture : Configured)
	{
		FTextLocalizationResource Resource;
		if (!Resource.LoadFromFile(Directory / Culture / TEXT("LordSimulator.locres"), 0)) return false;
	}
	CanonicalTable.Reset(LoadObject<UStringTable>(nullptr, TEXT("/Game/Localization/ST_LordSimulator.ST_LordSimulator")));
	return CanonicalTable.IsValid();
}

static bool Apply(const FString& Culture)
{
	if (!FInternationalization::Get().GetCulture(Culture).IsValid()) return false;
#if WITH_EDITOR
	if (GIsEditor)
	{
		// Unreal game-scoped text preview never changes the editor's language/locale.
		if (FTextLocalizationManager::Get().GetNativeCultureName(ELocalizedTextSourceCategory::Game).IsEmpty()) return false;
		FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);
		return true;
	}
#endif
	return FInternationalization::Get().SetCurrentLanguageAndLocale(Culture);
}

}

LordSimulatorLocalization::FState& FLordSimulatorLocalization::State()
{
	using namespace LordSimulatorLocalization;
	static FState Instance({Discover, ValidateResources, Apply,
		[] { return FLordSimulatorSettings::Get().Language; },
		[](const FString& Culture) { FLordSimulatorSettings::SetLanguageFromLocalization(Culture); },
		[](const FString& Message) { LS_LOG_DEFAULT(Message); }});
	return Instance;
}

bool FLordSimulatorLocalization::Initialize() { return State().Initialize(); }
const TArray<FLordSimulatorLanguage>& FLordSimulatorLocalization::GetSupportedLanguages() { return State().GetLanguages(); }
bool FLordSimulatorLocalization::IsSupported(const FString& Culture) { return State().IsSupported(Culture); }
bool FLordSimulatorLocalization::IsValidSetting(const FString& Culture) { return State().IsValidSetting(Culture); }
const FString& FLordSimulatorLocalization::GetConfiguredLanguage() { return FLordSimulatorSettings::Get().Language; }
const TOptional<FString>& FLordSimulatorLocalization::GetActiveLanguage() { return State().GetActive(); }
bool FLordSimulatorLocalization::ChangeLanguage(const FString& Culture) { return State().Change(Culture); }
void FLordSimulatorLocalization::ApplyStartupLanguage() { State().ApplyStartup(); }
void FLordSimulatorLocalization::CompleteStartup() { State().CompleteStartup(); }
bool FLordSimulatorLocalization::IsStartupComplete() { return State().IsStartupComplete(); }
