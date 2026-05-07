#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionRenderer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUEMotionRendererFinished, bool, bSuccess);

class UWorld;
class ACineCameraActor;
class ULevelSequence;
class UMoviePipelineQueue;
class UMoviePipelineExecutorBase;

UCLASS(BlueprintType)
class UUEMotionRenderer : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "UEMotion|Renderer")
	FOnUEMotionRendererFinished OnRenderFinishedDelegate;
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Renderer")
	void Initialize(UWorld* World, int32 Width = 1920, int32 Height = 1080);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Renderer")
	void BindCamera(AActor* CameraActor);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Renderer")
	void SetAntiAliasing(int32 AAMode);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Renderer")
	void SetOutputFormat(const FString& Format);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Renderer")
	void RenderSequence(ULevelSequence* Sequence, const FString& OutputDirectory, float Duration, float FPS = 30.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Renderer")
	void RenderSingleFrame(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Renderer")
	bool IsRendering() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Renderer")
	float GetProgress() const;

private:
	UPROPERTY()
	UWorld* TargetWorld = nullptr;

	UPROPERTY()
	AActor* BoundCamera = nullptr;

	UPROPERTY()
	ULevelSequence* ActiveSequence = nullptr;

	UPROPERTY()
	UMoviePipelineExecutorBase* ActiveExecutor = nullptr;

	int32 ResolutionWidth = 1920;
	int32 ResolutionHeight = 1080;
	int32 AntiAliasingMode = 1;
	FString OutputFileFormat = TEXT("png");
	bool bIsRendering = false;

	void OnRenderFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess);
};
