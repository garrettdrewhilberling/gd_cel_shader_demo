// Copyright 2026 Ares9323 All Rights Reserved.

#include "TonemapperSwapperSettings.h"
#include "TonemapperSwapper.h"
#include "Misc/FileHelper.h"

UTonemapperSwapperSettings::UTonemapperSwapperSettings()
{
	TonemapperMode = ETonemapperSwapperMode::AgX;
	Look = EAgXLook::Saturated;
	ApplyLookPreset(Look);

	GTLook = EGTLook::Default;
	ApplyGTLookPreset(GTLook);
}

void UTonemapperSwapperSettings::PostInitProperties()
{
	Super::PostInitProperties(); // Loads 'config' properties (enums) from ini

	// Float/vector calibration params are NOT marked 'config' (to prevent SaveConfig
	// from removing entries that match the CDO). Load them manually from GConfig.
	LoadCalibrationFromConfig();
}

void UTonemapperSwapperSettings::ApplyLookPreset(EAgXLook InLook)
{
	MinEV = -12.47393f;
	MaxEV = 0.526069f;

	switch (InLook)
	{
	case EAgXLook::Default:
		PreExposureBias = 0.2f;
		Slope = FVector(1.0, 1.0, 1.0);
		Power = FVector(1.0, 1.0, 1.0);
		Saturation = 1.0f;
		Offset = FVector(0.0, 0.0, 0.0);
		break;
	case EAgXLook::Saturated:
		PreExposureBias = 0.4f;
		Slope = FVector(1.0, 1.0, 1.0);
		Power = FVector(1.35, 1.35, 1.35);
		Saturation = 1.4f;
		Offset = FVector(0.0, 0.0, 0.0);
		break;
	case EAgXLook::Custom:
		break;
	}
}

void UTonemapperSwapperSettings::ApplyGTLookPreset(EGTLook InLook)
{
	switch (InLook)
	{
	case EGTLook::Default:
		GTPreExposureBias = 1.0f;
		MaxBrightness = 1.0f;
		GTContrast = 1.0f;
		LinearStart = 0.22f;
		LinearLength = 0.4f;
		BlackTightness = 1.33f;
		Pedestal = 0.0f;
		break;
	case EGTLook::HighContrast:
		GTPreExposureBias = 1.0f;
		MaxBrightness = 1.0f;
		GTContrast = 1.2f;
		LinearStart = 0.18f;
		LinearLength = 0.3f;
		BlackTightness = 1.5f;
		Pedestal = 0.0f;
		break;
	case EGTLook::Custom:
		break;
	}
}

static FString GetCalibrationFilePath()
{
	return FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("TonemapperCalibration.ini"));
}

void UTonemapperSwapperSettings::LoadCalibrationFromConfig()
{
	const FString FilePath = GetCalibrationFilePath();
	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *FilePath))
	{
		return;
	}

	TMap<FString, FString> Pairs;
	TArray<FString> Lines;
	Content.ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		FString Key, Value;
		if (Line.Split(TEXT("="), &Key, &Value))
		{
			Pairs.Add(Key, Value);
		}
	}

	auto GetFloat = [&](const TCHAR* Key, float& OutValue)
	{
		if (const FString* V = Pairs.Find(Key))
		{
			OutValue = FCString::Atof(**V);
		}
	};

	auto GetVector = [&](const TCHAR* Key, FVector& OutValue)
	{
		if (const FString* V = Pairs.Find(Key))
		{
			OutValue.InitFromString(*V);
		}
	};

	// AgX calibration
	GetFloat(TEXT("PreExposureBias"), PreExposureBias);
	GetFloat(TEXT("MinEV"), MinEV);
	GetFloat(TEXT("MaxEV"), MaxEV);
	GetFloat(TEXT("Saturation"), Saturation);
	GetVector(TEXT("Slope"), Slope);
	GetVector(TEXT("Power"), Power);
	GetVector(TEXT("Offset"), Offset);

	// GT calibration
	GetFloat(TEXT("GTPreExposureBias"), GTPreExposureBias);
	GetFloat(TEXT("MaxBrightness"), MaxBrightness);
	GetFloat(TEXT("GTContrast"), GTContrast);
	GetFloat(TEXT("LinearStart"), LinearStart);
	GetFloat(TEXT("LinearLength"), LinearLength);
	GetFloat(TEXT("BlackTightness"), BlackTightness);
	GetFloat(TEXT("Pedestal"), Pedestal);
}

void UTonemapperSwapperSettings::ForceWriteConfigFile()
{
	// Save enum properties via UE's config system.
	SaveConfig();

	// Write float/vector calibration to our own file, completely outside
	// UE's config system. No SaveConfig / TryUpdateDefaultConfigFile interference.
	const FString FilePath = GetCalibrationFilePath();

	FString Content;
	auto WriteFloat = [&](const TCHAR* Key, float Value)
	{
		Content += FString::Printf(TEXT("%s=%f\n"), Key, Value);
	};
	auto WriteVector = [&](const TCHAR* Key, const FVector& Value)
	{
		Content += FString::Printf(TEXT("%s=(X=%f,Y=%f,Z=%f)\n"), Key, Value.X, Value.Y, Value.Z);
	};

	// AgX calibration
	WriteFloat(TEXT("PreExposureBias"), PreExposureBias);
	WriteFloat(TEXT("MinEV"), MinEV);
	WriteFloat(TEXT("MaxEV"), MaxEV);
	WriteFloat(TEXT("Saturation"), Saturation);
	WriteVector(TEXT("Slope"), Slope);
	WriteVector(TEXT("Power"), Power);
	WriteVector(TEXT("Offset"), Offset);

	// GT calibration
	WriteFloat(TEXT("GTPreExposureBias"), GTPreExposureBias);
	WriteFloat(TEXT("MaxBrightness"), MaxBrightness);
	WriteFloat(TEXT("GTContrast"), GTContrast);
	WriteFloat(TEXT("LinearStart"), LinearStart);
	WriteFloat(TEXT("LinearLength"), LinearLength);
	WriteFloat(TEXT("BlackTightness"), BlackTightness);
	WriteFloat(TEXT("Pedestal"), Pedestal);

	FFileHelper::SaveStringToFile(Content, *FilePath);
}

#if WITH_EDITOR
void UTonemapperSwapperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	// --- AgX preset handling ---
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTonemapperSwapperSettings, Look))
	{
		if (Look != EAgXLook::Custom)
		{
			ApplyLookPreset(Look);
		}
	}
	// --- GT preset handling ---
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTonemapperSwapperSettings, GTLook))
	{
		if (GTLook != EGTLook::Custom)
		{
			ApplyGTLookPreset(GTLook);
		}
	}

	// Float/vector properties are NOT marked 'config', so SaveConfig (called by the
	// settings panel after this returns) cannot interfere with our GConfig writes.
	ForceWriteConfigFile();

	FTonemapperSwapperModule& Module = FModuleManager::GetModuleChecked<FTonemapperSwapperModule>("TonemapperSwapper");
	Module.ApplyTonemapperSetting(true);
}
#endif
