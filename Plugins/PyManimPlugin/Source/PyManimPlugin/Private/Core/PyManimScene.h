#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PyManimScene.generated.h"

class UPyManimMobject;
class UPyManimCamera;
class UPyManimAnimation;
class UPyManimRenderer;
class APyManimSceneActor;

UCLASS(BlueprintType)
class UPyManimScene : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void Initialize(int32 Width = 1920, int32 Height = 1080);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	bool IsInitialized() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	UPyManimMobject* CreateSphere(float Radius = 50.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	UPyManimMobject* CreateCube(float Size = 50.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	UPyManimMobject* CreateCylinder(float Radius = 50.0f, float Height = 100.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	UPyManimMobject* CreateCone(float Radius = 50.0f, float Height = 100.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	UPyManimMobject* CreatePlane(float Width = 500.0f, float Height = 500.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	UPyManimMobject* CreateTorus(float OuterRadius = 80.0f, float InnerRadius = 25.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	UPyManimCamera* GetCamera();

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void AddDirectionalLight(const FVector& Direction, const FLinearColor& Color, float Intensity = 10.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity = 5000.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void Play(UPyManimAnimation* Animation);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void Tick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void StopAll();

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	bool HasActiveAnimations() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void RenderFrames(const FString& OutputDirectory, float Duration, float FPS = 30.0f);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void RenderSingleFrame(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	TArray<UPyManimMobject*> GetAllMobjects() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void ClearScene();

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	void Destroy();

private:
	bool bInitialized = false;
	int32 ResolutionWidth = 1920;
	int32 ResolutionHeight = 1080;

	UPROPERTY()
	APyManimSceneActor* SceneActor;

	UPROPERTY()
	UPyManimCamera* Camera;

	UPROPERTY()
	TArray<UPyManimMobject*> Mobjects;

	UPROPERTY()
	TArray<UPyManimAnimation*> ActiveAnimations;

	UPROPERTY()
	UPyManimRenderer* Renderer;
};
