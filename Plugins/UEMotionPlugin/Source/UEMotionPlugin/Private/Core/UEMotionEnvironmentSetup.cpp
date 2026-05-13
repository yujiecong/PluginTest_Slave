#include "UEMotionEnvironmentSetup.h"
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

void UUEMotionEnvironmentSetup::Initialize(UWorld* InWorld, UUEMotionMaterialManager* InMaterialManager)
{
	World = InWorld;
	MaterialManager = InMaterialManager;
}

void UUEMotionEnvironmentSetup::SetupDefaultLighting(bool bUseUnlitMode)
{
	if (!World.IsValid()) return;

	if (bUseUnlitMode)
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Skipping default lighting (Unlit mode)"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADirectionalLight* DirLight = World.Get()->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(),
		FVector(300, -300, 600),
		FRotator(-55, 45, 0),
		SpawnParams
	);

	if (DirLight)
	{
		UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(DirLight->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(0.0f);
			LightComp->SetLightColor(FLinearColor::Black);

			UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Directional light set to zero intensity for dark background"));
		}
	}
}

void UUEMotionEnvironmentSetup::SetupCoordinateAxes(bool bShowAxes, float AxisLength, bool bIs2DView)
{
	if (!World.IsValid() || !bShowAxes) return;

	UE_LOG(LogTemp, Log,
		TEXT("UEMotionEnvironment: SetupCoordinateAxes() called - bIs2DView=%s, bShowAxes=%s"),
		bIs2DView ? TEXT("TRUE") : TEXT("FALSE"),
		bShowAxes ? TEXT("TRUE") : TEXT("FALSE"));

	AUEMotionAxisActor::CreateAxisMaterials();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;

	float Len = AxisLength;

	TArray<TPair<FString, FLinearColor>> AxisConfigs;
	AxisConfigs.Add(TPair<FString, FLinearColor>(TEXT("BP_Axis_X"), FLinearColor(1.0f, 0.25f, 0.25f, 1.0f)));
	AxisConfigs.Add(TPair<FString, FLinearColor>(TEXT("BP_Axis_Y"), FLinearColor(0.25f, 1.0f, 0.25f, 1.0f)));

	if (!bIs2DView)
	{
		AxisConfigs.Add(TPair<FString, FLinearColor>(TEXT("BP_Axis_Z"), FLinearColor(0.25f, 0.5f, 1.0f, 1.0f)));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: 2D view mode enabled - Z-axis will NOT be created (total axes: %d)"), AxisConfigs.Num());
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Creating %d axis(es) - Config: X=%s, Y=%s, Z=%s"),
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
				UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Reusing existing axis blueprint '%s'"), *BlueprintPath);
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

						USCS_Node* MeshNode = SCS->CreateNode(UProceduralMeshComponent::StaticClass(), TEXT("LineMesh"));
						RootNode->AddChildNode(MeshNode);

						UProceduralMeshComponent* MeshTemplate = Cast<UProceduralMeshComponent>(MeshNode->ComponentTemplate);
						if (MeshTemplate)
						{
							MeshTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
							MeshTemplate->CastShadow = false;
							MeshTemplate->bUseComplexAsSimpleCollision = false;

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
									AUEMotionAxisActor* TempActor = World.Get()->SpawnActor<AUEMotionAxisActor>(FVector::ZeroVector, FRotator::ZeroRotator);
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
									UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Assigned axis material '%s' to BP_%s line mesh template"), *MaterialName, *BPName);
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("UEMotionEnvironment: Failed to load/create axis material '%s' for BP_%s"), *MaterialName, *BPName);
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
					UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Created axis blueprint '%s' with SCS configuration"), *BlueprintPath);
				}
			}
		}

		if (AxisClass)
		{
			AActor* SpawnedActor = World.Get()->SpawnActor<AActor>(AxisClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
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

				UProceduralMeshComponent* AxisMesh = AxisActor->GetMeshComponent();
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
						TEXT("UEMotionEnvironment: Setup axis %d (Type=%s) with rotation (%.0f, %.0f, %.0f) - Procedural line mesh"),
						i,
						(AxisType == EAxis::X) ? TEXT("X") : (AxisType == EAxis::Y) ? TEXT("Y") : (AxisType == EAxis::Z) ? TEXT("Z") : TEXT("NONE"),
						TargetRotation.Pitch, TargetRotation.Yaw, TargetRotation.Roll);
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Coordinate axes created from blueprints (X=Red, Y=Green, Z=Blue, Length=%.0f)"), Len);
}

