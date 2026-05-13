#include "UEMotionScene.h"
#include "UEMotionMaterialManager.h"
#include "Actors/UEMotionAxisActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Engine/Blueprint.h"
#include "ProceduralMeshComponent.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "GameFramework/WorldSettings.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PackageTools.h"
#include "FileHelpers.h"
#include "HAL/PlatformFileManager.h"
#include "Editor/EditorEngine.h"
#include "UnrealEdGlobals.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "FileHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Utils/UEMotionSequencerCompat.h"

bool UUEMotionScene::CreateSceneMap()
{
	if (!GEditor) return false;

	static const FString DefaultMapPath = TEXT("/Game/UEMotion/Maps/M_UEMotionDefault");
	FString MapPath = GetMapPath();

	if (UEditorAssetLibrary::DoesAssetExist(DefaultMapPath))
	{
		if (MapPath != DefaultMapPath)
		{
			if (UEditorAssetLibrary::DoesAssetExist(MapPath))
			{
				UEditorAssetLibrary::DeleteAsset(MapPath);
			}
		}
	}

	if (UEditorAssetLibrary::DoesAssetExist(MapPath))
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Map '%s' already exists, loading it"), *MapPath);
		if (FEditorFileUtils::LoadMap(*MapPath))
		{
			SceneWorld = GEditor->GetEditorWorldContext().World();
		if (SceneWorld.IsValid())
			{
				for (TActorIterator<APointLight> It(SceneWorld.Get()); It; ++It)
				{
					It->Destroy();
				}
			}
			return SceneWorld != nullptr;
		}
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to load existing map '%s', recreating"), *MapPath);
		UEditorAssetLibrary::DeleteAsset(MapPath);
	}

	UWorld* NewWorld = UEditorLoadingAndSavingUtils::NewMapFromTemplate(TEXT("/Engine/Maps/Templates/Template_Default"), true);
	if (!NewWorld) return false;

	SceneWorld = GEditor->GetEditorWorldContext().World();
	if (!SceneWorld.IsValid()) return false;

	for (TActorIterator<AActor> It(SceneWorld.Get()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->GetClass() != AWorldSettings::StaticClass())
		{
			Actor->Destroy();
		}
	}

	for (TActorIterator<APointLight> It(SceneWorld.Get()); It; ++It)
	{
		It->Destroy();
	}

	AWorldSettings* WS = SceneWorld.Get()->GetWorldSettings();
	if (WS)
	{
		WS->bEnableWorldBoundsChecks = false;
	}

	if (MapPath == DefaultMapPath || !UEditorAssetLibrary::DoesAssetExist(DefaultMapPath))
	{
		if (!UEditorLoadingAndSavingUtils::SaveMap(SceneWorld.Get(), MapPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to save map to '%s'"), *MapPath);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created and saved default map to '%s'"), *MapPath);
		}
	}

	return true;
}

bool UUEMotionScene::CreateLevelSequenceAsset()
{
	FString SequencePackageName = GetSequencePath();

	if (UEditorAssetLibrary::DoesAssetExist(SequencePackageName))
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Sequence '%s' already exists, deleting it"), *SequencePackageName);
		UEditorAssetLibrary::DeleteAsset(SequencePackageName);
	}

	UPackage* SequencePackage = CreatePackage(*SequencePackageName);
	if (!SequencePackage) return false;

	ULevelSequence* NewSequence = NewObject<ULevelSequence>(SequencePackage, *FString::Printf(TEXT("LS_%s"), *SceneName), RF_Public | RF_Standalone | RF_Transactional);
	if (!NewSequence) return false;

	NewSequence->Initialize();

	UMovieScene* MovieScene = NewSequence->GetMovieScene();
	if (MovieScene)
	{
		FFrameRate FrameRate(FMath::RoundToInt(PlaybackFPS), 1);
		MovieScene->SetDisplayRate(FrameRate);
		int32 DefaultFrames = FMath::RoundToInt(1.0f * PlaybackFPS);
		FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
		FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, DefaultFrames);
		MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartTick, EndTick), true);
		float EndTime = (float)DefaultFrames / PlaybackFPS;
		MovieScene->SetWorkingRange(0.0, EndTime);
		MovieScene->SetViewRange(0.0, EndTime);
	}

	NewSequence->SetSequenceFlags(
		EMovieSceneSequenceFlags::Volatile | EMovieSceneSequenceFlags::BlockingEvaluation);

	FAssetRegistryModule::AssetCreated(NewSequence);
	UEMotionCompat::MarkPackageDirty(SequencePackage);
	UEMotionCompat::SavePackageToDisk(SequencePackage, TEXT(".uasset"));

	LevelSequence = NewSequence;

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: LevelSequence created and saved to '%s'"), *SequencePackageName);
	return true;
}

void UUEMotionScene::OpenLevelSequenceInEditor()
{
	if (!LevelSequence) return;

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (AssetEditorSubsystem)
	{
		AssetEditorSubsystem->OpenEditorForAsset(LevelSequence);
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Opened LevelSequence in Sequencer editor"));
	}
}
