#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionScene.generated.h"

class UUEMotionMobject;
class UUEMotionCamera;
class UUEMotionAnimation;
class UUEMotionRenderer;
class AUEMotionSceneActor;

UCLASS(BlueprintType)
class UUEMotionScene : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void Initialize(int32 Width = 1920, int32 Height = 1080);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	bool IsInitialized() const;

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

private:
	bool bInitialized = false;
	int32 ResolutionWidth = 1920;
	int32 ResolutionHeight = 1080;

	UPROPERTY()
	AUEMotionSceneActor* SceneActor;

	UPROPERTY()
	UUEMotionCamera* Camera;

	UPROPERTY()
	TArray<UUEMotionMobject*> Mobjects;

	UPROPERTY()
	TArray<UUEMotionAnimation*> ActiveAnimations;

	UPROPERTY()
	UUEMotionRenderer* Renderer;
};