void UUEMotionEnvironmentSetup::SetupSkyEnvironment()
{
	if (!World.IsValid()) return;

	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector(0, 0, 1000), FRotator::ZeroRotator);
	if (SkyLight)
	{
		USkyLightComponent* SkyComp = Cast<USkyLightComponent>(SkyLight->GetComponentByClass(USkyLightComponent::StaticClass()));
		if (SkyComp)
		{
			SkyComp->SetIntensity(0.0f);
			SkyComp->SetRealTimeCaptureEnabled(false);
			SkyComp->SetMobility(EComponentMobility::Movable);

			UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Created SkyLight with zero intensity for dark background"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Sky environment setup complete (SkyLight only, no Atmosphere/Sphere)"));
}

void UUEMotionEnvironmentSetup::SetupBlackBackgroundFloor(float FloorSize, UUEMotionMaterialManager* InMaterialManager)
{
	if (!World.IsValid()) return;

	UUEMotionMaterialManager* MatMgr = InMaterialManager ? InMaterialManager : MaterialManager;

	FString FloorBlueprintPath = TEXT("/Game/UEMotion/Blueprints/BP_BlackFloor");
	UClass* FloorClass = nullptr;

	if (UEditorAssetLibrary::DoesAssetExist(FloorBlueprintPath))
	{
		UBlueprint* ExistingBP = LoadObject<UBlueprint>(nullptr, *FloorBlueprintPath);
		if (ExistingBP && ExistingBP->GeneratedClass)
		{
			FloorClass = ExistingBP->GeneratedClass;
			UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Reusing existing floor blueprint '%s'"), *FloorBlueprintPath);
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

							UMaterialInterface* BlackMaterial = nullptr;
							if (MatMgr)
							{
								BlackMaterial = MatMgr->GetOrCreateBlackFloorMaterial();
							}
							if (BlackMaterial)
							{
								MeshTemplate->SetMaterial(0, BlackMaterial);
								UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Black material applied to floor mesh template"));
							}
							else
							{
								UE_LOG(LogTemp, Warning, TEXT("UEMotionEnvironment: Failed to create/load black material for floor"));
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
				UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Created black floor blueprint '%s' with SCS configuration"), *FloorBlueprintPath);
			}
		}
	}

	if (FloorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bNoFail = true;

		AActor* FloorActor = World->SpawnActor<AActor>(
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
				UMaterialInterface* BlackMaterial = nullptr;
				if (MatMgr)
				{
					BlackMaterial = MatMgr->GetOrCreateBlackFloorMaterial();
				}
				if (BlackMaterial)
				{
					FloorMesh->SetMaterial(0, BlackMaterial);
					UE_LOG(LogTemp, Log,
						TEXT("UEMotionEnvironment: Applied black material to spawned floor mesh"));
				}
				else
				{
					UE_LOG(LogTemp, Warning,
						TEXT("UEMotionEnvironment: Failed to apply black material - using default"));
				}
			}

			UE_LOG(LogTemp, Log,
				TEXT("UEMotionEnvironment: Spawned black background floor from blueprint uasset (%.0f x %.0f cm) [Size=%.1f UEMotion units]"),
				FloorSizeCM, FloorSizeCM, FloorSize);
		}
	}
}

void UUEMotionEnvironmentSetup::AddDirectionalLight(const FVector& Direction, const FLinearColor& Color, float Intensity)
{
	if (!World.IsValid()) return;

	ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(),
		FVector(300, -300, 600),
		Direction.Rotation()
	);

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

void UUEMotionEnvironmentSetup::AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity)
{
	if (!World.IsValid()) return;

	APointLight* PointLight = World->SpawnActor<APointLight>(
		APointLight::StaticClass(),
		Location,
		FRotator::ZeroRotator
	);

	if (PointLight)
	{
		UPointLightComponent* LightComp = Cast<UPointLightComponent>(PointLight->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(Intensity);
			LightComp->SetLightColor(Color);

			UE_LOG(LogTemp, Log, TEXT("UEMotionEnvironment: Added point light at (%.1f, %.1f, %.1f) with intensity=%.1f"),
				Location.X, Location.Y, Location.Z, Intensity);
		}
	}
}
