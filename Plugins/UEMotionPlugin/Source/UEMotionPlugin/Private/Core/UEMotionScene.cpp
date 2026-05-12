#include "UEMotionScene.h"
#include "UEMotionCamera.h"
#include "UEMotionMobject.h"
#include "UEMotionAssetFactory.h"
#include "Anim/UEMotionAnimation.h"
#include "Anim/UEMotionGroupAnimation.h"
#include "Anim/UEMotionMoveAnimation.h"
#include "Anim/UEMotionRotateAnimation.h"
#include "Anim/UEMotionScaleAnimation.h"
#include "Anim/UEMotionFadeAnimation.h"
#include "Anim/UEMotionColorAnimation.h"
#include "Anim/UEMotionWaitAnimation.h"
#include "Rendering/UEMotionRenderer.h"
#include "Actors/UEMotionSceneActor.h"
#include "Actors/UEMotionAxisActor.h"
#include "Utils/UEMotionSequencerCompat.h"
#include "UEMotionAssetFactory.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/Blueprint.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "GameFramework/WorldSettings.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlayer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PackageTools.h"
#include "FileHelpers.h"
#include "HAL/PlatformFileManager.h"
#include "Editor/EditorEngine.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "FileHelpers.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceDynamic.h"

FString UUEMotionScene::GetMapPath() const
{
	return FString::Printf(TEXT("/Game/UEMotion/Scenes/%s"), *SceneName);
}

FString UUEMotionScene::GetSequencePath() const
{
	return FString::Printf(TEXT("/Game/UEMotion/Sequences/LS_%s"), *SceneName);
}

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

void UUEMotionScene::SetupDefaultLighting()
{
	if (!SceneWorld.IsValid()) return;

	if (bUseUnlitMode)
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Skipping default lighting (Unlit mode)"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADirectionalLight* DirLight = SceneWorld.Get()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(300, -300, 600), FRotator(-55, 45, 0), SpawnParams);
	if (DirLight)
	{
		UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(DirLight->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(0.0f);
			LightComp->SetLightColor(FLinearColor::Black);

			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Directional light set to zero intensity for dark background"));
		}
	}
}

