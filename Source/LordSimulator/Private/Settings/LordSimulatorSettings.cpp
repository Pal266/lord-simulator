#include "Settings/LordSimulatorSettings.h"
#include "Localization/LordSimulatorLocalization.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "GenericPlatform/GenericWindow.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Templates/UniquePtr.h"
#include "UnrealEngine.h"
#include "Widgets/SWindow.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Templates/UniquePtr.h"

namespace LordSimulatorSettings
{
	/** The current in-memory settings object. */
	FLordSimulatorSettingsData Current;

	const TCHAR* const SectionHeader = TEXT("[Settings]");
	const TCHAR* const LogModeKey = TEXT("LogMode");
	const TCHAR* const LogModeDefaultValue = TEXT("Default");
	const TCHAR* const LogModeDevelopmentValue = TEXT("Development");

	/** Startup-only input retained until display validation can run against the game window. */
	struct FDisplayInput
	{
		FString Width = TEXT("auto");
		FString Height = TEXT("auto");
		bool bDuplicateDimension = false;
	};

	FDisplayInput StartupDisplay;

	bool ParseDimension(const FString& Value, int32& Out)
	{
		Out = 0;
		for (const TCHAR Digit : Value)
		{
			if (Digit < TEXT('0') || Digit > TEXT('9') || Out > (MAX_int32 - (Digit - TEXT('0'))) / 10)
			{
				return false;
			}
			Out = Out * 10 + (Digit - TEXT('0'));
		}
		return Out > 0;
	}

	bool IsAutomatic(const FDisplayInput& Input)
	{
		return !Input.bDuplicateDimension && Input.Width.Equals(TEXT("auto"), ESearchCase::CaseSensitive)
			&& Input.Height.Equals(TEXT("auto"), ESearchCase::CaseSensitive);
	}

	bool GetExplicitResolution(const FDisplayInput& Input, FIntPoint& Out)
	{
		return !Input.bDuplicateDimension && ParseDimension(Input.Width, Out.X) && ParseDimension(Input.Height, Out.Y);
	}

	void ApplyDisplay(FLordSimulatorSettingsData& Data, const FDisplayInput& Input,
		TFunctionRef<FIntPoint()> Detect, TFunctionRef<bool(FIntPoint, bool)> IsAvailable,
		TFunctionRef<bool(FIntPoint, bool)> Apply, TFunctionRef<void(const FString&)> LogRejection)
	{
		if (!IsAutomatic(Input))
		{
			FIntPoint Requested;
			if (GetExplicitResolution(Input, Requested) && IsAvailable(Requested, Data.bFullscreen)
				&& Apply(Requested, Data.bFullscreen))
			{
				return;
			}

			LogRejection(FString::Printf(TEXT("Configured display resolution '%sx%s' is invalid or unavailable. Falling back to automatic screen resolution."),
				*Input.Width, *Input.Height));
			Data.Width = TEXT("auto");
			Data.Height = TEXT("auto");
		}

		const FIntPoint Detected = Detect();
		if (Detected.X > 0 && Detected.Y > 0)
		{
			Apply(Detected, Data.bFullscreen);
		}
	}

	/** State of one required setting read from the settings file. */
	enum class ESettingReadState : uint8
	{
		Valid,
		Missing,
		Invalid
	};

	/** A complete settings state read from the settings file, with defaults used for missing or invalid settings. */
	struct FReadSettings
	{
		FLordSimulatorSettingsData Data;
		ESettingReadState LogModeState = ESettingReadState::Missing;
		ESettingReadState LanguageState = ESettingReadState::Missing;
		FDisplayInput Display;
		bool bCompleteDisplay = false;

		/** True when every required setting was present and valid. */
		bool IsComplete() const
		{
			return LogModeState == ESettingReadState::Valid && LanguageState == ESettingReadState::Valid && bCompleteDisplay;
		}

