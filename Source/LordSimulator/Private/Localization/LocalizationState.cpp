#include "Localization/LocalizationState.h"

namespace LordSimulatorLocalization
{
bool FState::ValidateCultures(const TArray<FString>& Cultures)
{
	TSet<FString> Seen;
	for (const FString& Culture : Cultures)
	{
		if (Culture.IsEmpty() || Seen.Contains(Culture)) return false;
		Seen.Add(Culture);
	}
	return Seen.Contains(TEXT("en"));
}

bool FState::SameCultures(const TArray<FString>& A, const TArray<FString>& B)
{
	if (!ValidateCultures(A) || !ValidateCultures(B) || A.Num() != B.Num()) return false;
	for (const FString& Culture : A)
	{
		if (!B.Contains(Culture)) return false;
	}
	return true;
}

bool FState::Initialize()
{
	if (bAttempted) return bInitialized;
	bAttempted = true;
	Environment.Log(TEXT("Localization initialization started."));
	bDiscovered = Environment.Discover(Languages);
	if (!bDiscovered) Languages.Reset();
	bInitialized = bDiscovered && Environment.ValidateResources();
	Environment.Log(bInitialized
		? TEXT("Localization initialization completed.")
		: TEXT("Localization initialization failed. Native language 'en' will be used where available."));
	return bInitialized;
}

bool FState::IsSupported(const FString& Culture) const
{
	return bDiscovered && Languages.ContainsByPredicate([&](const FLordSimulatorLanguage& Entry)
	{
		return Entry.Culture.Equals(Culture, ESearchCase::CaseSensitive);
	});
}

bool FState::IsValidSetting(const FString& Culture) const
{
	return Culture.Equals(TEXT("en"), ESearchCase::CaseSensitive) || IsSupported(Culture);
}

void FState::ApplyStartup()
{
	if (!bInitialized || bStartupComplete) return;
	const FString Culture = Environment.ReadConfigured();
	if (Environment.Apply(Culture))
	{
		Active = Culture;
		Environment.Log(FString::Printf(TEXT("Language '%s' was applied."), *Culture));
		return;
	}
	Active.Reset();
	if (Culture != TEXT("en") && Environment.Apply(TEXT("en")))
	{
		Active = TEXT("en");
		Environment.Log(FString::Printf(TEXT("Language '%s' could not be applied. Native language 'en' will be used for the current session."), *Culture));
	}
	else
	{
		Environment.Log(FString::Printf(TEXT("Language '%s' could not be applied and native language 'en' could not be activated. Localization is unavailable for the current session."), *Culture));
	}
}

void FState::LogRuntimeFailure(const FString& Culture, bool bUnsupported)
{
	const TCHAR* Reason = bUnsupported ? TEXT("is not supported") : TEXT("could not be applied");
	Environment.Log(Active.IsSet()
		? FString::Printf(TEXT("Language '%s' %s. Language '%s' will remain active."), *Culture, Reason, *Active.GetValue())
		: FString::Printf(TEXT("Language '%s' %s. No language is currently active."), *Culture, Reason));
}

bool FState::Change(const FString& Culture)
{
	// Initialization failure takes precedence over unsupported-culture validation (clarification 69).
	if (!bInitialized) { LogRuntimeFailure(Culture, false); return false; }
	if (!IsSupported(Culture)) { LogRuntimeFailure(Culture, true); return false; }
	if (Active.IsSet() && Active.GetValue() == Culture && Environment.ReadConfigured() == Culture) return true;
	if (!Environment.Apply(Culture)) { LogRuntimeFailure(Culture, false); return false; }
	Environment.WriteConfigured(Culture);
	Active = Culture;
	Environment.Log(FString::Printf(TEXT("Language '%s' was applied."), *Culture));
	return true;
}
}
