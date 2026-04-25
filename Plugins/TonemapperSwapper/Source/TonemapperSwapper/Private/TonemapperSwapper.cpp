// Copyright 2026 Ares9323 All Rights Reserved.

#include "TonemapperSwapper.h"
#include "TonemapperSwapperSettings.h"
#include "Engine/RendererSettings.h"
#include "Misc/FileHelper.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformFileManager.h"

DEFINE_LOG_CATEGORY(LogTonemapperSwapper);

// -------------------------------------------------------------------
// FTonemapperShaderRegistry
// -------------------------------------------------------------------

FTonemapperShaderRegistry& FTonemapperShaderRegistry::Get()
{
	static FTonemapperShaderRegistry Instance;
	return Instance;
}

bool FTonemapperShaderRegistry::RegisterShader(const FString& Filename, const FString& SourceCode)
{
	FScopeLock Lock(&RegistryLock);

	if (Filename.IsEmpty() || SourceCode.IsEmpty())
	{
		UE_LOG(LogTonemapperSwapper, Warning, TEXT("Cannot register shader with empty filename or source"));
		return false;
	}

	TSharedPtr<FTonemapperInMemoryShader> Shader = MakeShareable(new FTonemapperInMemoryShader(Filename, SourceCode));
	Shaders.Add(Filename, Shader);

	UE_LOG(LogTonemapperSwapper, Log, TEXT("Registered in-memory shader: %s (%d bytes)"),
		*Filename, Shader->SourceBytes.Num());
	return true;
}

bool FTonemapperShaderRegistry::UnregisterShader(const FString& Filename)
{
	FScopeLock Lock(&RegistryLock);

	if (Shaders.Remove(Filename) > 0)
	{
		UE_LOG(LogTonemapperSwapper, Log, TEXT("Unregistered in-memory shader: %s"), *Filename);
		return true;
	}
	return false;
}

bool FTonemapperShaderRegistry::HasShader(const FString& Filename) const
{
	FScopeLock Lock(&RegistryLock);
	return Shaders.Contains(Filename);
}

bool FTonemapperShaderRegistry::GetShaderBytes(const FString& Filename, TArray<uint8>& OutBytes) const
{
	FScopeLock Lock(&RegistryLock);

	const TSharedPtr<FTonemapperInMemoryShader>* Found = Shaders.Find(Filename);
	if (Found && (*Found)->bActive)
	{
		OutBytes = (*Found)->SourceBytes;
		return true;
	}
	return false;
}

int32 FTonemapperShaderRegistry::GetShaderCount() const
{
	FScopeLock Lock(&RegistryLock);
	return Shaders.Num();
}

void FTonemapperShaderRegistry::ClearAll()
{
	FScopeLock Lock(&RegistryLock);
	Shaders.Empty();
	UE_LOG(LogTonemapperSwapper, Log, TEXT("Cleared all in-memory shaders"));
}

// -------------------------------------------------------------------
// FTonemapperMemoryFileHandle
// -------------------------------------------------------------------

FTonemapperMemoryFileHandle::FTonemapperMemoryFileHandle(const TArray<uint8>& InData)
	: Data(InData)
	, Position(0)
{
}

FTonemapperMemoryFileHandle::~FTonemapperMemoryFileHandle()
{
}

int64 FTonemapperMemoryFileHandle::Tell()
{
	return Position;
}

bool FTonemapperMemoryFileHandle::Seek(int64 NewPosition)
{
	if (NewPosition < 0 || NewPosition > Data.Num())
	{
		return false;
	}
	Position = NewPosition;
	return true;
}

bool FTonemapperMemoryFileHandle::SeekFromEnd(int64 NewPositionRelativeToEnd)
{
	return Seek(Data.Num() + NewPositionRelativeToEnd);
}

bool FTonemapperMemoryFileHandle::Read(uint8* Destination, int64 BytesToRead)
{
	int64 BytesAvailable = Data.Num() - Position;
	int64 BytesToCopy = FMath::Min(BytesToRead, BytesAvailable);

	if (BytesToCopy <= 0)
	{
		return false;
	}

	FMemory::Memcpy(Destination, Data.GetData() + Position, BytesToCopy);
	Position += BytesToCopy;

	return BytesToCopy == BytesToRead;
}