		TArray<FString> GetIssues() const
		{
			TArray<FString> Issues;
			if (LogModeState == ESettingReadState::Missing)
				Issues.Add(TEXT("Setting 'LogMode' is missing. Default value 'Default' will be used."));
			else if (LogModeState == ESettingReadState::Invalid)
				Issues.Add(TEXT("Setting 'LogMode' has an invalid value. Default value 'Default' will be used."));
			if (LanguageState == ESettingReadState::Missing)
				Issues.Add(TEXT("Setting 'Language' is missing. Default value 'en' will be used."));
			else if (LanguageState == ESettingReadState::Invalid)
				Issues.Add(TEXT("Setting 'Language' has an invalid value. Default value 'en' will be used."));
			return Issues;
		}
	};

	/** The physical platform file bypasses pak/sandbox layers so absolute paths are used as-is. */
	IPlatformFile& GetPlatformFile()
	{
		return IPlatformFile::GetPlatformPhysical();
	}

	/** <Documents known folder>\Lord Simulator. FPlatformProcess::UserDir() is the Documents known folder on Windows. */
	FString GetDirectory()
	{
		return FPaths::Combine(FPlatformProcess::UserDir(), TEXT("Lord Simulator"));
	}

	FString GetFilePath()
	{
		return FPaths::Combine(GetDirectory(), TEXT("settings.ini"));
	}

	const TCHAR* LogModeToString(ELordSimulatorLogMode Mode)
	{
		return Mode == ELordSimulatorLogMode::Development ? LogModeDevelopmentValue : LogModeDefaultValue;
	}

	/** Serializes the complete settings object in INI format, inside the `[Settings]` section. */
	FString Serialize(const FLordSimulatorSettingsData& Data)
	{
		return FString::Printf(TEXT("%s%s%s=%s%sLanguage=%s%sWidth=%s%sHeight=%s%sFullscreen=%d%s"),
			SectionHeader, LINE_TERMINATOR, LogModeKey, LogModeToString(Data.LogMode), LINE_TERMINATOR, *Data.Language,
			LINE_TERMINATOR, *Data.Width, LINE_TERMINATOR, *Data.Height, LINE_TERMINATOR, Data.bFullscreen ? 1 : 0, LINE_TERMINATOR);
	}

	/** Reads the whole settings file as text. Returns false when the file cannot be read. */
	bool ReadFileText(FString& OutText)
	{
		TUniquePtr<IFileHandle> Handle(GetPlatformFile().OpenRead(*GetFilePath()));
		if (!Handle)
		{
			return false;
		}

		const int64 Size = Handle->Size();
		if (Size < 0 || Size > MAX_int32)
		{
			return false;
		}

		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized(static_cast<int32>(Size));
		if (Size > 0 && !Handle->Read(Bytes.GetData(), Size))
		{
			return false;
		}

		FFileHelper::BufferToString(OutText, Bytes.GetData(), Bytes.Num());
		return true;
	}

