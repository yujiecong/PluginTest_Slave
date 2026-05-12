#include "UEMotionScene.h"
#include "UEMotionMobject.h"
#include "UEMotionAssetFactory.h"
#include "Anim/UEMotionAnimation.h"
#include "Anim/UEMotionGroupAnimation.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"

UUEMotionMobject* UUEMotionScene::CreateSphere(float Radius)
{
	if (!bInitialized || !SceneActor.IsValid()) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	return CreateMobjectFromParams(TEXT("sphere"), Radius);
}

UUEMotionMobject* UUEMotionScene::CreateCube(float Size)
{
	if (!bInitialized || !SceneActor.IsValid()) return nullptr;
	if (Size <= 0.0f) Size = 1.0f;
	return CreateMobjectFromParams(TEXT("cube"), Size);
}

UUEMotionMobject* UUEMotionScene::CreateCylinder(float Radius, float Height)
{
	if (!bInitialized || !SceneActor.IsValid()) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;
	return CreateMobjectFromParams(TEXT("cylinder"), Radius);
}

UUEMotionMobject* UUEMotionScene::CreateCone(float Radius, float Height)
{
	if (!bInitialized || !SceneActor.IsValid()) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;
	return CreateMobjectFromParams(TEXT("cone"), Radius);
}

UUEMotionMobject* UUEMotionScene::CreatePlane(float Width, float Height)
{
	if (!bInitialized || !SceneActor.IsValid()) return nullptr;
	if (Width <= 0.0f) Width = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;
	return CreateMobjectFromParams(TEXT("plane"), Width);
}

UUEMotionMobject* UUEMotionScene::CreateTorus(float OuterRadius, float InnerRadius)
{
	if (!bInitialized || !SceneActor.IsValid()) return nullptr;
	if (OuterRadius <= 0.0f) OuterRadius = 1.0f;
	if (InnerRadius <= 0.0f) InnerRadius = 1.0f;
	return CreateMobjectFromParams(TEXT("torus"), OuterRadius);
}

UUEMotionCamera* UUEMotionScene::GetCamera()
{
	return Camera;
}

UUEMotionAssetFactory* UUEMotionScene::GetAssetFactory()
{
	if (!AssetFactory)
	{
		AssetFactory = NewObject<UUEMotionAssetFactory>(this);
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created AssetFactory instance"));
	}
	return AssetFactory;
}

UUEMotionMobject* UUEMotionScene::CreateMobjectFromAsset(
	const FString& BlueprintPath,
	const FString& MeshType,
	float Size,
	const FLinearColor& Color,
	float Metallic,
	float Roughness,
	float Opacity,
	const FString& CustomMeshPath)
{
	if (!bInitialized || !SceneActor.IsValid()) return nullptr;

	UUEMotionAssetFactory* Factory = GetAssetFactory();
	if (!Factory)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: AssetFactory is null"));
		return nullptr;
	}

	AActor* SpawnedActor = Factory->SpawnFromBlueprintAsset(BlueprintPath);
	if (!SpawnedActor)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to spawn from Blueprint '%s'"), *BlueprintPath);
		return nullptr;
	}

	UStaticMeshComponent* MeshComp = SpawnedActor->FindComponentByClass<UStaticMeshComponent>();

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeFromSpawnedActor(SpawnedActor, MeshComp);
		Obj->SetSourceAssetPath(BlueprintPath);
		Obj->SetMobjectName(MeshType);
		Mobjects.Add(Obj);
		AddActorToSequencer(SpawnedActor);
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created Mobject from Blueprint '%s'"), *BlueprintPath);
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateMobjectFromParams(
	const FString& MeshType,
	float Size,
	const FLinearColor& Color,
	float Metallic,
	float Roughness,
	float Opacity,
	const FString& CustomMeshPath)
{
	if (!bInitialized || !SceneActor.IsValid()) return nullptr;

	UUEMotionAssetFactory* Factory = GetAssetFactory();
	if (!Factory)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: AssetFactory is null, cannot create Mobject"));
		return nullptr;
	}

	FMotionAssetConfig Config;
	Config.MeshType = MeshType;
	Config.Size = Size;
	Config.BaseColor = Color;
	Config.Metallic = Metallic;
	Config.Roughness = Roughness;
	Config.Opacity = Opacity;
	Config.CustomMeshPath = CustomMeshPath;

	FString AssetName = FString::Printf(TEXT("BP_%s_%d"),
		*(Config.MeshType.Left(1).ToUpper() + Config.MeshType.RightChop(1)),
		static_cast<int32>(Config.Size));

	FString BlueprintPath = CreateAndSaveBlueprintAsset(
		Config.MeshType, Config.Size, Config.BaseColor,
		Config.Metallic, Config.Roughness, Config.Opacity,
		Config.CustomMeshPath, AssetName);
	if (BlueprintPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to create Blueprint Asset for '%s'"), *AssetName);
		return nullptr;
	}

	AActor* SpawnedActor = Factory->SpawnFromBlueprintAsset(BlueprintPath);
	if (!SpawnedActor)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to spawn from Blueprint '%s'"), *BlueprintPath);
		return nullptr;
	}

	UStaticMeshComponent* MeshComp = SpawnedActor->FindComponentByClass<UStaticMeshComponent>();

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeFromSpawnedActor(SpawnedActor, MeshComp);
		Obj->SetSourceAssetPath(BlueprintPath);
		Obj->SetMobjectName(Config.MeshType);
		Mobjects.Add(Obj);
		AddActorToSequencer(SpawnedActor);
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created Mobject from Params (Asset-Based) - Type='%s' Size=%.1f BP='%s'"),
		*Config.MeshType, Config.Size, *BlueprintPath);
	return Obj;
}