bool FTonemapperMemoryFileHandle::ReadAt(uint8* Destination, int64 BytesToRead, int64 Offset)
{
	if (Offset < 0 || Offset + BytesToRead > Data.Num())
	{
		return false;
	}
	FMemory::Memcpy(Destination, Data.GetData() + Offset, BytesToRead);
	return true;
}

bool FTonemapperMemoryFileHandle::Write(const uint8* Source, int64 BytesToWrite)
{
	return false;
}

bool FTonemapperMemoryFileHandle::Truncate(int64 NewSize)
{
	return false;
}

int64 FTonemapperMemoryFileHandle::Size()
{
	return Data.Num();
}

bool FTonemapperMemoryFileHandle::Flush(const bool bFullFlush)
{
	return true;
}

// -------------------------------------------------------------------
// FTonemapperPlatformFile
// -------------------------------------------------------------------

FTonemapperPlatformFile::FTonemapperPlatformFile(IPlatformFile* InInnerFile)
	: InnerFile(InInnerFile)
{
}

FTonemapperPlatformFile::~FTonemapperPlatformFile()
{
}

bool FTonemapperPlatformFile::ShouldBeUsed(IPlatformFile* Inner, const TCHAR* CmdLine) const
{
	return true;
}

bool FTonemapperPlatformFile::Initialize(IPlatformFile* Inner, const TCHAR* CmdLine)
{
	InnerFile = Inner;
	UE_LOG(LogTonemapperSwapper, Log, TEXT("TonemapperSwapperPlatformFile initialized"));
	return true;
}

bool FTonemapperPlatformFile::ShouldInterceptFile(const TCHAR* Filename) const
{
	if (!Filename)
	{
		return false;
	}

	FString Path(Filename);
	if (!Path.EndsWith(TEXT(".usf")) && !Path.EndsWith(TEXT(".ush")))
	{
		return false;
	}

	return FTonemapperShaderRegistry::Get().HasShader(ExtractShaderFilename(Path));
}

FString FTonemapperPlatformFile::ExtractShaderFilename(const FString& Path) const
{
	return FPaths::GetCleanFilename(Path);
}

bool FTonemapperPlatformFile::FileExists(const TCHAR* Filename)
{
	if (ShouldInterceptFile(Filename))
	{
		return true;
	}
	return InnerFile->FileExists(Filename);
}

int64 FTonemapperPlatformFile::FileSize(const TCHAR* Filename)
{
	if (ShouldInterceptFile(Filename))
	{
		TArray<uint8> Bytes;
		if (FTonemapperShaderRegistry::Get().GetShaderBytes(ExtractShaderFilename(Filename), Bytes))
		{
			return Bytes.Num();
		}
	}
	return InnerFile->FileSize(Filename);
}

bool FTonemapperPlatformFile::DeleteFile(const TCHAR* Filename)
{
	if (ShouldInterceptFile(Filename))
	{
		return false;
	}
	return InnerFile->DeleteFile(Filename);
}

bool FTonemapperPlatformFile::IsReadOnly(const TCHAR* Filename)
{
	if (ShouldInterceptFile(Filename))
	{
		return true;
	}
	return InnerFile->IsReadOnly(Filename);
}

bool FTonemapperPlatformFile::MoveFile(const TCHAR* To, const TCHAR* From)
{
	if (ShouldInterceptFile(From) || ShouldInterceptFile(To))
	{
		return false;
	}
	return InnerFile->MoveFile(To, From);
}

bool FTonemapperPlatformFile::SetReadOnly(const TCHAR* Filename, bool bIsReadOnly)
{
	if (ShouldInterceptFile(Filename))
	{
		return false;
	}
	return InnerFile->SetReadOnly(Filename, bIsReadOnly);
}

FDateTime FTonemapperPlatformFile::GetTimeStamp(const TCHAR* Filename)
{
	if (ShouldInterceptFile(Filename))
	{
		return FDateTime::UtcNow();
	}
	return InnerFile->GetTimeStamp(Filename);
}

void FTonemapperPlatformFile::SetTimeStamp(const TCHAR* Filename, FDateTime DateTime)
{
	if (!ShouldInterceptFile(Filename))
	{
		InnerFile->SetTimeStamp(Filename, DateTime);
	}
}

FDateTime FTonemapperPlatformFile::GetAccessTimeStamp(const TCHAR* Filename)
{
	if (ShouldInterceptFile(Filename))
	{
		return FDateTime::UtcNow();
	}
	return InnerFile->GetAccessTimeStamp(Filename);
}