	/**
	 * Parses INI text into a complete settings state.
	 *
	 * Only `Key=Value` lines inside the exact, case-sensitive `[Settings]` section are read. Surrounding
	 * whitespace of a section-header line, and of keys and values, is ignored; whitespace inside a section
	 * header is not. Blank lines and lines beginning with `;` or `#` are ignored. Keys and values are
	 * case-sensitive. A setting appearing more than once in the `[Settings]` section is invalid.
	 */
	FReadSettings Parse(const FString& Text)
	{
		bool bInSettingsSection = false;
		int32 LogModeCount = 0;
		FString LogModeValue;
		int32 LanguageCount = 0;
		FString LanguageValue;
		int32 WidthCount = 0;
		int32 HeightCount = 0;
		int32 FullscreenCount = 0;
		FReadSettings Result;
		FString Fullscreen;

		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, /*InCullEmpty*/ true);
		for (const FString& RawLine : Lines)
		{
			const FString Line = RawLine.TrimStartAndEnd();
			if (Line.IsEmpty() || Line.StartsWith(TEXT(";")) || Line.StartsWith(TEXT("#")))
			{
				continue;
			}

			if (Line.StartsWith(TEXT("[")))
			{
				bInSettingsSection = Line.Equals(SectionHeader, ESearchCase::CaseSensitive);
				continue;
			}

			if (!bInSettingsSection)
			{
				continue;
			}

			FString Key;
			FString Value;
			if (!Line.Split(TEXT("="), &Key, &Value)) continue;
			Key.TrimStartAndEndInline();
			Value.TrimStartAndEndInline();
			if (Key.Equals(TEXT("Width"), ESearchCase::CaseSensitive))
			{
				++WidthCount;
				Result.Display.Width = Value;
			}
			else if (Key.Equals(TEXT("Height"), ESearchCase::CaseSensitive))
			{
				++HeightCount;
				Result.Display.Height = Value;
			}
			else if (Key.Equals(TEXT("Fullscreen"), ESearchCase::CaseSensitive))
			{
				++FullscreenCount;
				Fullscreen = Value;
			}

			if (Key.Equals(TEXT("Language"), ESearchCase::CaseSensitive))
			{
				++LanguageCount;
				LanguageValue = Value;
			}
			if (Key.Equals(LogModeKey, ESearchCase::CaseSensitive))
			{
				++LogModeCount;
				LogModeValue = Value;
			}
		}
		Result.Display.bDuplicateDimension = WidthCount > 1 || HeightCount > 1;
		FIntPoint Requested;
		const bool bExplicit = GetExplicitResolution(Result.Display, Requested);
		if (bExplicit)
		{
			Result.Data.Width = Result.Display.Width;
			Result.Data.Height = Result.Display.Height;
		}
		Result.Data.bFullscreen = FullscreenCount != 1 || Fullscreen != TEXT("0");
		Result.bCompleteDisplay = WidthCount == 1 && HeightCount == 1 && FullscreenCount == 1
			&& (IsAutomatic(Result.Display) || bExplicit) && (Fullscreen == TEXT("0") || Fullscreen == TEXT("1"));

		if (LogModeCount == 0)
		{
			Result.LogModeState = ESettingReadState::Missing;
		}
		else if (LogModeCount > 1)
		{
			Result.LogModeState = ESettingReadState::Invalid;
		}
		else if (LogModeValue.Equals(LogModeDefaultValue, ESearchCase::CaseSensitive))
		{
			Result.Data.LogMode = ELordSimulatorLogMode::Default;
			Result.LogModeState = ESettingReadState::Valid;
		}
		else if (LogModeValue.Equals(LogModeDevelopmentValue, ESearchCase::CaseSensitive))
		{
			Result.Data.LogMode = ELordSimulatorLogMode::Development;
			Result.LogModeState = ESettingReadState::Valid;
		}
		else
		{
			Result.LogModeState = ESettingReadState::Invalid;
		}

		if (LanguageCount > 0)
		{
			Result.LanguageState = ESettingReadState::Invalid;
			if (LanguageCount == 1 && FLordSimulatorLocalization::IsValidSetting(LanguageValue))
			{
				Result.LanguageState = ESettingReadState::Valid;
				Result.Data.Language = LanguageValue;
			}
		}
		return Result;
	}

	/** Writes the complete settings object to the settings file, replacing its contents. Returns false on failure. */
	bool WriteFile(const FLordSimulatorSettingsData& Data)
	{
		IPlatformFile& PlatformFile = GetPlatformFile();

		const FString Directory = GetDirectory();
		if (!PlatformFile.DirectoryExists(*Directory) && !PlatformFile.CreateDirectory(*Directory))
		{
			return false;
		}

		TUniquePtr<IFileHandle> Handle(PlatformFile.OpenWrite(*GetFilePath(), /*bAppend*/ false, /*bAllowRead*/ false));
		if (!Handle)
		{
			return false;
		}

		const FTCHARToUTF8 Utf8(*Serialize(Data));
		return Handle->Write(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()) && Handle->Flush();
	}
}

