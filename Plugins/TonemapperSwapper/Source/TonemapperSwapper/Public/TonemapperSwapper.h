// Copyright 2026 Ares9323 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTonemapperSwapper, Log, All);

// -------------------------------------------------------------------
// FTonemapperInMemoryShader
// -------------------------------------------------------------------

/** Represents an in-memory shader file served to the shader compiler. */
struct FTonemapperInMemoryShader
{
	FString Filename;
	FString SourceCode;
	TArray<uint8> SourceBytes;
	FDateTime Timestamp;
	bool bActive;

	FTonemapperInMemoryShader()
		: Timestamp(FDateTime::UtcNow())
		, bActive(true)
	{
	}

	FTonemapperInMemoryShader(const FString& InFilename, const FString& InSourceCode)
		: Filename(InFilename)
		, SourceCode(InSourceCode)
		, Timestamp(FDateTime::UtcNow())
		, bActive(true)
	{
		FTCHARToUTF8 UTF8Converter(*SourceCode);
		SourceBytes.Append(
			reinterpret_cast<const uint8*>(UTF8Converter.Get()),
			UTF8Converter.Length());
	}
};

// -------------------------------------------------------------------
// FTonemapperShaderRegistry
// -------------------------------------------------------------------

/** Thread-safe singleton registry for in-memory shader overrides. */
class FTonemapperShaderRegistry
{
public:
	static FTonemapperShaderRegistry& Get();

	bool RegisterShader(const FString& Filename, const FString& SourceCode);
	bool UnregisterShader(const FString& Filename);
	bool HasShader(const FString& Filename) const;
	bool GetShaderBytes(const FString& Filename, TArray<uint8>& OutBytes) const;
	int32 GetShaderCount() const;
	void ClearAll();

private:
	FTonemapperShaderRegistry() = default;

	TMap<FString, TSharedPtr<FTonemapperInMemoryShader>> Shaders;
	mutable FCriticalSection RegistryLock;
};

// -------------------------------------------------------------------
// FTonemapperMemoryFileHandle
// -------------------------------------------------------------------

/** IFileHandle that reads from a byte buffer in memory. */
class FTonemapperMemoryFileHandle : public IFileHandle
{
public:
	FTonemapperMemoryFileHandle(const TArray<uint8>& InData);
	virtual ~FTonemapperMemoryFileHandle();

	virtual int64 Tell() override;
	virtual bool Seek(int64 NewPosition) override;
	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) override;
	virtual bool Read(uint8* Destination, int64 BytesToRead) override;
	virtual bool ReadAt(uint8* Destination, int64 BytesToRead, int64 Offset) override;
	virtual bool Write(const uint8* Source, int64 BytesToWrite) override;
	virtual bool Truncate(int64 NewSize) override;
	virtual int64 Size() override;
	virtual bool Flush(const bool bFullFlush = false) override;

private:
	TArray<uint8> Data;
	int64 Position;
};

// -------------------------------------------------------------------
// FTonemapperPlatformFile
// -------------------------------------------------------------------

/**
 * IPlatformFile wrapper that intercepts reads for .usf/.ush files
 * registered in FTonemapperShaderRegistry, serving them from memory.
 */
class FTonemapperPlatformFile : public IPlatformFile
{
public:
	FTonemapperPlatformFile(IPlatformFile* InInnerFile);
	virtual ~FTonemapperPlatformFile();

	virtual bool ShouldBeUsed(IPlatformFile* Inner, const TCHAR* CmdLine) const override;
	virtual bool Initialize(IPlatformFile* Inner, const TCHAR* CmdLine) override;
	virtual IPlatformFile* GetLowerLevel() override { return InnerFile; }
	virtual void SetLowerLevel(IPlatformFile* NewLowerLevel) override { InnerFile = NewLowerLevel; }
	virtual const TCHAR* GetName() const override { return TEXT("TonemapperSwapperPlatformFile"); }

	virtual bool FileExists(const TCHAR* Filename) override;
	virtual int64 FileSize(const TCHAR* Filename) override;
	virtual bool DeleteFile(const TCHAR* Filename) override;
	virtual bool IsReadOnly(const TCHAR* Filename) override;
	virtual bool MoveFile(const TCHAR* To, const TCHAR* From) override;
	virtual bool SetReadOnly(const TCHAR* Filename, bool bIsReadOnly) override;
	virtual FDateTime GetTimeStamp(const TCHAR* Filename) override;
	virtual void SetTimeStamp(const TCHAR* Filename, FDateTime DateTime) override;
	virtual FDateTime GetAccessTimeStamp(const TCHAR* Filename) override;
	virtual FString GetFilenameOnDisk(const TCHAR* Filename) override;

	virtual IFileHandle* OpenRead(const TCHAR* Filename, bool bAllowWrite) override;
	virtual IFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead) override;

	virtual bool DirectoryExists(const TCHAR* Directory) override;
	virtual bool CreateDirectory(const TCHAR* Directory) override;
	virtual bool DeleteDirectory(const TCHAR* Directory) override;

	virtual FFileStatData GetStatData(const TCHAR* FilenameOrDirectory) override;

	virtual void FindFiles(TArray<FString>& FoundFiles, const TCHAR* Directory, const TCHAR* FileExtension) override;
	virtual void FindFilesRecursively(TArray<FString>& FoundFiles, const TCHAR* Directory, const TCHAR* FileExtension) override;

	virtual bool IterateDirectory(const TCHAR* Directory, FDirectoryVisitor& Visitor) override;
	virtual bool IterateDirectoryRecursively(const TCHAR* Directory, FDirectoryVisitor& Visitor) override;
	virtual bool IterateDirectoryStat(const TCHAR* Directory, FDirectoryStatVisitor& Visitor) override;
	virtual bool IterateDirectoryStatRecursively(const TCHAR* Directory, FDirectoryStatVisitor& Visitor) override;

private:
	bool ShouldInterceptFile(const TCHAR* Filename) const;
	FString ExtractShaderFilename(const FString& Path) const;

	IPlatformFile* InnerFile;
};

// -------------------------------------------------------------------
// FTonemapperSwapperModule
// -------------------------------------------------------------------

class FTonemapperSwapperModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void ApplyTonemapperSetting(bool bRecompileShaders);
	FString GenerateTonemapperShader();
	FString GeneratePatchedCombineLUTs();

	static FString GetPluginFilePath(const FString& RelativePath);

private:
	bool InstallPlatformFileWrapper();
	void RemovePlatformFileWrapper();

	FTonemapperPlatformFile* PlatformFileWrapper = nullptr;
	bool bWrapperInstalled = false;
};
