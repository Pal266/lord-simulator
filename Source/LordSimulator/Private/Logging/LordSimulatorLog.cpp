#include "Logging/LordSimulatorLog.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformProcess.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Templates/UniquePtr.h"

#include <atomic>

namespace LordSimulatorLog
{
	/** The active logging mode. Logging starts in Default mode; FLordSimulatorLog::SetMode changes it at runtime. */
	std::atomic<ELordSimulatorLogMode> ActiveMode{ ELordSimulatorLogMode::Default };

	FCriticalSection Mutex;
	TUniquePtr<IFileHandle> File;

	/** Writes one entry. Caller must hold Mutex. */
	void WriteEntry(const FString& Message)
	{
		if (!File)
		{
			return;
		}

		const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
		const FString Line = FString::Printf(TEXT("[%s] %s%s"), *Timestamp, *Message, LINE_TERMINATOR);

		const FTCHARToUTF8 Utf8(*Line);
		if (File->Write(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()))
		{
			File->Flush();
		}
	}
}

void FLordSimulatorLog::Initialize()
{
	using namespace LordSimulatorLog;

	FScopeLock Lock(&Mutex);
	if (File)
	{
		return;
	}

	// The physical platform file bypasses pak/sandbox layers so absolute paths are used as-is.
	IPlatformFile& PlatformFile = IPlatformFile::GetPlatformPhysical();

	// FPlatformProcess::UserDir() is the current user's Documents known folder on Windows.
	const FString Directory = FPaths::Combine(FPlatformProcess::UserDir(), TEXT("Lord Simulator"));
	if (!PlatformFile.DirectoryExists(*Directory) && !PlatformFile.CreateDirectory(*Directory))
	{
		return;
	}

	// Opening without append creates the file or removes its existing contents.
	const FString FilePath = FPaths::Combine(Directory, TEXT("log.log"));
	File.Reset(PlatformFile.OpenWrite(*FilePath, /*bAppend*/ false, /*bAllowRead*/ true));
	if (!File)
	{
		return;
	}

	WriteEntry(TEXT("New log file was created."));
}

void FLordSimulatorLog::Shutdown()
{
	using namespace LordSimulatorLog;

	FScopeLock Lock(&Mutex);
	File.Reset();
}

ELordSimulatorLogMode FLordSimulatorLog::GetMode()
{
	return LordSimulatorLog::ActiveMode.load();
}

void FLordSimulatorLog::SetMode(ELordSimulatorLogMode Mode)
{
	LordSimulatorLog::ActiveMode.store(Mode);
}

bool FLordSimulatorLog::IsDevelopmentMode()
{
	return GetMode() == ELordSimulatorLogMode::Development;
}

void FLordSimulatorLog::LogDefault(const FString& Message)
{
	using namespace LordSimulatorLog;

	FScopeLock Lock(&Mutex);
	WriteEntry(Message);
}

void FLordSimulatorLog::LogDevelopment(const FString& Message)
{
	using namespace LordSimulatorLog;

	if (!IsDevelopmentMode())
	{
		return;
	}

	FScopeLock Lock(&Mutex);
	WriteEntry(Message);
}
