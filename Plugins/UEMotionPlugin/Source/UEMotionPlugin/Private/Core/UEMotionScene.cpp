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
#include "Utils/UEMotionSequencerCompat.h"
#include "UEMotionAssetFactory.h"
#include "Materials/MaterialParameterCollection.h"
#include "Tracks/MovieSceneMaterialParameterCollectionTrack.h"
#include "Sections/MovieSceneParameterSection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlayer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PackageTools.h"
#include "FileHelpers.h"
#include "HAL/PlatformFileManager.h"
#include "Editor/EditorEngine.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Sections/MovieSceneCameraCutSection.h"

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

	FString MapPath = GetMapPath();

	if (UEditorAssetLibrary::DoesAssetExist(MapPath))
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Map '%s' already exists, deleting it"), *MapPath);
		UEditorAssetLibrary::DeleteAsset(MapPath);
	}

	UWorld* NewWorld = UEditorLoadingAndSavingUtils::NewMapFromTemplate(TEXT("/Engine/Maps/Templates/Template_Default"), true);
	if (!NewWorld) return false;

	SceneWorld = GEditor->GetEditorWorldContext().World();
	if (!SceneWorld) return false;

	if (!UEditorLoadingAndSavingUtils::SaveMap(SceneWorld, MapPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to save map to '%s'"), *MapPath);
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Map created and saved to '%s'"), *MapPath);
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
		int32 DefaultFrames = FMath::RoundToInt(2.0f * PlaybackFPS);
		FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
		FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, DefaultFrames);
		MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartTick, EndTick), true);
		float EndTime = (float)DefaultFrames / PlaybackFPS;
		MovieScene->SetWorkingRange(0.0, EndTime);
		MovieScene->SetViewRange(0.0, EndTime);
	}

	FAssetRegistryModule::AssetCreated(NewSequence);
	UEMotionCompat::MarkPackageDirty(SequencePackage);
	UEMotionCompat::SavePackageToDisk(SequencePackage, TEXT(".uasset"));

	LevelSequence = NewSequence;

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene: LevelSequence created and saved to '%s'"), *SequencePackageName);
	return true;
}

void UUEMotionScene::SetupDefaultLighting()
{
	if (!SceneWorld) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADirectionalLight* DirLight = SceneWorld->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0, 0, 500), FRotator(-45, 0, 0), SpawnParams);
	if (DirLight)
	{
		UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(DirLight->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(10.0f);
			LightComp->SetLightColor(FLinearColor::White);
		}
	}
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

	SceneActor = SceneWorld->SpawnActor<AUEMotionSceneActor>(AUEMotionSceneActor::StaticClass(), Location, Rotation, SpawnParams);

	if (SceneActor)
	{
		SceneActor->SetOwnerScene(this);

		Camera = NewObject<UUEMotionCamera>(this);
		Camera->Init(SceneActor);
		Camera->LookAt(FVector(0, 0, 0));

		FGuid CameraBinding = AddActorToSequencer(SceneActor);

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

					FVector CamLoc = SceneActor->GetActorLocation();
					FRotator CamRot = SceneActor->GetActorRotation();
					FVector CamScale = SceneActor->GetActorScale();
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
		bInitialized = true;

		UEditorLoadingAndSavingUtils::SaveMap(SceneWorld, GetMapPath());

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
	return SceneWorld;
}

ULevelSequence* UUEMotionScene::GetLevelSequence() const
{
	return LevelSequence;
}

UUEMotionMobject* UUEMotionScene::CreateSphere(float Radius)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	return CreateMobjectFromParams(TEXT("sphere"), Radius);
}

UUEMotionMobject* UUEMotionScene::CreateCube(float Size)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Size <= 0.0f) Size = 1.0f;
	return CreateMobjectFromParams(TEXT("cube"), Size);
}

UUEMotionMobject* UUEMotionScene::CreateCylinder(float Radius, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;
	return CreateMobjectFromParams(TEXT("cylinder"), Radius);
}

UUEMotionMobject* UUEMotionScene::CreateCone(float Radius, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;
	return CreateMobjectFromParams(TEXT("cone"), Radius);
}

UUEMotionMobject* UUEMotionScene::CreatePlane(float Width, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Width <= 0.0f) Width = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;
	return CreateMobjectFromParams(TEXT("plane"), Width);
}