FString FTonemapperPlatformFile::GetFilenameOnDisk(const TCHAR* Filename)
{
	if (ShouldInterceptFile(Filename))
	{
		return FString(Filename);
	}
	return InnerFile->GetFilenameOnDisk(Filename);
}

IFileHandle* FTonemapperPlatformFile::OpenRead(const TCHAR* Filename, bool bAllowWrite)
{
	if (ShouldInterceptFile(Filename))
	{
		TArray<uint8> Bytes;
		if (FTonemapperShaderRegistry::Get().GetShaderBytes(ExtractShaderFilename(Filename), Bytes))
		{
			UE_LOG(LogTonemapperSwapper, Verbose, TEXT("Serving in-memory shader: %s"),
				*ExtractShaderFilename(Filename));
			return new FTonemapperMemoryFileHandle(Bytes);
		}
	}
	return InnerFile->OpenRead(Filename, bAllowWrite);
}

IFileHandle* FTonemapperPlatformFile::OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead)
{
	if (ShouldInterceptFile(Filename))
	{
		return nullptr;
	}
	return InnerFile->OpenWrite(Filename, bAppend, bAllowRead);
}

bool FTonemapperPlatformFile::DirectoryExists(const TCHAR* Directory)
{
	return InnerFile->DirectoryExists(Directory);
}

bool FTonemapperPlatformFile::CreateDirectory(const TCHAR* Directory)
{
	return InnerFile->CreateDirectory(Directory);
}

bool FTonemapperPlatformFile::DeleteDirectory(const TCHAR* Directory)
{
	return InnerFile->DeleteDirectory(Directory);
}

FFileStatData FTonemapperPlatformFile::GetStatData(const TCHAR* FilenameOrDirectory)
{
	if (ShouldInterceptFile(FilenameOrDirectory))
	{
		TArray<uint8> Bytes;
		if (FTonemapperShaderRegistry::Get().GetShaderBytes(
			ExtractShaderFilename(FilenameOrDirectory), Bytes))
		{
			return FFileStatData(
				FDateTime::UtcNow(),
				FDateTime::UtcNow(),
				Bytes.Num(),
				true,   // bIsFile
				true,   // bIsReadOnly
				true    // bIsValid
			);
		}
	}
	return InnerFile->GetStatData(FilenameOrDirectory);
}

void FTonemapperPlatformFile::FindFiles(TArray<FString>& FoundFiles, const TCHAR* Directory, const TCHAR* FileExtension)
{
	InnerFile->FindFiles(FoundFiles, Directory, FileExtension);
}

void FTonemapperPlatformFile::FindFilesRecursively(TArray<FString>& FoundFiles, const TCHAR* Directory, const TCHAR* FileExtension)
{
	InnerFile->FindFilesRecursively(FoundFiles, Directory, FileExtension);
}

bool FTonemapperPlatformFile::IterateDirectory(const TCHAR* Directory, FDirectoryVisitor& Visitor)
{
	return InnerFile->IterateDirectory(Directory, Visitor);
}

bool FTonemapperPlatformFile::IterateDirectoryRecursively(const TCHAR* Directory, FDirectoryVisitor& Visitor)
{
	return InnerFile->IterateDirectoryRecursively(Directory, Visitor);
}

bool FTonemapperPlatformFile::IterateDirectoryStat(const TCHAR* Directory, FDirectoryStatVisitor& Visitor)
{
	return InnerFile->IterateDirectoryStat(Directory, Visitor);
}

bool FTonemapperPlatformFile::IterateDirectoryStatRecursively(const TCHAR* Directory, FDirectoryStatVisitor& Visitor)
{
	return InnerFile->IterateDirectoryStatRecursively(Directory, Visitor);
}

// -------------------------------------------------------------------
// FTonemapperSwapperModule - Platform File Wrapper
// -------------------------------------------------------------------