void UUEMotionScene::SetupCoordinateAxes()
{
	if (!SceneWorld.IsValid() || !bShowCoordinateAxes) return;

	AUEMotionAxisActor::CreateAxisMaterials();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;

	float Len = CoordinateAxisLength;

	TArray<TPair<FString, FLinearColor>> AxisConfigs;
	AxisConfigs.Add(TPair<FString, FLinearColor>(TEXT("BP_Axis_X"), FLinearColor(1.0f, 0.25f, 0.25f, 1.0f)));
	AxisConfigs.Add(TPair<FString, FLinearColor>(TEXT("BP_Axis_Y"), FLinearColor(0.25f, 1.0f, 0.25f, 1.0f)));

	if (!bIs2DView)
	{
		AxisConfigs.Add(TPair<FString, FLinearColor>(TEXT("BP_Axis_Z"), FLinearColor(0.25f, 0.5f, 1.0f, 1.0f)));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: 2D view mode enabled - Z-axis will not be created"));
	}

	for (int32 i = 0; i < AxisConfigs.Num(); i++)
	{
		const FString& BPName = AxisConfigs[i].Key;
		const FLinearColor& Color = AxisConfigs[i].Value;

		FString BlueprintPath = FString::Printf(TEXT("/Game/UEMotion/Blueprints/%s"), *BPName);
		UClass* AxisClass = nullptr;

		if (UEditorAssetLibrary::DoesAssetExist(BlueprintPath))
		{
			UBlueprint* ExistingBP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
			if (ExistingBP && ExistingBP->GeneratedClass)
			{
				AxisClass = ExistingBP->GeneratedClass;
				UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Reusing existing axis blueprint '%s'"), *BlueprintPath);
			}
		}

		if (!AxisClass)
		{
			UPackage* Package = CreatePackage(*BlueprintPath);
			if (Package)
			{
				UBlueprintFactory* BPFactory = NewObject<UBlueprintFactory>();
				BPFactory->ParentClass = AUEMotionAxisActor::StaticClass();

				UBlueprint* NewBP = Cast<UBlueprint>(BPFactory->FactoryCreateNew(
					UBlueprint::StaticClass(), Package, *BPName,
					RF_Public | RF_Standalone, nullptr, GWarn));

				if (NewBP)
				{
					USimpleConstructionScript* SCS = NewBP->SimpleConstructionScript;
					if (SCS)
					{
						USCS_Node* RootNode = SCS->CreateNode(USceneComponent::StaticClass(), TEXT("DefaultSceneRoot"));
						SCS->AddNode(RootNode);

						USCS_Node* MeshNode = SCS->CreateNode(UStaticMeshComponent::StaticClass(), TEXT("Mesh"));
						RootNode->AddChildNode(MeshNode);

						UStaticMeshComponent* MeshTemplate = Cast<UStaticMeshComponent>(MeshNode->ComponentTemplate);
						if (MeshTemplate)
						{
							UStaticMesh* GizmoMesh = LoadObject<UStaticMesh>(
								nullptr, TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle"));
							if (GizmoMesh)
							{
								MeshTemplate->SetStaticMesh(GizmoMesh);
							}

							MeshTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
							MeshTemplate->CastShadow = false;
						}
					}

					FKismetEditorUtilities::CompileBlueprint(NewBP);

					FAssetRegistryModule::AssetCreated(NewBP);
					Package->MarkPackageDirty();

					TArray<UPackage*> PackagesToSave;
					PackagesToSave.Add(Package);

					FSavePackageArgs SaveArgs;
					SaveArgs.SaveFlags = RF_Public | RF_Standalone;

					for (UPackage* Pkg : PackagesToSave)
					{
					 FString PkgFilename = FPackageName::LongPackageNameToFilename(
						 Pkg->GetName(), FPackageName::GetAssetPackageExtension());

					 UPackage::SavePackage(Pkg, nullptr, *PkgFilename, SaveArgs);
					}

					AxisClass = NewBP->GeneratedClass;
					UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created axis blueprint '%s' with SCS configuration"), *BlueprintPath);
				}
			}
		}

		if (AxisClass)
		{
			AActor* SpawnedActor = SceneWorld.Get()->SpawnActor<AActor>(AxisClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (AUEMotionAxisActor* AxisActor = Cast<AUEMotionAxisActor>(SpawnedActor))
			{
				EAxis::Type AxisType = static_cast<EAxis::Type>(i);
				AxisActor->InitializeAxis(AxisType, Len, Color);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Coordinate axes created from blueprints (X=Red, Y=Green, Z=Blue, Length=%.0f)"), Len);
}

void UUEMotionScene::SetupSkyEnvironment()
{
	if (!SceneWorld.IsValid()) return;

	ASkyLight* SkyLight = SceneWorld->SpawnActor<ASkyLight>(FVector(0, 0, 1000), FRotator::ZeroRotator);
	if (SkyLight)
	{
		USkyLightComponent* SkyComp = Cast<USkyLightComponent>(SkyLight->GetComponentByClass(USkyLightComponent::StaticClass()));
		if (SkyComp)
		{
			SkyComp->SetIntensity(0.0f);
			SkyComp->SetRealTimeCaptureEnabled(false);
			SkyComp->SetMobility(EComponentMobility::Movable);

			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created SkyLight with zero intensity for dark background"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Sky environment setup complete (SkyLight only, no Atmosphere/Sphere)"));
}

void UUEMotionScene::SetupBlackBackgroundFloor()
{
	if (!SceneWorld.IsValid()) return;

	FString FloorBlueprintPath = TEXT("/Game/UEMotion/Blueprints/BP_BlackFloor");
	UClass* FloorClass = nullptr;

	if (UEditorAssetLibrary::DoesAssetExist(FloorBlueprintPath))
	{
		UBlueprint* ExistingBP = LoadObject<UBlueprint>(nullptr, *FloorBlueprintPath);
		if (ExistingBP && ExistingBP->GeneratedClass)
		{
			FloorClass = ExistingBP->GeneratedClass;
			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Reusing existing floor blueprint '%s'"), *FloorBlueprintPath);
		}
	}

	if (!FloorClass)
	{
		UPackage* Package = CreatePackage(*FloorBlueprintPath);
		if (Package)
		{
			UBlueprintFactory* BPFactory = NewObject<UBlueprintFactory>();
			BPFactory->ParentClass = AActor::StaticClass();

			UBlueprint* NewBP = Cast<UBlueprint>(BPFactory->FactoryCreateNew(
				UBlueprint::StaticClass(), Package, TEXT("BP_BlackFloor"),
				RF_Public | RF_Standalone, nullptr, GWarn));

			if (NewBP)
			{
				USimpleConstructionScript* SCS = NewBP->SimpleConstructionScript;
				if (SCS)
				{
					USCS_Node* RootNode = SCS->CreateNode(USceneComponent::StaticClass(), TEXT("DefaultSceneRoot"));
					SCS->AddNode(RootNode);

					USCS_Node* MeshNode = SCS->CreateNode(UStaticMeshComponent::StaticClass(), TEXT("FloorMesh"));
					RootNode->AddChildNode(MeshNode);

					UStaticMeshComponent* MeshTemplate = Cast<UStaticMeshComponent>(MeshNode->ComponentTemplate);
					if (MeshTemplate)
					{
						UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(
							nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));

						if (PlaneMesh)
						{
							MeshTemplate->SetStaticMesh(PlaneMesh);

							float FloorSizeCM = FloorSize * 50.0f;
							MeshTemplate->SetRelativeScale3D(FVector(FloorSizeCM / 200.0f, FloorSizeCM / 200.0f, 1.0f));

							MeshTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
							MeshTemplate->CastShadow = false;
							MeshTemplate->bReceivesDecals = false;

							UMaterialInterface* BlackMaterial = CreateOrLoadBlackMaterial();
							if (BlackMaterial)
							{
								MeshTemplate->SetMaterial(0, BlackMaterial);
								UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Black material applied to floor mesh template"));
							}
							else
							{
								UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to create/load black material for floor"));
							}
						}
					}
				}

				FKismetEditorUtilities::CompileBlueprint(NewBP);

				NewBP->SetFlags(RF_Public | RF_Standalone);
				Package->MarkPackageDirty();

#if WITH_EDITOR
				NewBP->PostEditChange();
#endif

				FAssetRegistryModule::AssetCreated(NewBP);

			 FString BPFilename = FPackageName::LongPackageNameToFilename(
				 Package->GetName(), FPackageName::GetAssetPackageExtension());

			 FSavePackageArgs SaveArgs;
			 SaveArgs.SaveFlags = RF_Public | RF_Standalone;

			 UPackage::SavePackage(Package, NewBP, *BPFilename, SaveArgs);

				FloorClass = NewBP->GeneratedClass;
				UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created black floor blueprint '%s' with SCS configuration"), *FloorBlueprintPath);
			}
		}
	}

	if (FloorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bNoFail = true;

		AActor* FloorActor = SceneWorld->SpawnActor<AActor>(
			FloorClass,
			FVector(0, 0, -0.01),
			FRotator::ZeroRotator,
			SpawnParams);

		if (FloorActor)
		{
			float FloorSizeCM = FloorSize * 50.0f;
			UE_LOG(LogTemp, Log,
				TEXT("UEMotionScene: Spawned black background floor from blueprint uasset (%.0f x %.0f cm) [Size=%.1f UEMotion units]"),
				FloorSizeCM, FloorSizeCM, FloorSize);
		}
	}
}

UMaterialInterface* UUEMotionScene::CreateOrLoadBlackMaterial()
{
	const FString MaterialPath = TEXT("/Game/UEMotion/Materials/M_BlackBackground");

	if (UEditorAssetLibrary::DoesAssetExist(MaterialPath))
	{
		UMaterialInterface* ExistingMat = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
		if (ExistingMat)
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Reusing existing black material '%s'"), *MaterialPath);
			return ExistingMat;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Black material exists but failed to load, recreating..."));
			UEditorAssetLibrary::DeleteAsset(MaterialPath);
		}
	}

	UPackage* Package = CreatePackage(*MaterialPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to create package for black material"));
		return nullptr;
	}

	UMaterialInstanceConstant* BlackMIC = NewObject<UMaterialInstanceConstant>(
		Package, TEXT("M_BlackBackground"), RF_Public | RF_Standalone | RF_Transactional);

	if (!BlackMIC)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to create black material instance"));
		return nullptr;
	}

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (!BaseMat)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to load base material 'BasicShapeMaterial'"));
		return nullptr;
	}

	BlackMIC->SetParentEditorOnly(BaseMat);

#if WITH_EDITOR
	FLinearColor PureBlack(0.0f, 0.0f, 0.0f, 1.0f);
	BlackMIC->SetVectorParameterValueEditorOnly(FName("Color"), PureBlack);
	BlackMIC->SetScalarParameterValueEditorOnly(FName("Metallic"), 0.0f);
	BlackMIC->SetScalarParameterValueEditorOnly(FName("Roughness"), 1.0f);
	BlackMIC->SetScalarParameterValueEditorOnly(FName("Specular"), 0.0f);
	BlackMIC->PostEditChange();
#endif

	BlackMIC->SetFlags(RF_Public | RF_Standalone);
	Package->MarkPackageDirty();

	FString FilePath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("UEMotion/Materials/"),
		TEXT("M_BlackBackground.uasset"));

	FSavePackageArgs SaveArgs;
	SaveArgs.SaveFlags = RF_Public | RF_Standalone;

	bool bSaveSuccess = UPackage::SavePackage(
		Package,
		BlackMIC,
		*FilePath,
		SaveArgs);

	if (bSaveSuccess)
	{
		UE_LOG(LogTemp, Log,
			TEXT("UEMotionScene: Created ultra-black material instance '%s'")
			TEXT("\n  Parent: BasicShapeMaterial (Engine Built-in)")
			TEXT("\n  Parameters:")
			TEXT("\n    - Color = (0, 0, 0, 1) [Pure Black]")
			TEXT("\n    - Metallic = 0.0 [Non-metallic]")
			TEXT("\n    - Roughness = 1.0 [Maximum - No reflections]")
			TEXT("\n    - Specular = 0.0 [No highlights]"),
			*FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to save black material to '%s'"), *FilePath);
		return nullptr;
	}

	return BlackMIC;
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

void UUEMotionScene::Initialize(const FString& InSceneName, int32 Width, int32 Height)
{
	if (bInitialized) return;

	SceneName = InSceneName.IsEmpty() ? TEXT("default") : InSceneName;
	ResolutionWidth = Width;
	ResolutionHeight = Height;
	CurrentTime = 0.0f;

	if (!CreateSceneMap())
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene::Initialize: Failed to create scene map '%s'"), *SceneName);
		return;
	}

	if (!CreateLevelSequenceAsset())
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene::Initialize: Failed to create level sequence for '%s'"), *SceneName);
		return;
	}

	FVector Location(-500, -500, 300);
	FRotator Rotation = FRotator::ZeroRotator;
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SceneActor = SceneWorld.Get()->SpawnActor<AUEMotionSceneActor>(AUEMotionSceneActor::StaticClass(), Location, Rotation, SpawnParams);

	if (SceneActor.IsValid())
	{
		SceneActor.Get()->SetOwnerScene(this);

		Camera = NewObject<UUEMotionCamera>(this);
		Camera->Init(SceneActor.Get());
		Camera->LookAt(FVector(0, 0, 0));

		FGuid CameraBinding = AddActorToSequencer(SceneActor.Get());

		if (CameraBinding.IsValid())
		{
			UMovieScene* MovieScene = LevelSequence->GetMovieScene();

			UMovieSceneTrack* CamCutTrack = MovieScene->GetCameraCutTrack();
			if (!CamCutTrack)
			{
				CamCutTrack = MovieScene->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass());
			}
			UMovieSceneCameraCutTrack* CutTrack = Cast<UMovieSceneCameraCutTrack>(CamCutTrack);
			if (CutTrack)
			{
				UMovieSceneCameraCutSection* CamCutSection = NewObject<UMovieSceneCameraCutSection>(CutTrack, NAME_None, RF_Transactional);

				FMovieSceneObjectBindingID BindingID = UE::MovieScene::FRelativeObjectBindingID(CameraBinding);
				CamCutSection->SetCameraBindingID(BindingID);

				FFrameRate DisplayRate = MovieScene->GetDisplayRate();
				int32 DefaultEndFrame = FMath::RoundToInt(2.0f * PlaybackFPS);
				FFrameNumber StartFrame = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
				FFrameNumber EndFrame = UEMotionCompat::DisplayFrameToTick(MovieScene, DefaultEndFrame);
				CamCutSection->SetRange(TRange<FFrameNumber>(StartFrame, EndFrame));

				CutTrack->AddSection(*CamCutSection);

				UE_LOG(LogTemp, Log, TEXT("UEMotionScene: CameraCutSection added - BindingID=%s, Range=[%d, %d)"),
					*CameraBinding.ToString(), StartFrame.Value, EndFrame.Value);
			}

			UMovieScene3DTransformTrack* CamTransformTrack = Cast<UMovieScene3DTransformTrack>(
				MovieScene->AddTrack<UMovieScene3DTransformTrack>(CameraBinding));
			if (CamTransformTrack)
			{
				UMovieSceneSection* CamSection = UEMotionCompat::CreateAndAddSection(CamTransformTrack);
				if (CamSection)
				{
					FFrameNumber StartFrame = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
					int32 DefaultEndFrame = FMath::RoundToInt(2.0f * PlaybackFPS);
					FFrameNumber EndFrame = UEMotionCompat::DisplayFrameToTick(MovieScene, DefaultEndFrame);
					CamSection->SetRange(TRange<FFrameNumber>(StartFrame, EndFrame));

					FVector CamLoc = SceneActor.Get()->GetActorLocation();
					FRotator CamRot = SceneActor.Get()->GetActorRotation();
					FVector CamScale = SceneActor.Get()->GetActorScale();
					UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(CamSection);
					if (TransformSection)
					{
						UEMotionCompat::RecordTransformKeys(MovieScene, TransformSection, 0, CamLoc, CamRot, CamScale);
					}

					UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Camera TransformTrack created - Range=[%d, %d), InitialKey at frame 0"),
						StartFrame.Value, EndFrame.Value);
				}
			}
		}

		SetupDefaultLighting();
		SetupSkyEnvironment();
		SetupBlackBackgroundFloor();
		SetupCoordinateAxes();
		bInitialized = true;

		UEditorLoadingAndSavingUtils::SaveMap(SceneWorld.Get(), GetMapPath());

		OpenLevelSequenceInEditor();

		UE_LOG(LogTemp, Log, TEXT("UEMotionScene::Initialize: Scene '%s' created with Map + LevelSequence"), *SceneName);
	}
}

