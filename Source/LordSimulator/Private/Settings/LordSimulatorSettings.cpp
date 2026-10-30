#include "Settings/LordSimulatorSettings.h"

#include "GenericPlatform/GenericPlatformFile.h"
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

		/** True when every required setting was present and valid. */
		bool IsComplete() const
		{
			return LogModeState == ESettingReadState::Valid;
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
		return FString::Printf(TEXT("%s%s%s=%s%s"), SectionHeader, LINE_TERMINATOR, LogModeKey, LogModeToString(Data.LogMode), LINE_TERMINATOR);
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
			if (Line.Split(TEXT("="), &Key, &Value) && Key.TrimStartAndEnd().Equals(LogModeKey, ESearchCase::CaseSensitive))
			{
				++LogModeCount;
				LogModeValue = Value.TrimStartAndEnd();
			}
		}

		FReadSettings Result;

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

			if (Read.LogModeState == ESettingReadState::Missing)
			{
				LS_LOG_DEFAULT(TEXT("Setting 'LogMode' is missing. Default value 'Default' will be used."));
			}
			else if (Read.LogModeState == ESettingReadState::Invalid)
			{
				LS_LOG_DEFAULT(TEXT("Setting 'LogMode' has an invalid value. Default value 'Default' will be used."));
			}

			NewState = Read.Data;
		}
	}

	Current = NewState;
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