bool FTonemapperSwapperModule::InstallPlatformFileWrapper()
{
	FPlatformFileManager& PFM = FPlatformFileManager::Get();
	IPlatformFile* CurrentFile = &PFM.GetPlatformFile();

	if (FCString::Strcmp(CurrentFile->GetName(), TEXT("TonemapperSwapperPlatformFile")) == 0)
	{
		UE_LOG(LogTonemapperSwapper, Warning, TEXT("Platform file wrapper already installed"));
		return true;
	}

	PlatformFileWrapper = new FTonemapperPlatformFile(CurrentFile);
	if (PlatformFileWrapper->Initialize(CurrentFile, nullptr))
	{
		PFM.SetPlatformFile(*PlatformFileWrapper);
		bWrapperInstalled = true;
		UE_LOG(LogTonemapperSwapper, Log, TEXT("Installed TonemapperSwapper platform file wrapper"));
		return true;
	}

	UE_LOG(LogTonemapperSwapper, Error, TEXT("Failed to install platform file wrapper"));
	delete PlatformFileWrapper;
	PlatformFileWrapper = nullptr;
	return false;
}

void FTonemapperSwapperModule::RemovePlatformFileWrapper()
{
	if (!bWrapperInstalled || !PlatformFileWrapper)
	{
		return;
	}

	FPlatformFileManager& PFM = FPlatformFileManager::Get();
	IPlatformFile* CurrentFile = &PFM.GetPlatformFile();

	if (CurrentFile == PlatformFileWrapper)
	{
		PFM.SetPlatformFile(*PlatformFileWrapper->GetLowerLevel());
		UE_LOG(LogTonemapperSwapper, Log, TEXT("Removed TonemapperSwapper platform file wrapper"));
	}

	delete PlatformFileWrapper;
	PlatformFileWrapper = nullptr;
	bWrapperInstalled = false;
}

// -------------------------------------------------------------------
// FTonemapperSwapperModule - Paths
// -------------------------------------------------------------------

FString FTonemapperSwapperModule::GetPluginFilePath(const FString& RelativePath)
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TonemapperSwapper"));
	if (!Plugin.IsValid())
	{
		return FString();
	}
	return FPaths::Combine(Plugin->GetBaseDir(), RelativePath);
}

// -------------------------------------------------------------------
// FTonemapperSwapperModule - Shader Generation
// -------------------------------------------------------------------

FString FTonemapperSwapperModule::GenerateTonemapperShader()
{
	const FString EngineVersion = FString::Printf(TEXT("%d.%d"), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION);
	const FString TemplatePath = GetPluginFilePath(
		FString::Printf(TEXT("Resources/Patched/PostProcessTonemap_%s.usf"), *EngineVersion));

	FString Content;
	if (TemplatePath.IsEmpty() || !FFileHelper::LoadFileToString(Content, *TemplatePath))
	{
		UE_LOG(LogTonemapperSwapper, Error, TEXT("Failed to read tonemap template for engine %s"), *EngineVersion);
		return FString();
	}

	const UTonemapperSwapperSettings* Settings = GetDefault<UTonemapperSwapperSettings>();

	// Set tonemapper mode: 0 = AgX, 1 = GT
	if (Settings->TonemapperMode == ETonemapperSwapperMode::AgX)
	{
		Content = Content.Replace(TEXT("${TONEMAPPER_MODE}"), TEXT("0"));

		Content = Content.Replace(TEXT("${AGX_PRE_EXPOSURE_BIAS}"), *FString::Printf(TEXT("%.6f"), Settings->PreExposureBias));
		Content = Content.Replace(TEXT("${AGX_MIN_EV}"), *FString::Printf(TEXT("%.6f"), Settings->MinEV));
		Content = Content.Replace(TEXT("${AGX_MAX_EV}"), *FString::Printf(TEXT("%.6f"), Settings->MaxEV));
		Content = Content.Replace(TEXT("${AGX_SATURATION}"), *FString::Printf(TEXT("%.6f"), Settings->Saturation));
		Content = Content.Replace(TEXT("${AGX_SLOPE_R}"), *FString::Printf(TEXT("%.6f"), Settings->Slope.X));
		Content = Content.Replace(TEXT("${AGX_SLOPE_G}"), *FString::Printf(TEXT("%.6f"), Settings->Slope.Y));
		Content = Content.Replace(TEXT("${AGX_SLOPE_B}"), *FString::Printf(TEXT("%.6f"), Settings->Slope.Z));
		Content = Content.Replace(TEXT("${AGX_POWER_R}"), *FString::Printf(TEXT("%.6f"), Settings->Power.X));
		Content = Content.Replace(TEXT("${AGX_POWER_G}"), *FString::Printf(TEXT("%.6f"), Settings->Power.Y));
		Content = Content.Replace(TEXT("${AGX_POWER_B}"), *FString::Printf(TEXT("%.6f"), Settings->Power.Z));
		Content = Content.Replace(TEXT("${AGX_OFFSET_R}"), *FString::Printf(TEXT("%.6f"), Settings->Offset.X));
		Content = Content.Replace(TEXT("${AGX_OFFSET_G}"), *FString::Printf(TEXT("%.6f"), Settings->Offset.Y));
		Content = Content.Replace(TEXT("${AGX_OFFSET_B}"), *FString::Printf(TEXT("%.6f"), Settings->Offset.Z));
	}
	else if (Settings->TonemapperMode == ETonemapperSwapperMode::GT)
	{
		Content = Content.Replace(TEXT("${TONEMAPPER_MODE}"), TEXT("1"));

		Content = Content.Replace(TEXT("${GT_PRE_EXPOSURE_BIAS}"), *FString::Printf(TEXT("%.6f"), Settings->GTPreExposureBias));
		Content = Content.Replace(TEXT("${GT_P}"), *FString::Printf(TEXT("%.6f"), Settings->MaxBrightness));
		Content = Content.Replace(TEXT("${GT_A}"), *FString::Printf(TEXT("%.6f"), Settings->GTContrast));
		Content = Content.Replace(TEXT("${GT_M}"), *FString::Printf(TEXT("%.6f"), Settings->LinearStart));
		Content = Content.Replace(TEXT("${GT_L}"), *FString::Printf(TEXT("%.6f"), Settings->LinearLength));
		Content = Content.Replace(TEXT("${GT_C}"), *FString::Printf(TEXT("%.6f"), Settings->BlackTightness));
		Content = Content.Replace(TEXT("${GT_B}"), *FString::Printf(TEXT("%.6f"), Settings->Pedestal));
	}

	return Content;
}