bool UUEMotionScene::IsInitialized() const
{
	return bInitialized;
}

FString UUEMotionScene::GetSceneName() const
{
	return SceneName;
}

UWorld* UUEMotionScene::GetSceneWorld() const
{
	return SceneWorld.Get();
}

ULevelSequence* UUEMotionScene::GetLevelSequence() const
{
	return LevelSequence;
}

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

FGuid UUEMotionScene::AddActorToSequencer(AActor* Actor)
{
	if (!LevelSequence || !Actor) return FGuid();

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return FGuid();

	FGuid Binding = UEMotionCompat::FindOrAddPossessable(MovieScene, Actor, LevelSequence, SceneWorld.Get());
	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: AddActorToSequencer '%s' -> Binding %s"),
		*Actor->GetName(), Binding.IsValid() ? *Binding.ToString() : TEXT("INVALID"));
	return Binding;
}

UMovieScene3DTransformTrack* UUEMotionScene::GetOrCreateTransformTrack(UUEMotionMobject* Mobject)
{
	if (!LevelSequence || !Mobject) return nullptr;

	AActor* Actor = Mobject->GetInternalActor();
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: GetOrCreateTransformTrack - Mobject has no internal actor"));
		return nullptr;
	}

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return nullptr;

	FGuid ObjectBinding = UEMotionCompat::FindOrAddPossessable(MovieScene, Actor, LevelSequence, SceneWorld.Get());
	if (!ObjectBinding.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: GetOrCreateTransformTrack - Failed to get binding for '%s'"), *Actor->GetName());
		return nullptr;
	}

	UMovieScene3DTransformTrack* ExistingTrack = UEMotionCompat::FindTransformTrack(MovieScene, ObjectBinding);
	if (ExistingTrack)
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Reusing existing TransformTrack for '%s'"), *Actor->GetName());
		return ExistingTrack;
	}

	UMovieScene3DTransformTrack* NewTrack = Cast<UMovieScene3DTransformTrack>(
		MovieScene->AddTrack<UMovieScene3DTransformTrack>(ObjectBinding));
	if (NewTrack)
	{
		UEMotionCompat::CreateAndAddSection(NewTrack);
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Created new TransformTrack for '%s' with binding %s"),
			*Actor->GetName(), *ObjectBinding.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to create TransformTrack for '%s'"), *Actor->GetName());
	}

	return NewTrack;
}

