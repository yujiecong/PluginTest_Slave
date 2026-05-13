#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionEnvironmentSetup.generated.h"

class UWorld;
class UUEMotionMaterialManager;
class AUEMotionAxisActor;

UCLASS(BlueprintType)
class UUEMotionEnvironmentSetup : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* InWorld, UUEMotionMaterialManager* InMaterialManager);

	void SetupDefaultLighting(bool bUseUnlitMode = true);

	void SetupCoordinateAxes(
		bool bShowAxes = false,
		float AxisLength = 200.0f,
		bool bIs2DView = false
	);

	void SetupSkyEnvironment();

	void SetupBlackBackgroundFloor(
		float FloorSize = 50.0f,
		UUEMotionMaterialManager* MaterialManager = nullptr
	);

	void AddDirectionalLight(
		const FVector& Direction,
		const FLinearColor& Color = FLinearColor::White,
		float Intensity = 10.0f
	);

	void AddPointLight(
		const FVector& Location,
		const FLinearColor& Color = FLinearColor::White,
		float Intensity = 5000.0f
	);

private:
	UPROPERTY()
	TWeakObjectPtr<UWorld> World;

	UPROPERTY()
	UUEMotionMaterialManager* MaterialManager = nullptr;
};