FString FTonemapperSwapperModule::GeneratePatchedCombineLUTs()
{
	const FString CombineLUTsPath = FPaths::Combine(
		FPaths::EngineDir(), TEXT("Shaders/Private/PostProcessCombineLUTs.usf"));

	FString LUTContent;

	// Read the ORIGINAL engine file bypassing our interception layer
	if (bWrapperInstalled && PlatformFileWrapper)
	{
		IPlatformFile* InnerFile = PlatformFileWrapper->GetLowerLevel();
		IFileHandle* Handle = InnerFile->OpenRead(*CombineLUTsPath, false);
		if (Handle)
		{
			int64 FileSize = Handle->Size();
			TArray<uint8> RawBytes;
			RawBytes.SetNumUninitialized(FileSize);
			Handle->Read(RawBytes.GetData(), FileSize);
			delete Handle;

			FUTF8ToTCHAR Converter(
				reinterpret_cast<const ANSICHAR*>(RawBytes.GetData()), RawBytes.Num());
			LUTContent = FString(Converter.Length(), Converter.Get());
		}
	}
	else
	{
		FFileHelper::LoadFileToString(LUTContent, *CombineLUTsPath);
	}

	if (LUTContent.IsEmpty())
	{
		UE_LOG(LogTonemapperSwapper, Error, TEXT("Failed to read engine PostProcessCombineLUTs.usf"));
		return FString();
	}

	bool bPatched = false;

	// Disable gamut expansion
	bPatched |= LUTContent.ReplaceInline(
		TEXT("ColorAP1 = lerp( ColorAP1, ColorExpand, ExpandAmount );"),
		TEXT("ColorAP1 = lerp( ColorAP1, ColorExpand, 0.0 ); // TonemapperSwapper: gamut expansion disabled")) > 0;

	// Disable blue correction (forward)
	bPatched |= LUTContent.ReplaceInline(
		TEXT("ColorAP1 = lerp( ColorAP1, mul( BlueCorrectAP1, ColorAP1 ), BlueCorrection );"),
		TEXT("ColorAP1 = lerp( ColorAP1, mul( BlueCorrectAP1, ColorAP1 ), 0.0 ); // TonemapperSwapper: blue correction disabled")) > 0;

	// Disable blue uncorrection
	bPatched |= LUTContent.ReplaceInline(
		TEXT("ColorAP1 = lerp( ColorAP1, mul( BlueCorrectInvAP1, ColorAP1 ), BlueCorrection );"),
		TEXT("ColorAP1 = lerp( ColorAP1, mul( BlueCorrectInvAP1, ColorAP1 ), 0.0 ); // TonemapperSwapper: blue correction disabled")) > 0;

	// Disable tone curve
	bPatched |= LUTContent.ReplaceInline(
		TEXT("ColorAP1 = lerp(ColorAP1, ToneMappedColorAP1, ToneCurveAmount);"),
		TEXT("ColorAP1 = lerp(ColorAP1, ToneMappedColorAP1, 0.0); // TonemapperSwapper: tone curve disabled")) > 0;

	if (!bPatched)
	{
		UE_LOG(LogTonemapperSwapper, Warning,
			TEXT("CombineLUTs: no patches applied -- engine shader may have changed"));
	}

	return LUTContent;
}