UMovieSceneFloatTrack* UUEMotionScene::GetOrCreateFloatTrack(UUEMotionMobject* Mobject, const FString& PropertyName)
{
	if (!LevelSequence || !Mobject) return nullptr;

	AActor* Actor = Mobject->GetInternalActor();
	if (!Actor) return nullptr;

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return nullptr;

	FGuid ObjectBinding = UEMotionCompat::FindOrAddPossessable(MovieScene, Actor, LevelSequence, SceneWorld.Get());
	if (!ObjectBinding.IsValid()) return nullptr;

	UMovieSceneFloatTrack* NewTrack = Cast<UMovieSceneFloatTrack>(
		MovieScene->AddTrack<UMovieSceneFloatTrack>(ObjectBinding));
	if (NewTrack)
	{
		NewTrack->SetPropertyNameAndPath(FName(*PropertyName), PropertyName);
		UEMotionCompat::CreateAndAddSection(NewTrack);
	}

	return NewTrack;
}

void UUEMotionScene::RecordTransformKey(UMovieScene3DTransformTrack* Track, int32 Frame, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	if (!Track || !LevelSequence) return;

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return;

	UMovieSceneSection* Section = Track->GetAllSections().Num() > 0 ? Track->GetAllSections()[0] : nullptr;
	if (!Section) return;

	UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(Section);
	if (!TransformSection) return;

	FFrameNumber TickFrame = UEMotionCompat::DisplayFrameToTick(MovieScene, Frame);
	UEMotionCompat::RecordTransformKeys(MovieScene, TransformSection, Frame, Location, Rotation, Scale);

	TRange<FFrameNumber> CurrentRange = Section->GetRange();
	FFrameNumber MinTick = CurrentRange.GetLowerBound().IsOpen() ? TickFrame : FMath::Min(CurrentRange.GetLowerBoundValue(), TickFrame);
	FFrameNumber MaxTick = CurrentRange.GetUpperBound().IsOpen() ? TickFrame : FMath::Max(CurrentRange.GetUpperBoundValue(), TickFrame);
	Section->SetRange(TRange<FFrameNumber>(MinTick, MaxTick));

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Recorded key at DisplayFrame=%d (Tick=%d) - Loc(%s) Rot(%s) Scale(%s) [Section Range: %d-%d]"),
		Frame, TickFrame.Value, *Location.ToString(), *Rotation.ToString(), *Scale.ToString(), MinTick.Value, MaxTick.Value);
}

