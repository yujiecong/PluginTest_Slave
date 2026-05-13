#include "UEMotionScene.h"
#include "Actors/UEMotionAxisActor.h"
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

	UE_LOG(LogTemp, Log,
		TEXT("UEMotionScene: SetupCoordinateAxes() called - bIs2DView=%s, bShowCoordinateAxes=%s"),
		bIs2DView ? TEXT("TRUE") : TEXT("FALSE"),
		bShowCoordinateAxes ? TEXT("TRUE") : TEXT("FALSE"));

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
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: 2D view mode enabled - Z-axis will NOT be created (total axes: %d)"), AxisConfigs.Num());
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Creating %d axis(es) - Config: X=%s, Y=%s, Z=%s"),
		AxisConfigs.Num(),
		TEXT("YES"),
		TEXT("YES"),
		bIs2DView ? TEXT("NO (2D MODE)") : TEXT("YES"));

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

							FString MaterialName;
							if (BPName.EndsWith(TEXT("X")))
								MaterialName = TEXT("M_Axis_X");
							else if (BPName.EndsWith(TEXT("Y")))
								MaterialName = TEXT("M_Axis_Y");
							else if (BPName.EndsWith(TEXT("Z")))
								MaterialName = TEXT("M_Axis_Z");

							if (!MaterialName.IsEmpty())
							{
								FString MaterialPath = FString::Printf(TEXT("/Game/UEMotion/Materials/%s"), *MaterialName);
								UMaterialInterface* AxisMaterial = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);

								if (!AxisMaterial)
								{
									AUEMotionAxisActor* TempActor = SceneWorld.Get()->SpawnActor<AUEMotionAxisActor>(FVector::ZeroVector, FRotator::ZeroRotator);
									if (TempActor)
									{
										TempActor->CreateAxisMaterials();
										AxisMaterial = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
										TempActor->Destroy();
									}
								}

								if (AxisMaterial)
								{
									MeshTemplate->SetMaterial(0, AxisMaterial);
									UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Assigned axis material '%s' to BP_%s mesh template"), *MaterialName, *BPName);
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to load/create axis material '%s' for BP_%s"), *MaterialName, *BPName);
								}
							}
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
				EAxis::Type AxisType;

				if (bIs2DView)
				{
					if (i == 0) AxisType = EAxis::X;
					else if (i == 1) AxisType = EAxis::Y;
					else AxisType = EAxis::None;
				}
				else
				{
					AxisType = static_cast<EAxis::Type>(i);
				}

				AxisActor->InitializeAxis(AxisType, Len, Color);

				UStaticMeshComponent* AxisMesh = AxisActor->GetMeshComponent();
				if (AxisMesh)
				{
					FRotator TargetRotation = FRotator::ZeroRotator;
					switch (AxisType)
					{
					case EAxis::X:
						TargetRotation = FRotator(0, 0, 0);
						break;
					case EAxis::Y:
						TargetRotation = FRotator(0, -90, 0);
						break;
					case EAxis::Z:
						TargetRotation = FRotator(90, 0, 0);
						break;
					default:
						break;
					}

					AxisMesh->SetRelativeRotation(TargetRotation);

					UE_LOG(LogTemp, Log,
						TEXT("UEMotionScene: Setup axis %d (Type=%s) with rotation (%.0f, %.0f, %.0f) - Blueprint material preserved"),
						i,
						(AxisType == EAxis::X) ? TEXT("X") : (AxisType == EAxis::Y) ? TEXT("Y") : (AxisType == EAxis::Z) ? TEXT("Z") : TEXT("NONE"),
						TargetRotation.Pitch, TargetRotation.Yaw, TargetRotation.Roll);
				}
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

		UStaticMeshComponent* FloorMesh = Cast<UStaticMeshComponent>(
			FloorActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));

		if (FloorMesh)
		{
			UMaterialInterface* BlackMaterial = CreateOrLoadBlackMaterial();
			if (BlackMaterial)
			{
				FloorMesh->SetMaterial(0, BlackMaterial);
				UE_LOG(LogTemp, Log,
					TEXT("UEMotionScene: Applied black material to spawned floor mesh"));
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("UEMotionScene: Failed to apply black material - using default"));
			}
		}

		UE_LOG(LogTemp, Log,
			TEXT("UEMotionScene: Spawned black background floor from blueprint uasset (%.0f x %.0f cm) [Size=%.1f UEMotion units]"),
			FloorSizeCM, FloorSizeCM, FloorSize);
	}
	}
}

