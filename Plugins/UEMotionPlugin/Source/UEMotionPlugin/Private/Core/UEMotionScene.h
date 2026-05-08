#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionScene.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUEMotionRenderFinished, UUEMotionScene*, Scene, bool, bSuccess);

class UUEMotionMobject;
class UUEMotionCamera;
class UUEMotionAnimation;
class UUEMotionRenderer;
class AUEMotionSceneActor;
class ULevelSequence;
class UMovieScene;
class UMovieScene3DTransformTrack;
class UMovieSceneFloatTrack;
class UMovieSceneSpawnTrack;

UCLASS(BlueprintType)
class UUEMotionScene : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "UEMotion")
	FOnUEMotionRenderFinished OnRenderFinished;
	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void Initialize(const FString& SceneName = TEXT("default"), int32 Width = 1920, int32 Height = 1080);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	bool IsInitialized() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	FString GetSceneName() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UWorld* GetSceneWorld() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	ULevelSequence* GetLevelSequence() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UUEMotionMobject* CreateSphere(float Radius = 50.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UUEMotionMobject* CreateCube(float Size = 50.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UUEMotionMobject* CreateCylinder(float Radius = 50.0f, float Height = 100.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UUEMotionMobject* CreateCone(float Radius = 50.0f, float Height = 100.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UUEMotionMobject* CreatePlane(float Width = 500.0f, float Height = 500.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UUEMotionMobject* CreateTorus(float OuterRadius = 80.0f, float InnerRadius = 25.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UUEMotionCamera* GetCamera();

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void AddDirectionalLight(const FVector& Direction, const FLinearColor& Color, float Intensity = 10.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity = 5000.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void Play(UUEMotionAnimation* Animation);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void Wait(float Duration = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void Tick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void StopAll();

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	bool HasActiveAnimations() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void RenderFrames(const FString& OutputDirectory, float Duration, float FPS = 30.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void RenderSingleFrame(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	TArray<UUEMotionMobject*> GetAllMobjects() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void ClearScene();

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void Destroy();

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void SaveAssets();

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void CleanupAssets();

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void UpdateCameraKey();

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void SetAutoCleanup(bool bCleanup);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	bool GetAutoCleanup() const;

private:
	bool bInitialized = false;
	FString SceneName;
	int32 ResolutionWidth = 1920;
	int32 ResolutionHeight = 1080;
	bool bAutoCleanup = true;
	float CurrentTime = 0.0f;
	float PlaybackFPS = 30.0f;

	UPROPERTY()
	UWorld* SceneWorld = nullptr;

	UPROPERTY()
	ULevelSequence* LevelSequence = nullptr;

	UPROPERTY()
	AUEMotionSceneActor* SceneActor = nullptr;

	UPROPERTY()
	UUEMotionCamera* Camera = nullptr;

	UPROPERTY()
	TArray<UUEMotionMobject*> Mobjects;

	UPROPERTY()
	TArray<UUEMotionAnimation*> ActiveAnimations;

	UPROPERTY()
	UUEMotionRenderer* Renderer = nullptr;

	bool CreateSceneMap();
	bool CreateLevelSequenceAsset();
	void SetupDefaultLighting();
	void OpenLevelSequenceInEditor();

	FGuid AddActorToSequencer(AActor* Actor);
	UMovieScene3DTransformTrack* GetOrCreateTransformTrack(UUEMotionMobject* Mobject);
	UMovieSceneFloatTrack* GetOrCreateFloatTrack(UUEMotionMobject* Mobject, const FString& PropertyName);
	void RecordTransformKey(UMovieScene3DTransformTrack* Track, int32 Frame, const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	void RecordFloatKey(UMovieSceneFloatTrack* Track, int32 Frame, float Value);
	void UpdateCameraCutRange(int32 EndFrame);
	void UpdateCameraTransformRange(int32 EndFrame);

	UFUNCTION()
	void OnRendererFinished(bool bSuccess);

	FString GetMapPath() const;
	FString GetSequencePath() const;
};