void UUEMotionScene::RecordFloatKey(UMovieSceneFloatTrack* Track, int32 Frame, float Value)
{
	if (!Track || !LevelSequence) return;

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return;

	UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->GetAllSections().Num() > 0 ? Track->GetAllSections()[0] : nullptr);
	if (!Section) return;

	FFrameNumber TickFrame = UEMotionCompat::DisplayFrameToTick(MovieScene, Frame);
	UEMotionCompat::RecordFloatKey(MovieScene, Section, Frame, Value);

	TRange<FFrameNumber> CurrentRange = Section->GetRange();
	FFrameNumber MinTick = CurrentRange.GetLowerBound().IsOpen() ? TickFrame : FMath::Min(CurrentRange.GetLowerBoundValue(), TickFrame);
	FFrameNumber MaxTick = CurrentRange.GetUpperBound().IsOpen() ? TickFrame : FMath::Max(CurrentRange.GetUpperBoundValue(), TickFrame);
	Section->SetRange(TRange<FFrameNumber>(MinTick, MaxTick));
}

void UUEMotionScene::UpdateCameraCutRange(int32 EndFrame)
{
	if (!LevelSequence) return;
	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return;

	UMovieSceneTrack* CamCutTrack = MovieScene->GetCameraCutTrack();
	if (!CamCutTrack) return;

	FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
	FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, EndFrame);

	for (UMovieSceneSection* Section : CamCutTrack->GetAllSections())
	{
		if (Section)
		{
			Section->SetRange(TRange<FFrameNumber>(StartTick, EndTick));
		}
	}
}