UMaterialInterface* UUEMotionScene::CreateOrLoadBlackMaterial()
{
	const FString MaterialPath = TEXT("/Game/UEMotion/Materials/M_BlackFloor");

	if (UEditorAssetLibrary::DoesAssetExist(MaterialPath))
	{
		UMaterialInterface* ExistingMat = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
		if (ExistingMat)
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Reusing existing black floor material '%s'"), *MaterialPath);
			return ExistingMat;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Black floor material exists but failed to load, recreating..."));
			UEditorAssetLibrary::DeleteAsset(MaterialPath);
		}
	}

	UPackage* Package = CreatePackage(*MaterialPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to create package for black floor material"));
		return nullptr;
	}

	UMaterialInstanceConstant* BlackMIC = NewObject<UMaterialInstanceConstant>(
		Package, TEXT("M_BlackFloor"), RF_Public | RF_Standalone | RF_Transactional);

	if (!BlackMIC)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to create black floor material instance"));
		return nullptr;
	}

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/EngineMaterials/M_Unlit.M_Unlit"));

	if (!BaseMat)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to load unlit base material 'M_Unlit'"));
		return nullptr;
	}

	BlackMIC->SetParentEditorOnly(BaseMat);

#if WITH_EDITOR
	FLinearColor PureBlack(0.0f, 0.0f, 0.0f, 1.0f);
	BlackMIC->SetVectorParameterValueEditorOnly(FName("BaseColor"), PureBlack);
	BlackMIC->PostEditChange();
#endif

	BlackMIC->SetFlags(RF_Public | RF_Standalone);
	Package->MarkPackageDirty();

	FString FilePath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("UEMotion/Materials/"),
		TEXT("M_BlackFloor.uasset"));

	FSavePackageArgs SaveArgs;
	SaveArgs.SaveFlags = RF_Public | RF_Standalone;

	bool bSaveSuccess = UPackage::SavePackage(
		Package,
		BlackMIC,
		*FilePath,
		SaveArgs);

	if (bSaveSuccess)
	{
		FAssetRegistryModule::AssetCreated(BlackMIC);

		UE_LOG(LogTemp, Log,
			TEXT("UEMotionScene: Created non-reflective black floor material UAsset '%s'")
			TEXT("\n  Parent: M_Unlit (Unlit Shading Model)")
			TEXT("\n  BaseColor: (0, 0, 0, 1) [Pure Black]")
			TEXT("\n  Properties: Zero reflectivity, no lighting calculations")
			TEXT("\n  Type: MaterialInstanceConstant (Static UAsset)"),
			*FilePath);

		UE_LOG(LogTemp, Log,
			TEXT("UEMotionScene: BlackFloor material saved successfully!")
			TEXT("\n  Full Path: %s")
			TEXT("\n  Package: %s"),
			*FilePath, *Package->GetName());

#if WITH_EDITOR
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FString> MaterialPaths;
		MaterialPaths.Add(MaterialPath);
		AssetRegistryModule.Get().ScanPathsSynchronous(MaterialPaths);

		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Force synced asset registry for '%s'"), *MaterialPath);
#endif
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionScene: Failed to save black floor material to '%s'"), *FilePath);

#if WITH_EDITOR
		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: File exists but SavePackage returned false - possible registry issue"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UEMotionScene: File does NOT exist at path: %s"), *FilePath);
		}
#endif
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
