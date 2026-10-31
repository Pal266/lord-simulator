#pragma once

#include "Localization/LordSimulatorLocalization.h"

namespace LordSimulatorLocalization
{
/** Internal environment seam; the same state machine is used by production and automation. */
struct FEnvironment
{
	TFunction<bool(TArray<FLordSimulatorLanguage>&)> Discover;
	TFunction<bool()> ValidateResources;
	TFunction<bool(const FString&)> Apply;
	TFunction<FString()> ReadConfigured;
	TFunction<void(const FString&)> WriteConfigured;
	TFunction<void(const FString&)> Log;
};

class FState
{
public:
	explicit FState(FEnvironment InEnvironment) : Environment(MoveTemp(InEnvironment)) {}
	bool Initialize();
	bool IsSupported(const FString& Culture) const;
	bool IsValidSetting(const FString& Culture) const;
	void ApplyStartup();
	bool Change(const FString& Culture);
	void CompleteStartup() { bStartupComplete = true; }
	bool IsStartupComplete() const { return bStartupComplete; }
	const TArray<FLordSimulatorLanguage>& GetLanguages() const { return Languages; }
	const TOptional<FString>& GetActive() const { return Active; }
	static bool ValidateCultures(const TArray<FString>& Cultures);
	static bool SameCultures(const TArray<FString>& A, const TArray<FString>& B);
private:
	void LogRuntimeFailure(const FString& Culture, bool bUnsupported);
	FEnvironment Environment;
	TArray<FLordSimulatorLanguage> Languages;
	TOptional<FString> Active;
	bool bAttempted = false;
	bool bDiscovered = false;
	bool bInitialized = false;
	bool bStartupComplete = false;
};
}