void UUEMotionScene::UpdateCameraTransformRange(int32 EndFrame)
{
	if (!LevelSequence || !SceneActor.IsValid()) return;
	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return;

	FGuid CameraBinding = UEMotionCompat::FindObjectBinding(MovieScene, SceneActor.Get());
	if (!CameraBinding.IsValid()) return;

	UMovieScene3DTransformTrack* CamTrack = UEMotionCompat::FindTransformTrack(MovieScene, CameraBinding);
	if (!CamTrack) return;

	FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
	FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, EndFrame);

	for (UMovieSceneSection* Section : CamTrack->GetAllSections())
	{
		if (Section)
		{
			TRange<FFrameNumber> CurrentRange = Section->GetRange();
			FFrameNumber CurrentEnd = CurrentRange.GetUpperBound().IsOpen() ? EndTick : FMath::Max(CurrentRange.GetUpperBoundValue(), EndTick);
			Section->SetRange(TRange<FFrameNumber>(StartTick, CurrentEnd));
		}
	}
}

void UUEMotionScene::Play(UUEMotionAnimation* Animation)
{
	if (!Animation || !bInitialized) return;
	if (Animation->GetDuration() <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::Play: Animation duration <= 0, skipping"));
		return;
	}

	UUEMotionGroupAnimation* Group = Cast<UUEMotionGroupAnimation>(Animation);
	if (Group && Group->GetAnimationCount() == 0) return;

	float StartTime = CurrentTime;
	float Duration = Animation->GetDuration();
	float EndTime = StartTime + Duration;

	int32 StartFrame = FMath::RoundToInt(StartTime * PlaybackFPS);
	int32 EndFrame = FMath::RoundToInt(EndTime * PlaybackFPS);

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene::Play: Duration=%.2f, Frames %d->%d, CurrentTime=%.2f->%.2f"),
		Duration, StartFrame, EndFrame, StartTime, EndTime);

	if (UUEMotionMoveAnimation* MoveAnim = Cast<UUEMotionMoveAnimation>(Animation))
	{
		UUEMotionMobject* MoveTarget = MoveAnim->GetTargetMobject();
		if (MoveTarget)
		{
			UMovieScene3DTransformTrack* Track = GetOrCreateTransformTrack(MoveTarget);
			if (Track)
			{
				FVector StartLoc = MoveTarget->GetLocation();
				FVector EndLoc = MoveAnim->GetTargetLocation();
				FRotator Rot = MoveTarget->GetRotation();
				FVector Scale = MoveTarget->GetScale();

				RecordTransformKey(Track, StartFrame, StartLoc, Rot, Scale);
				RecordTransformKey(Track, EndFrame, EndLoc, Rot, Scale);
			}
		}
	}
	else if (UUEMotionRotateAnimation* RotAnim = Cast<UUEMotionRotateAnimation>(Animation))
	{
		UUEMotionMobject* RotTarget = RotAnim->GetTargetMobject();
		if (RotTarget)
		{
			UMovieScene3DTransformTrack* Track = GetOrCreateTransformTrack(RotTarget);
			if (Track)
			{
				FVector Loc = RotTarget->GetLocation();
				FRotator StartRot = RotTarget->GetRotation();
				FRotator EndRot = StartRot + FRotator(0, RotAnim->GetRotationAngle(), 0);
				FVector Scale = RotTarget->GetScale();

				RecordTransformKey(Track, StartFrame, Loc, StartRot, Scale);
				RecordTransformKey(Track, EndFrame, Loc, EndRot, Scale);
			}
		}
	}
	else if (UUEMotionScaleAnimation* ScaleAnim = Cast<UUEMotionScaleAnimation>(Animation))
	{
		UUEMotionMobject* ScaleTarget = ScaleAnim->GetTargetMobject();
		if (ScaleTarget)
		{
			UMovieScene3DTransformTrack* Track = GetOrCreateTransformTrack(ScaleTarget);
			if (Track)
			{
				FVector Loc = ScaleTarget->GetLocation();
				FRotator Rot = ScaleTarget->GetRotation();
				FVector StartScale = ScaleTarget->GetScale();
				FVector EndScale = ScaleAnim->GetEndScale();

				RecordTransformKey(Track, StartFrame, Loc, Rot, StartScale);
				RecordTransformKey(Track, EndFrame, Loc, Rot, EndScale);
			}
		}
	}
	else if (UUEMotionFadeAnimation* FadeAnim = Cast<UUEMotionFadeAnimation>(Animation))
	{
		UUEMotionMobject* Target = FadeAnim->GetTargetMobject();
		if (Target && Target->GetInternalActor())
		{
			UMovieSceneFloatTrack* OpacityTrack = GetOrCreateFloatTrack(Target, TEXT("Opacity"));
			if (OpacityTrack)
			{
				float StartOpacity = FadeAnim->IsFadeOut() ? 1.0f : 0.0f;
				float EndOpacity = FadeAnim->IsFadeOut() ? 0.0f : 1.0f;

				RecordFloatKey(OpacityTrack, StartFrame, StartOpacity);
				RecordFloatKey(OpacityTrack, EndFrame, EndOpacity);

				UE_LOG(LogTemp, Log,
					TEXT("UEMotionScene: Recorded per-instance fade keys for '%s' - Start=%.2f@Frame%d End=%.2f@Frame%d"),
					*Target->GetName(), StartOpacity, StartFrame, EndOpacity, EndFrame);
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("UEMotionScene: Failed to create Opacity FloatTrack for '%s'"),
					*Target->GetName());
			}
		}
	}
	else if (UUEMotionGroupAnimation* GroupAnim = Cast<UUEMotionGroupAnimation>(Animation))
	{
		for (UUEMotionAnimation* ChildAnim : GroupAnim->GetChildAnimations())
		{
			Play(ChildAnim);
		}
		return;
	}

	ActiveAnimations.Add(Animation);
	Animation->Reset();

	CurrentTime = EndTime;

	UMovieScene* MovieScene = LevelSequence ? LevelSequence->GetMovieScene() : nullptr;
	if (MovieScene)
	{
		int32 TotalFrames = FMath::RoundToInt(CurrentTime * PlaybackFPS) + 1;
		FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
		FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, TotalFrames);
		MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartTick, EndTick), true);
		float RangeEndTime = (float)TotalFrames / PlaybackFPS;
		MovieScene->SetWorkingRange(0.0, RangeEndTime);
		MovieScene->SetViewRange(0.0, RangeEndTime);
		UpdateCameraCutRange(TotalFrames);
		UpdateCameraTransformRange(TotalFrames);
	}
}