void FLordSimulatorSettings::Load()
{
	using namespace LordSimulatorSettings;

	// The complete state is built first and replaces the in-memory state only at the end.
	FLordSimulatorSettingsData NewState;
	StartupDisplay = FDisplayInput();

	if (!GetPlatformFile().FileExists(*GetFilePath()))
	{
		LS_LOG_DEFAULT(TEXT("Settings file does not exist. A new settings file will be created."));

		if (WriteFile(NewState))
		{
			LS_LOG_DEFAULT(TEXT("Settings file was created."));
		}
		else
		{
			LS_LOG_DEFAULT(TEXT("Settings file could not be created. Default settings will be used."));
		}
	}
	else
	{
		FString Text;
		if (!ReadFileText(Text))
		{
			LS_LOG_DEFAULT(TEXT("Settings file could not be read. Default settings will be used."));
		}
		else
		{
			const FReadSettings Read = Parse(Text);

			for (const FString& Issue : Read.GetIssues())
			{
				LS_LOG_DEFAULT(Issue);
			}
			NewState = Read.Data;
			StartupDisplay = Read.Display;
		}
	}

	Current = NewState;
}

void FLordSimulatorSettings::ApplyStartupDisplay()
{
#if PLATFORM_WINDOWS
	using namespace LordSimulatorSettings;
	if (GIsEditor || IsRunningCommandlet() || !GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport)
	{
		return;
	}

	const TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
	if (!Window || !Window->GetNativeWindow())
	{
		return;
	}

	const HWND Handle = static_cast<HWND>(Window->GetNativeWindow()->GetOSWindowHandle());
	const HMONITOR Monitor = MonitorFromWindow(Handle, MONITOR_DEFAULTTONEAREST);
	MONITORINFOEXW MonitorInfo = {};
	MonitorInfo.cbSize = sizeof(MonitorInfo);
	const bool bHasMonitor = GetMonitorInfoW(Monitor, reinterpret_cast<MONITORINFO*>(&MonitorInfo)) != 0;
	const wchar_t* Device = bHasMonitor ? MonitorInfo.szDevice : nullptr;

	ApplyDisplay(Current, StartupDisplay,
		[Device, &MonitorInfo, bHasMonitor]()
		{
			DEVMODEW Mode = {};
			Mode.dmSize = sizeof(Mode);
			if (EnumDisplaySettingsW(Device, ENUM_CURRENT_SETTINGS, &Mode))
			{
				return FIntPoint(Mode.dmPelsWidth, Mode.dmPelsHeight);
			}
			if (bHasMonitor)
			{
				return FIntPoint(MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
					MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top);
			}
			return FIntPoint::ZeroValue;
		},
		[Device](FIntPoint Resolution, bool bFullscreen)
		{
			if (!bFullscreen)
			{
				DEVMODEW Desktop = {};
				Desktop.dmSize = sizeof(Desktop);
				return !EnumDisplaySettingsW(Device, ENUM_CURRENT_SETTINGS, &Desktop)
					|| (static_cast<DWORD>(Resolution.X) <= Desktop.dmPelsWidth
						&& static_cast<DWORD>(Resolution.Y) <= Desktop.dmPelsHeight);
			}

			DEVMODEW Mode = {};
			Mode.dmSize = sizeof(Mode);
			for (DWORD Index = 0; EnumDisplaySettingsW(Device, Index, &Mode); ++Index)
			{
				if (Mode.dmPelsWidth == static_cast<DWORD>(Resolution.X)
					&& Mode.dmPelsHeight == static_cast<DWORD>(Resolution.Y))
				{
					return true;
				}
			}
			return false;
		},
		[](FIntPoint Resolution, bool bFullscreen)
		{
			const EWindowMode::Type Mode = bFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed;
			FSystemResolution::RequestResolutionChange(Resolution.X, Resolution.Y, Mode);
			IConsoleManager::Get().CallAllConsoleVariableSinks();
			return GSystemResolution.ResX == Resolution.X && GSystemResolution.ResY == Resolution.Y
				&& GSystemResolution.WindowMode == Mode;
		},
		[](const FString& Message) { LS_LOG_DEFAULT(Message); });
#endif
}

