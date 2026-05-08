#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionAssetFactory.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceConstant;
class UMaterialInstanceDynamic;
class AActor;
class UBlueprint;

USTRUCT(BlueprintType)
struct FMotionAssetConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
    FString MeshType = TEXT("cube");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
    float Size = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    FLinearColor BaseColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Metallic = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Roughness = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Opacity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    FString NormalMapPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    FString EmissiveColorHex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    bool bSimulatePhysics = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
    FString CustomMeshPath;

    FString GetMeshPath() const
	{
		if (!CustomMeshPath.IsEmpty()) return CustomMeshPath;

		if (MeshType == TEXT("sphere"))  return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		if (MeshType == TEXT("cylinder")) return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		if (MeshType == TEXT("cone"))    return TEXT("/Engine/BasicShapes/Cone.Cone");
		if (MeshType == TEXT("plane"))   return TEXT("/Engine/BasicShapes/Plane.Plane");
		if (MeshType == TEXT("torus"))   return TEXT("/Engine/BasicShapes/Torus.Torus");
		return TEXT("/Engine/BasicShapes/Cube.Cube");
	}
};

UCLASS(BlueprintType)
class UUEMotionAssetFactory : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
    UMaterialInstanceConstant* CreateAndSaveMaterialAsset(
        const FMotionAssetConfig& Config,
        const FString& AssetName,
        const FString& OutPackagePath = TEXT("/Game/UEMotion/Assets/Materials")
    );

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
    UClass* CreateAndSaveBlueprintAsset(
        const FMotionAssetConfig& Config,
        const FString& AssetName,
        const FString& OutPackagePath = TEXT("/Game/UEMotion/Assets/Blueprints")
    );

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
    AActor* SpawnFromBlueprintAsset(
        const FString& BlueprintPath,
        const FVector& Location = FVector(0, 0, 0),
        const FRotator& Rotation = FRotator(0, 0, 0),
        const FVector& Scale = FVector(1, 1, 1)
    );

    UFUNCTION(BlueprintPure, Category = "UEMotion|Asset")
    static bool DoesAssetExist(const FString& AssetPath);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
    UMaterialInstanceDynamic* CreateDynamicMaterialFromConfig(const FMotionAssetConfig& Config);

private:
    UPROPERTY()
    UMaterialInterface* CachedBaseMaterial = nullptr;

    UMaterialInterface* GetOrCreateBaseMaterial();
    bool SaveAssetToObject(UObject* Asset, const FString& PackagePath, const FString& AssetName);
    FString ResolveAssetPath(const FString& PackagePath, const FString& AssetName) const;
};