void UUEMotionScene::Wait(float Duration)
{
	CurrentTime += Duration;

	UMovieScene* MovieScene = LevelSequence ? LevelSequence->GetMovieScene() : nullptr;
	if (MovieScene)
	{
		int32 TotalFrames = FMath::RoundToInt(CurrentTime * PlaybackFPS) + 1;
		FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
		FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, TotalFrames);
		MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartTick, EndTick), true);
		float RangeEndTime = (float)TotalFrames / PlaybackFPS;
		MovieScene->SetWorkingRange(0.0, RangeEndTime);
		MovieScene->SetViewRange(0.0, RangeEndTime);
		UpdateCameraCutRange(TotalFrames);
		UpdateCameraTransformRange(TotalFrames);
	}
}

void UUEMotionScene::StopAll()
{
	ActiveAnimations.Empty();
}

bool UUEMotionScene::HasActiveAnimations() const
{
	return ActiveAnimations.Num() > 0;
}

void UUEMotionScene::UpdateAnimations(float DeltaTime)
{
	for (int32 i = ActiveAnimations.Num() - 1; i >= 0; --i)
	{
		UUEMotionAnimation* Animation = ActiveAnimations[i];
		if (!Animation) continue;

		Animation->Advance(DeltaTime);

		if (Animation->IsFinished())
		{
			ActiveAnimations.RemoveAt(i);
		}
	}
}

void UUEMotionScene::RenderFrames(const FString& OutputDirectory, float Duration, float FPS)
{
	if (!bInitialized || !LevelSequence) return;

	if (!Renderer)
	{
		Renderer = NewObject<UUEMotionRenderer>(this);
		Renderer->Initialize(SceneWorld.Get(), ResolutionWidth, ResolutionHeight);
		Renderer->SetUseUnlit(bUseUnlitMode);
		Renderer->OnRenderFinishedDelegate.AddDynamic(this, &UUEMotionScene::OnRendererFinished);
	}

	Renderer->BindCamera(SceneActor.Get());

	SaveAssets();

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene::RenderFrames: Starting MRQ render to '%s', Duration=%.2fs, FPS=%.0f"),
		*OutputDirectory, Duration, FPS);

	Renderer->RenderSequence(LevelSequence, OutputDirectory, Duration, FPS);
}