void FLordSimulatorSettings::Shutdown()
{
	using namespace LordSimulatorSettings;

	// The persistent state is only compared; it is never loaded into the in-memory settings object.
	// A missing, unreadable, incomplete or invalid file counts as different.
	bool bEquivalent = false;
	FString Text;
	if (GetPlatformFile().FileExists(*GetFilePath()) && ReadFileText(Text))
	{
		const FReadSettings Persistent = Parse(Text);
		bEquivalent = Persistent.IsComplete() && Persistent.Data == Current;
	}

	if (bEquivalent)
	{
		return;
	}

	if (WriteFile(Current))
	{
		LS_LOG_DEFAULT(TEXT("Settings were saved."));
	}
	else
	{
		LS_LOG_DEFAULT(TEXT("Settings could not be saved."));
	}
}

const FLordSimulatorSettingsData& FLordSimulatorSettings::Get()
{
	return LordSimulatorSettings::Current;
}

void FLordSimulatorSettings::SetLogMode(ELordSimulatorLogMode LogMode)
{
	LordSimulatorSettings::Current.LogMode = LogMode;
}

void FLordSimulatorSettings::SetLanguageFromLocalization(const FString& Language)
{
	LordSimulatorSettings::Current.Language = Language;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordLocalizationSettingsTest, "LordSimulator.Localization.Settings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLordLocalizationSettingsTest::RunTest(const FString&)
{
	using namespace LordSimulatorSettings;
	const FLordSimulatorSettingsData Defaults;
	TestEqual(TEXT("Default language"), Defaults.Language, FString(TEXT("en")));
	const FReadSettings Valid = Parse(Serialize(Defaults));
	TestTrue(TEXT("Complete default serialization"), Valid.IsComplete());
	TestTrue(TEXT("Roundtrip complete state"), Valid.Data == Defaults);
	TestTrue(TEXT("Language serialized even at default"), Serialize(Defaults).Contains(TEXT("Language=en")));
	const FReadSettings Missing = Parse(TEXT("[Settings]\nLogMode=Default\n"));
	TestFalse(TEXT("Missing requires normal shutdown repair"), Missing.IsComplete());
	TestEqual(TEXT("Missing defaults"), Missing.Data.Language, FString(TEXT("en")));
	TestEqual(TEXT("Missing exact log"), Missing.GetIssues()[0], FString(TEXT("Setting 'Language' is missing. Default value 'en' will be used.")));
	for (const FString& Lines : {FString(TEXT("Language=EN")), FString(TEXT("Language=en-US")), FString(TEXT("Language=")), FString(TEXT("Language=en\nLanguage=en"))})
	{
		const FReadSettings Invalid = Parse(TEXT("[Settings]\nLogMode=Default\n") + Lines);
		TestFalse(TEXT("Invalid incomplete"), Invalid.IsComplete());
		TestEqual(TEXT("Invalid default language"), Invalid.Data.Language, FString(TEXT("en")));
		TestEqual(TEXT("Invalid exact log"), Invalid.GetIssues()[0], FString(TEXT("Setting 'Language' has an invalid value. Default value 'en' will be used.")));
	}
	FLordSimulatorSettingsData Changed = Defaults;
	Changed.Language = TEXT("fr");
	TestTrue(TEXT("Complete-state comparison includes Language"), Changed != Defaults);
	const FReadSettings Complete = Parse(TEXT("[Settings]\nLanguage=en\nLogMode=Development\nWidth=auto\nHeight=auto\nFullscreen=1\n"));
	TestTrue(TEXT("Language and LogMode parsed together"), Complete.IsComplete());
	TestEqual(TEXT("Other settings preserved"), Complete.Data.LogMode, ELordSimulatorLogMode::Development);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordDisplaySettingsTest, "LordSimulator.Settings.StartupDisplay", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLordDisplaySettingsTest::RunTest(const FString&)
{
	using namespace LordSimulatorSettings;
	const FReadSettings Missing = Parse(TEXT("[Settings]\n"));
	TestEqual(TEXT("Missing width"), Missing.Data.Width, FString(TEXT("auto")));
	TestEqual(TEXT("Missing height"), Missing.Data.Height, FString(TEXT("auto")));
	TestTrue(TEXT("Missing fullscreen"), Missing.Data.bFullscreen);

	const FIntPoint Desktop(2560, 1440);
	const FIntPoint Explicit(1920, 1080);
	const auto Check = [&](const TCHAR* Width, const TCHAR* Height, const TCHAR* Fullscreen,
		bool bAvailable, bool bApplicationSucceeds, bool bRejected, bool bAutomatic, bool bFullscreen)
	{
		FReadSettings Read = Parse(FString::Printf(TEXT("[Settings]\nWidth=%s\nHeight=%s\nFullscreen=%s\n"), Width, Height, Fullscreen));
		int32 Detections = 0;
		TArray<FIntPoint> Attempts;
		TArray<FString> Logs;
		ApplyDisplay(Read.Data, Read.Display,
			[&]() { ++Detections; return Desktop; },
			[&](FIntPoint, bool) { return bAvailable; },
			[&](FIntPoint Resolution, bool bMode)
			{
				TestEqual(TEXT("Applied fullscreen mode"), bMode, bFullscreen);
				Attempts.Add(Resolution);
				return Resolution == Desktop || bApplicationSucceeds;
			},
			[&](const FString& Message) { Logs.Add(Message); });

		TestEqual(TEXT("Detection count"), Detections, bAutomatic ? 1 : 0);
		if (TestTrue(TEXT("A resolution was applied"), !Attempts.IsEmpty()))
		{
			TestEqual(TEXT("Final applied resolution"), Attempts.Last(), bAutomatic ? Desktop : Explicit);
		}
		TestEqual(TEXT("Rejection count"), Logs.Num(), bRejected ? 1 : 0);
		if (bRejected && !Logs.IsEmpty())
		{
			TestEqual(TEXT("Exact rejection message"), Logs[0], FString::Printf(
				TEXT("Configured display resolution '%sx%s' is invalid or unavailable. Falling back to automatic screen resolution."),
				Width, Height));
		}
		if (bAvailable && !bApplicationSucceeds)
		{
			if (TestEqual(TEXT("Explicit request and fallback attempts"), Attempts.Num(), 2))
			{
				TestEqual(TEXT("First attempt"), Attempts[0], Explicit);
			}
		}
	};

	Check(TEXT("auto"), TEXT("auto"), TEXT("1"), true, true, false, true, true);
	Check(TEXT("1920"), TEXT("1080"), TEXT("1"), true, true, false, false, true);
	Check(TEXT("1920"), TEXT("1080"), TEXT("0"), true, true, false, false, false);
	Check(TEXT("1920"), TEXT("1080"), TEXT("invalid"), true, true, false, false, true);
	Check(TEXT("auto"), TEXT("1080"), TEXT("0"), true, true, true, true, false);
	Check(TEXT("1920"), TEXT("auto"), TEXT("1"), true, true, true, true, true);
	for (const TCHAR* Bad : {TEXT("bad"), TEXT("0"), TEXT("-1"), TEXT("2147483648"), TEXT("AUTO")})
	{
		Check(Bad, TEXT("1080"), TEXT("1"), true, true, true, true, true);
		Check(TEXT("1920"), Bad, TEXT("0"), true, true, true, true, false);
	}
	Check(TEXT("1920"), TEXT("1080"), TEXT("0"), false, true, true, true, false);
	Check(TEXT("1920"), TEXT("1080"), TEXT("0"), true, false, true, true, false);
	return true;
}
#endif
