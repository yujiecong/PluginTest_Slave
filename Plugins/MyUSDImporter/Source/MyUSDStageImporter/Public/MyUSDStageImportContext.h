// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Objects/MyUSDInfoCache.h"
#include "MyUSDLevelSequenceHelper.h"
#include "MyUSDStageOptions.h"

#include "UsdWrappers/MyUsdStage.h"

#include "CoreMinimal.h"

#include "MyUSDStageImportContext.generated.h"

#define UE_API MYUSDSTAGEIMPORTER_API

class UMyUsdAssetCache2;
class UMyUsdAssetCache3;
class UMyUsdStageImportOptions;
class FTokenizedMessage;

USTRUCT()
struct FMyUsdStageImportContext
{
	GENERATED_BODY()

	UWorld* World;

	/**
	 * Whenever we spawn the scene actor, it should have this local transform and be attached to this parent.
	 * We use this so that Actions->Import can spawn the scene actor exactly where the original stage actor was
	 */
	FTransform TargetSceneActorTargetTransform;
	USceneComponent* TargetSceneActorAttachParent;

	/** Spawned actor that contains the imported scene as a child hierarchy */
	UPROPERTY()
	TObjectPtr<AActor> SceneActor;

	/** Name to use when importing a single mesh */
	UPROPERTY()
	FString ObjectName;

	UPROPERTY()
	FString PackagePath;

	/** Path of the main usd file to import */
	UPROPERTY()
	FString FilePath;

	UPROPERTY()
	TObjectPtr<UMyUsdStageImportOptions> ImportOptions;

	/** Keep track of the last imported object so that we have something valid to return to upstream code that calls the import factories */
	UPROPERTY()
	TObjectPtr<UObject> ImportedAsset;

	UPROPERTY()
	TArray<TObjectPtr<UObject>> ImportedAssets;

	/** Level sequence that will contain the animation data during the import process */
	FMyUsdLevelSequenceHelper LevelSequenceHelper;

	UPROPERTY()
	TObjectPtr<UMyUsdAssetCache3> UsdAssetCache;

	UE_DEPRECATED(5.5, "Use the 'UsdAssetCache' member instead, which is of the new UMyUsdAssetCache3 type")
	UPROPERTY()
	TObjectPtr<UMyUsdAssetCache2> AssetCache;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	/** Caches various information about prims that are expensive to query */
	UE_DEPRECATED(5.3, "The import process now always builds its own InfoCache, so this member is no longer used")
	TSharedPtr<FMyUsdInfoCache> InfoCache;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	/** Bounding box cache used for the USD stage in case we have to spawn bounds components */
	TSharedPtr<UE::FMyUsdGeomBBoxCache> BBoxCache;

	/**
	 * When parsing materials, we keep track of which primvar we mapped to which UV channel.
	 * When parsing meshes later, we use this data to place the correct primvar values in each UV channel.
	 */
	TMap<FString, TMap<FString, int32>> MaterialToPrimvarToUVIndex;

	/** USD Stage to import */
	UE::FMyUsdStage Stage;

	/** Object flags to apply to newly imported objects */
	EObjectFlags ImportObjectFlags;

	/** If true, options dialog won't be shown */
	bool bIsAutomated;

	/**
	 * If true, this will try loading the stage from the static stage cache before re-reading the file. If false,
	 * the USD file at FilePath is reopened (but the stage is left untouched).
	 **/
	bool bReadFromStageCache;

	/** If we're reading from the stage cache and the stage was originally open, it will be left open when the import is completed */
	bool bStageWasOriginallyOpenInCache;

	/** We modify the stage with our meters per unit import option on import. If the stage was already open, we use this to undo the changes after
	 * import */
	double OriginalMetersPerUnit;
	EMyUsdUpAxis OriginalUpAxis;

	/** If we need to run GC after the import is complete */
	bool bNeedsGarbageCollection;

public:
	UE_API FMyUsdStageImportContext();

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	~FMyUsdStageImportContext() = default;
	FMyUsdStageImportContext(const FMyUsdStageImportContext& Other) = default;
	FMyUsdStageImportContext& operator=(const FMyUsdStageImportContext& Other) = default;
	UE_API PRAGMA_ENABLE_DEPRECATION_WARNINGS

	bool Init(
		const FString& InName,
		const FString& InFilePath,
		const FString& InInitialPackagePath,
		EObjectFlags InFlags,
		bool bInIsAutomated,
		bool bIsReimport = false,
		bool bAllowActorImport = true
	);
	UE_API void Reset();

private:
	/** Error messages **/
	TArray<TSharedRef<FTokenizedMessage>> TokenizedErrorMessages;
};

#undef UE_API