UUEMotionMobject* UUEMotionScene::CreateTorus(float OuterRadius, float InnerRadius)
{
	if (!bInitialized || !SceneActor) return nullptr;
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
	if (!bInitialized || !SceneActor) return nullptr;

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
	if (!bInitialized || !SceneActor) return nullptr;

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
	if (!bInitialized || !SceneWorld) return;

	ADirectionalLight* Light = SceneWorld->SpawnActor<ADirectionalLight>();
	if (Light)
	{
		Light->SetActorRotation(Direction.Rotation());
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
	if (!bInitialized || !SceneWorld) return;

	APointLight* Light = SceneWorld->SpawnActor<APointLight>();
	if (Light)
	{
		Light->SetActorLocation(Location);
		UPointLightComponent* LightComp = Cast<UPointLightComponent>(Light->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(Intensity);
			LightComp->SetLightColor(Color);
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

	FGuid Binding = UEMotionCompat::FindOrAddPossessable(MovieScene, Actor, LevelSequence, SceneWorld);
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

	FGuid ObjectBinding = UEMotionCompat::FindOrAddPossessable(MovieScene, Actor, LevelSequence, SceneWorld);
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

	FGuid ObjectBinding = UEMotionCompat::FindOrAddPossessable(MovieScene, Actor, LevelSequence, SceneWorld);
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
	if (!LevelSequence || !SceneActor) return;
	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return;

	FGuid CameraBinding = UEMotionCompat::FindObjectBinding(MovieScene, SceneActor);
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
		static const FString MPCPath = TEXT("/Game/UEMotion/Materials/MPC_UEMotionFade");
		UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, *MPCPath);
		if (MPC)
		{
			UMovieScene* MovieScene = LevelSequence->GetMovieScene();
			if (MovieScene)
			{
				UMovieSceneMaterialParameterCollectionTrack* MPCTrack = nullptr;

				for (UMovieSceneTrack* Track : MovieScene->GetTracks())
				{
					UMovieSceneMaterialParameterCollectionTrack* CandidateTrack =
						Cast<UMovieSceneMaterialParameterCollectionTrack>(Track);
					if (CandidateTrack && CandidateTrack->MPC == MPC)
					{
						MPCTrack = CandidateTrack;
						break;
					}
				}

				if (!MPCTrack)
				{
					MPCTrack = MovieScene->AddTrack<UMovieSceneMaterialParameterCollectionTrack>();
					if (MPCTrack)
					{
						MPCTrack->MPC = MPC;
					}
				}

				if (MPCTrack)
				{
					UMovieSceneParameterSection* Section = Cast<UMovieSceneParameterSection>(
						MPCTrack->GetAllSections().Num() > 0 ? MPCTrack->GetAllSections()[0] : nullptr);
					if (!Section)
					{
						Section = Cast<UMovieSceneParameterSection>(MPCTrack->CreateNewSection());
						if (Section)
						{
							Section->SetRange(TRange<FFrameNumber>::All());
							MPCTrack->AddSection(*Section);
						}
					}

					if (Section)
					{
						float StartOpacity = FadeAnim->IsFadeOut() ? 1.0f : 0.0f;
						float EndOpacity = FadeAnim->IsFadeOut() ? 0.0f : 1.0f;

						FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, StartFrame);
						FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, EndFrame);

						Section->AddScalarParameterKey(FName("Opacity"), StartTick, StartOpacity);
						Section->AddScalarParameterKey(FName("Opacity"), EndTick, EndOpacity);

						TRange<FFrameNumber> CurrentRange = Section->GetRange();
						FFrameNumber MinTick = CurrentRange.GetLowerBound().IsOpen() ? StartTick :
							FMath::Min(CurrentRange.GetLowerBoundValue(), StartTick);
						FFrameNumber MaxTick = CurrentRange.GetUpperBound().IsOpen() ? EndTick :
							FMath::Max(CurrentRange.GetUpperBoundValue(), EndTick);
						Section->SetRange(TRange<FFrameNumber>(MinTick, MaxTick));

						UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Recorded MPC fade keys - Start=%f Frame=%d End=%f Frame=%d"),
							StartOpacity, StartFrame, EndOpacity, EndFrame);
					}
				}
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

void UUEMotionScene::Tick(float DeltaTime)
{
	if (!bInitialized) return;
	if (DeltaTime <= 0.0f) return;

	for (int32 i = ActiveAnimations.Num() - 1; i >= 0; --i)
	{
		UUEMotionAnimation* Anim = ActiveAnimations[i];
		if (!Anim) continue;
		Anim->Advance(DeltaTime);
		if (Anim->IsFinished())
		{
			ActiveAnimations.RemoveAt(i);
		}
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

void UUEMotionScene::RenderFrames(const FString& OutputDirectory, float Duration, float FPS)
{
	if (!bInitialized || !LevelSequence) return;

	if (!Renderer)
	{
		Renderer = NewObject<UUEMotionRenderer>(this);
		Renderer->Initialize(SceneWorld, ResolutionWidth, ResolutionHeight);
		Renderer->OnRenderFinishedDelegate.AddDynamic(this, &UUEMotionScene::OnRendererFinished);
	}

	Renderer->BindCamera(SceneActor);

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

	if (SceneActor)
	{
		SceneActor->Destroy();
		SceneActor = nullptr;
	}

	if (Renderer)
	{
		Renderer->OnRenderFinishedDelegate.RemoveAll(this);
		Renderer = nullptr;
	}

	Camera = nullptr;
	LevelSequence = nullptr;
	SceneWorld = nullptr;
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

	if (SceneWorld)
	{
		FString MapPath = GetMapPath();
		if (UEditorLoadingAndSavingUtils::SaveMap(SceneWorld, MapPath))
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
	if (!LevelSequence || !SceneActor || !bInitialized) return;

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return;

	FGuid CameraBinding = UEMotionCompat::FindObjectBinding(MovieScene, SceneActor);
	if (!CameraBinding.IsValid()) return;

	UMovieScene3DTransformTrack* CamTrack = UEMotionCompat::FindTransformTrack(MovieScene, CameraBinding);
	if (!CamTrack) return;

	int32 CurrentFrame = FMath::RoundToInt(CurrentTime * PlaybackFPS);
	FVector CamLoc = SceneActor->GetActorLocation();
	FRotator CamRot = SceneActor->GetActorRotation();
	FVector CamScale = SceneActor->GetActorScale();

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

void UUEMotionScene::OnRendererFinished(bool bSuccess)
{
	OnRenderFinished.Broadcast(this, bSuccess);
}