void UUEMotionScene::RenderSingleFrame(const FString& FilePath)
{
	if (!bInitialized || !LevelSequence) return;

	FString Directory = FPaths::GetPath(FilePath);
	FString Extension = FPaths::GetExtension(FilePath);
	if (Directory.IsEmpty())
	{
		Directory = FPaths::ProjectSavedDir() / TEXT("UEMotion") / TEXT("Renders");
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	RenderFrames(Directory, 1.0f / 30.0f, 30.0f);
}

void UUEMotionScene::ClearScene()
{
	for (UUEMotionMobject* Obj : Mobjects)
	{
		if (Obj)
		{
			Obj->Destroy();
		}
	}
	Mobjects.Empty();
}

void UUEMotionScene::Destroy()
{
	ClearScene();

	if (SceneActor.IsValid())
	{
		SceneActor.Get()->Destroy();
		SceneActor.Reset();
	}

	if (Renderer)
	{
		Renderer->OnRenderFinishedDelegate.RemoveAll(this);
		Renderer = nullptr;
	}

	Camera = nullptr;
	LevelSequence = nullptr;
	SceneWorld.Reset();
	bInitialized = false;

	if (bAutoCleanup)
	{
		CleanupAssets();
	}
}

void UUEMotionScene::SaveAssets()
{
	if (LevelSequence)
	{
		LevelSequence->MarkPackageDirty();
		UPackage* SeqPackage = LevelSequence->GetOutermost();
		if (SeqPackage)
		{
			UEMotionCompat::MarkPackageDirty(SeqPackage);
			if (UEMotionCompat::SavePackageToDisk(SeqPackage, TEXT(".uasset")))
			{
				UE_LOG(LogTemp, Log, TEXT("UEMotionScene: LevelSequence saved to '%s'"), *SeqPackage->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to save LevelSequence '%s'"), *SeqPackage->GetName());
			}
		}
	}

	if (SceneWorld.IsValid())
	{
		FString MapPath = GetMapPath();
		if (UEditorLoadingAndSavingUtils::SaveMap(SceneWorld.Get(), MapPath))
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Map saved to '%s'"), *MapPath);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to save map '%s'"), *MapPath);
		}
	}
}

void UUEMotionScene::CleanupAssets()
{
	FString MapPath = GetMapPath();
	FString SeqPath = GetSequencePath();

	if (UEditorAssetLibrary::DoesAssetExist(MapPath))
	{
		UEditorAssetLibrary::DeleteAsset(MapPath);
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Cleaned up map asset: %s"), *MapPath);
	}

	if (UEditorAssetLibrary::DoesAssetExist(SeqPath))
	{
		UEditorAssetLibrary::DeleteAsset(SeqPath);
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Cleaned up sequence asset: %s"), *SeqPath);
	}
}

void UUEMotionScene::UpdateCameraKey()
{
	if (!LevelSequence || !SceneActor.IsValid() || !bInitialized) return;

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return;

	FGuid CameraBinding = UEMotionCompat::FindObjectBinding(MovieScene, SceneActor.Get());
	if (!CameraBinding.IsValid()) return;

	UMovieScene3DTransformTrack* CamTrack = UEMotionCompat::FindTransformTrack(MovieScene, CameraBinding);
	if (!CamTrack) return;

	int32 CurrentFrame = FMath::RoundToInt(CurrentTime * PlaybackFPS);
	FVector CamLoc = SceneActor.Get()->GetActorLocation();
	FRotator CamRot = SceneActor.Get()->GetActorRotation();
	FVector CamScale = SceneActor.Get()->GetActorScale();

	RecordTransformKey(CamTrack, CurrentFrame, CamLoc, CamRot, CamScale);
}

void UUEMotionScene::SetAutoCleanup(bool bCleanup)
{
	bAutoCleanup = bCleanup;
}

bool UUEMotionScene::GetAutoCleanup() const
{
	return bAutoCleanup;
}

void UUEMotionScene::SetBackgroundColor(const FLinearColor& Color)
{
	BackgroundColor = Color;
}

FLinearColor UUEMotionScene::GetBackgroundColor() const
{
	return BackgroundColor;
}

void UUEMotionScene::SetShowCoordinateAxes(bool bShow)
{
	bShowCoordinateAxes = bShow;
}

bool UUEMotionScene::GetShowCoordinateAxes() const
{
	return bShowCoordinateAxes;
}

void UUEMotionScene::SetCoordinateAxisLength(float Length)
{
	CoordinateAxisLength = FMath::Max(Length, 10.0f);
}

float UUEMotionScene::GetCoordinateAxisLength() const
{
	return CoordinateAxisLength;
}

void UUEMotionScene::SetIs2DView(bool b2D)
{
	bIs2DView = b2D;
}

bool UUEMotionScene::Is2DView() const
{
	return bIs2DView;
}

void UUEMotionScene::SetUseUnlit(bool bUnlit)
{
	bUseUnlitMode = bUnlit;
}

bool UUEMotionScene::IsUsingUnlit() const
{
	return bUseUnlitMode;
}

void UUEMotionScene::SetFloorSize(float Size)
{
	FloorSize = FMath::Clamp(Size, 10.0f, 200.0f);
	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Floor size set to %.1f UEMotion units (%.0f x %.0f cm)"),
		FloorSize, FloorSize * 50.0f, FloorSize * 50.0f);
}

float UUEMotionScene::GetFloorSize() const
{
	return FloorSize;
}

void UUEMotionScene::OnRendererFinished(bool bSuccess)
{
	OnRenderFinished.Broadcast(this, bSuccess);
}