// -------------------------------------------------------------------
// FTonemapperSwapperModule - Apply / Startup / Shutdown
// -------------------------------------------------------------------

void FTonemapperSwapperModule::ApplyTonemapperSetting(bool bRecompileShaders)
{
	const UTonemapperSwapperSettings* Settings = GetDefault<UTonemapperSwapperSettings>();
	FTonemapperShaderRegistry& Registry = FTonemapperShaderRegistry::Get();
	bool bChanged = false;

	if (Settings->TonemapperMode == ETonemapperSwapperMode::AgX
		|| Settings->TonemapperMode == ETonemapperSwapperMode::GT)
	{
		// AgX requires sRGB working color space
		if (Settings->TonemapperMode == ETonemapperSwapperMode::AgX)
		{
			URendererSettings* RenderSettings = GetMutableDefault<URendererSettings>();
			if (RenderSettings->WorkingColorSpaceChoice != EWorkingColorSpace::sRGB)
			{
				RenderSettings->WorkingColorSpaceChoice = EWorkingColorSpace::sRGB;
				RenderSettings->PostEditChange();
				RenderSettings->SaveConfig();
				RenderSettings->TryUpdateDefaultConfigFile();
			}
		}

		// --- PostProcessTonemap ---
		const FString GeneratedTonemap = GenerateTonemapperShader();
		if (!GeneratedTonemap.IsEmpty())
		{
			Registry.UnregisterShader(TEXT("PostProcessTonemap.usf"));
			if (Registry.RegisterShader(TEXT("PostProcessTonemap.usf"), GeneratedTonemap))
			{
				bChanged = true;
			}
		}

		// --- PostProcessCombineLUTs ---
		if (!Registry.HasShader(TEXT("PostProcessCombineLUTs.usf")))
		{
			const FString PatchedLUTs = GeneratePatchedCombineLUTs();
			if (!PatchedLUTs.IsEmpty())
			{
				if (Registry.RegisterShader(TEXT("PostProcessCombineLUTs.usf"), PatchedLUTs))
				{
					bChanged = true;
				}
			}
		}
	}
	else // UnrealDefault
	{
		if (Registry.HasShader(TEXT("PostProcessTonemap.usf")))
		{
			Registry.UnregisterShader(TEXT("PostProcessTonemap.usf"));
			bChanged = true;
		}
		if (Registry.HasShader(TEXT("PostProcessCombineLUTs.usf")))
		{
			Registry.UnregisterShader(TEXT("PostProcessCombineLUTs.usf"));
			bChanged = true;
		}
	}

	if (bChanged && bRecompileShaders && GEngine)
	{
		GEngine->Exec(nullptr, TEXT("RecompileShaders Changed"));
	}
}

void FTonemapperSwapperModule::StartupModule()
{
	UTonemapperSwapperSettings* Settings = GetMutableDefault<UTonemapperSwapperSettings>();

	// Re-load calibration values from config. PostInitProperties may have run before
	// GConfig loaded project ini files, so this ensures Custom values are available.
	Settings->LoadCalibrationFromConfig();

	// For non-Custom presets, overwrite with preset values (config may have stale custom values)
	if (Settings->Look != EAgXLook::Custom)
	{
		Settings->ApplyLookPreset(Settings->Look);
	}
	if (Settings->GTLook != EGTLook::Custom)
	{
		Settings->ApplyGTLookPreset(Settings->GTLook);
	}
	Settings->ForceWriteConfigFile();

	InstallPlatformFileWrapper();
	ApplyTonemapperSetting(true);
}

void FTonemapperSwapperModule::ShutdownModule()
{
	FTonemapperShaderRegistry::Get().ClearAll();
	RemovePlatformFileWrapper();
}

IMPLEMENT_MODULE(FTonemapperSwapperModule, TonemapperSwapper)
