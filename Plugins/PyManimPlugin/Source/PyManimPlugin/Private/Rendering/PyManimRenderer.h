#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PyManimRenderer.generated.h"

class UWorld;
class ACineCameraActor;
class ULevelSequence;
class UMoviePipelineQueue;
class UMoviePipelineExecutorBase;

UCLASS(BlueprintType)
class UPyManimRenderer : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PyManim|Renderer")
	void Initialize(UWorld* World, int32 Width = 1920, int32 Height = 1080);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Renderer")
	void BindCamera(AActor* CameraActor);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Renderer")
	void SetAntiAliasing(int32 AAMode);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Renderer")
	void SetOutputFormat(const FString& Format);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Renderer")
	void RenderSequence(const FString& OutputDirectory, float Duration, float FPS = 30.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Renderer")
	void RenderSingleFrame(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Renderer")
	bool IsRendering() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Renderer")
	float GetProgress() const;

private:
	UPROPERTY()
	UWorld* TargetWorld = nullptr;

	UPROPERTY()
	AActor* BoundCamera = nullptr;

	UPROPERTY()
	ULevelSequence* TempSequence = nullptr;

	UPROPERTY()
	UMoviePipelineExecutorBase* ActiveExecutor = nullptr;

	int32 ResolutionWidth = 1920;
	int32 ResolutionHeight = 1080;
	int32 AntiAliasingMode = 1;
	FString OutputFileFormat = TEXT("png");
	bool bIsRendering = false;

	ULevelSequence* CreateTempSequence(float Duration, float FPS);
	void OnRenderFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess);
};