FString UUEMotionScene::CreateAndSaveBlueprintAsset(
	const FString& MeshType,
	float Size,
	const FLinearColor& Color,
	float Metallic,
	float Roughness,
	float Opacity,
	const FString& CustomMeshPath,
	const FString& AssetName,
	const FString& OutPackagePath)
{
	UUEMotionAssetFactory* Factory = GetAssetFactory();
	if (!Factory) return TEXT("");

	FMotionAssetConfig Config;
	Config.MeshType = MeshType;
	Config.Size = Size;
	Config.BaseColor = Color;
	Config.Metallic = Metallic;
	Config.Roughness = Roughness;
	Config.Opacity = Opacity;
	Config.CustomMeshPath = CustomMeshPath;

	UClass* BPClass = Factory->CreateAndSaveBlueprintAsset(Config, AssetName, OutPackagePath);
	if (!BPClass) return TEXT("");

	FString FullPath = FString::Printf(TEXT("%s/%s"), *OutPackagePath, *AssetName);
	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created and saved Blueprint Asset '%s'"), *FullPath);
	return FullPath;
}

bool UUEMotionScene::DoesAssetExist(const FString& AssetPath)
{
	return UUEMotionAssetFactory::DoesAssetExist(AssetPath);
}

void UUEMotionScene::AddDirectionalLight(const FVector& Direction, const FLinearColor& Color, float Intensity)
{
	if (!bInitialized || !SceneWorld.IsValid()) return;

	ADirectionalLight* Light = SceneWorld->SpawnActor<ADirectionalLight>(FVector(300, -300, 600), Direction.Rotation());
	if (Light)
	{
		UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(Light->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(Intensity);
			LightComp->SetLightColor(Color);
		}
	}
}

void UUEMotionScene::AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity)
{
	if (!bInitialized || !SceneWorld.IsValid()) return;

	APointLight* PointLight = SceneWorld->SpawnActor<APointLight>(Location, FRotator::ZeroRotator);
	if (PointLight)
	{
		UPointLightComponent* LightComp = Cast<UPointLightComponent>(PointLight->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(Intensity);
			LightComp->SetLightColor(Color);

			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Added point light at (%.1f, %.1f, %.1f) with intensity=%.1f"),
				Location.X, Location.Y, Location.Z, Intensity);
		}
	}
}

TArray<UUEMotionMobject*> UUEMotionScene::GetAllMobjects() const
{
	return Mobjects;
}
