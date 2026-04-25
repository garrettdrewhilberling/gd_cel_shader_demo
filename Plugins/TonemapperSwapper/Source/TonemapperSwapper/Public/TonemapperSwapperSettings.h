// Copyright 2026 Ares9323 All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "TonemapperSwapperSettings.generated.h"

UENUM()
enum class ETonemapperSwapperMode : uint8
{
	AgX UMETA(DisplayName = "AgX"),
	GT UMETA(DisplayName = "GT (Uchimura)"),
	UnrealDefault UMETA(DisplayName = "Unreal Default"),
};

UENUM()
enum class EAgXLook : uint8
{
	Default UMETA(DisplayName = "Default"),
	Saturated UMETA(DisplayName = "Saturated"),
	Custom UMETA(DisplayName = "Custom"),
};

UENUM()
enum class EGTLook : uint8
{
	Default UMETA(DisplayName = "Default"),
	HighContrast UMETA(DisplayName = "High Contrast"),
	Custom UMETA(DisplayName = "Custom"),
};

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "TonemapperSwapper"))
class UTonemapperSwapperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTonemapperSwapperSettings();

	UPROPERTY(config, EditAnywhere, Category = "Tonemapper", meta = (
		DisplayName = "Tonemapper Mode",
		ToolTip = "Choose between AgX, GT (Uchimura) and Unreal's default tonemapper."))
	ETonemapperSwapperMode TonemapperMode;

	// --- AgX ---

	UPROPERTY(config, EditAnywhere, Category = "AgX Calibration", meta = (
		DisplayName = "Look",
		ToolTip = "AgX look preset. Select Custom to edit parameters manually.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::AgX"))
	EAgXLook Look;

	UPROPERTY(EditAnywhere, Category = "AgX Calibration", meta = (
		DisplayName = "Pre-Exposure Bias",
		ToolTip = "Multiplier applied to scene linear color before AgX tonemapping. Adjusts overall brightness to match Unreal's light intensity scale without altering the tonemapper curve.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::AgX && Look == EAgXLook::Custom",
		ClampMin = "0.001", ClampMax = "10.0",
		NoSpinbox = true))
	float PreExposureBias;

	UPROPERTY(EditAnywhere, Category = "AgX Calibration", meta = (
		DisplayName = "Min EV",
		ToolTip = "Minimum exposure value (shadow range). Lower values capture more shadow detail.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::AgX && Look == EAgXLook::Custom",
		NoSpinbox = true))
	float MinEV;

	UPROPERTY(EditAnywhere, Category = "AgX Calibration", meta = (
		DisplayName = "Max EV",
		ToolTip = "Maximum exposure value (highlight range). Higher values capture more highlight detail.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::AgX && Look == EAgXLook::Custom",
		NoSpinbox = true))
	float MaxEV;

	UPROPERTY(EditAnywhere, Category = "AgX Calibration", meta = (
		DisplayName = "Saturation",
		ToolTip = "Color saturation multiplier.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::AgX && Look == EAgXLook::Custom",
		NoSpinbox = true))
	float Saturation;

	UPROPERTY(EditAnywhere, Category = "AgX Calibration", meta = (
		DisplayName = "Slope",
		ToolTip = "Per-channel contrast (ASC CDL slope).",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::AgX && Look == EAgXLook::Custom",
		NoSpinbox = true))
	FVector Slope;

	UPROPERTY(EditAnywhere, Category = "AgX Calibration", meta = (
		DisplayName = "Power",
		ToolTip = "Per-channel gamma/punch (ASC CDL power).",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::AgX && Look == EAgXLook::Custom",
		NoSpinbox = true))
	FVector Power;

	UPROPERTY(EditAnywhere, Category = "AgX Calibration", meta = (
		DisplayName = "Offset",
		ToolTip = "Per-channel brightness offset (ASC CDL offset).",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::AgX && Look == EAgXLook::Custom",
		NoSpinbox = true))
	FVector Offset;

	// --- GT (Uchimura) ---

	UPROPERTY(config, EditAnywhere, Category = "GT Calibration", meta = (
		DisplayName = "Look",
		ToolTip = "GT look preset. Select Custom to edit parameters manually.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::GT"))
	EGTLook GTLook;

	UPROPERTY(EditAnywhere, Category = "GT Calibration", meta = (
		DisplayName = "Pre-Exposure Bias",
		ToolTip = "Multiplier applied to scene linear color before GT tonemapping.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::GT && GTLook == EGTLook::Custom",
		ClampMin = "0.001", ClampMax = "10.0",
		NoSpinbox = true))
	float GTPreExposureBias;

	UPROPERTY(EditAnywhere, Category = "GT Calibration", meta = (
		DisplayName = "Max Brightness (P)",
		ToolTip = "Maximum display brightness. Controls the ceiling of the tone curve.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::GT && GTLook == EGTLook::Custom",
		ClampMin = "0.01", ClampMax = "10.0",
		NoSpinbox = true))
	float MaxBrightness;

	UPROPERTY(EditAnywhere, Category = "GT Calibration", meta = (
		DisplayName = "Contrast (a)",
		ToolTip = "Contrast of the linear section. Higher values increase the slope of midtones.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::GT && GTLook == EGTLook::Custom",
		ClampMin = "0.01", ClampMax = "5.0",
		NoSpinbox = true))
	float GTContrast;

	UPROPERTY(EditAnywhere, Category = "GT Calibration", meta = (
		DisplayName = "Linear Start (m)",
		ToolTip = "Start of the linear section. Lower values extend the toe region.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::GT && GTLook == EGTLook::Custom",
		ClampMin = "0.01", ClampMax = "1.0",
		NoSpinbox = true))
	float LinearStart;

	UPROPERTY(EditAnywhere, Category = "GT Calibration", meta = (
		DisplayName = "Linear Length (l)",
		ToolTip = "Length of the linear section as a fraction. Controls how much of the curve is linear.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::GT && GTLook == EGTLook::Custom",
		ClampMin = "0.01", ClampMax = "1.0",
		NoSpinbox = true))
	float LinearLength;

	UPROPERTY(EditAnywhere, Category = "GT Calibration", meta = (
		DisplayName = "Black Tightness (c)",
		ToolTip = "Tightness of the black/shadow region. Higher values create a sharper toe.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::GT && GTLook == EGTLook::Custom",
		ClampMin = "1.0", ClampMax = "3.0",
		NoSpinbox = true))
	float BlackTightness;

	UPROPERTY(EditAnywhere, Category = "GT Calibration", meta = (
		DisplayName = "Pedestal (b)",
		ToolTip = "Black level pedestal/lift. Raises the black point.",
		EditCondition = "TonemapperMode == ETonemapperSwapperMode::GT && GTLook == EGTLook::Custom",
		ClampMin = "0.0", ClampMax = "1.0",
		NoSpinbox = true))
	float Pedestal;

	void ApplyLookPreset(EAgXLook InLook);
	void ApplyGTLookPreset(EGTLook InLook);

	/** Loads float/vector calibration params from GConfig (not managed by SaveConfig). */
	void LoadCalibrationFromConfig();

	/** Force-writes all calibration params to config, bypassing archetype comparison. */
	void ForceWriteConfigFile();

	virtual void PostInitProperties() override;
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
