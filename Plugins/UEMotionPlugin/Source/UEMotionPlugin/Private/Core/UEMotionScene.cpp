#include "UEMotionScene.h"
#include "UEMotionCamera.h"
#include "UEMotionMobject.h"
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
#include "PackageTools.h"
#include "FileHelpers.h"
#include "Editor/EditorEngine.h"

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

	UWorld* NewWorld = UEditorLoadingAndSavingUtils::NewMapFromTemplate(TEXT("/Engine/Maps/Templates/Template_Default"), true);
	if (!NewWorld) return false;

	SceneWorld = GEditor->GetEditorWorldContext().World();
	if (!SceneWorld) return false;

	FString MapPath = GetMapPath();
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
		MovieScene->SetPlaybackRange(TRange<FFrameNumber>(0, 1), true);
		MovieScene->SetWorkingRange(0.0, 10.0);
		MovieScene->SetViewRange(0.0, 10.0);
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

		AddActorToSequencer(SceneActor);

		SetupDefaultLighting();
		bInitialized = true;

		UEditorLoadingAndSavingUtils::SaveMap(SceneWorld, GetMapPath());

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

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsSphere(SceneActor, Radius);
		Mobjects.Add(Obj);
		AddActorToSequencer(Obj->GetInternalActor());
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateCube(float Size)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Size <= 0.0f) Size = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cube.Cube"), Size);
		Obj->SetMobjectName(TEXT("Cube"));
		Mobjects.Add(Obj);
		AddActorToSequencer(Obj->GetInternalActor());
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateCylinder(float Radius, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), Radius);
		Obj->SetMobjectName(TEXT("Cylinder"));
		Mobjects.Add(Obj);
		AddActorToSequencer(Obj->GetInternalActor());
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateCone(float Radius, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cone.Cone"), Radius);
		Obj->SetMobjectName(TEXT("Cone"));
		Mobjects.Add(Obj);
		AddActorToSequencer(Obj->GetInternalActor());
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreatePlane(float Width, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Width <= 0.0f) Width = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Plane.Plane"), Width);
		Obj->SetMobjectName(TEXT("Plane"));
		Mobjects.Add(Obj);
		AddActorToSequencer(Obj->GetInternalActor());
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateTorus(float OuterRadius, float InnerRadius)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (OuterRadius <= 0.0f) OuterRadius = 1.0f;
	if (InnerRadius <= 0.0f) InnerRadius = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Torus.Torus"), OuterRadius);
		Obj->SetMobjectName(TEXT("Torus"));
		Mobjects.Add(Obj);
		AddActorToSequencer(Obj->GetInternalActor());
	}
	return Obj;
}

UUEMotionCamera* UUEMotionScene::GetCamera()
{
	return Camera;
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

void UUEMotionScene::AddActorToSequencer(AActor* Actor)
{
	if (!LevelSequence || !Actor) return;

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return;

	UEMotionCompat::FindOrAddPossessable(MovieScene, Actor, LevelSequence, SceneWorld);
}

UMovieScene3DTransformTrack* UUEMotionScene::GetOrCreateTransformTrack(UUEMotionMobject* Mobject)
{
	if (!LevelSequence || !Mobject) return nullptr;

	AActor* Actor = Mobject->GetInternalActor();
	if (!Actor) return nullptr;

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (!MovieScene) return nullptr;

	FGuid ObjectBinding = UEMotionCompat::FindOrAddPossessable(MovieScene, Actor, LevelSequence, SceneWorld);
	if (!ObjectBinding.IsValid()) return nullptr;

	UMovieScene3DTransformTrack* ExistingTrack = UEMotionCompat::FindTransformTrack(MovieScene, ObjectBinding);
	if (ExistingTrack)
	{
		return ExistingTrack;
	}

	UMovieScene3DTransformTrack* NewTrack = Cast<UMovieScene3DTransformTrack>(
		MovieScene->AddTrack<UMovieScene3DTransformTrack>(ObjectBinding));
	if (NewTrack)
	{
		UEMotionCompat::CreateAndAddSection(NewTrack);
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
	if (!Track) return;

	UMovieSceneSection* Section = Track->GetAllSections().Num() > 0 ? Track->GetAllSections()[0] : nullptr;
	if (!Section) return;

	UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(Section);
	if (!TransformSection) return;

	FFrameNumber FrameNumber(Frame);
	UEMotionCompat::RecordTransformKeys(TransformSection, FrameNumber, Location, Rotation, Scale);
	Section->SetRange(TRange<FFrameNumber>::All());
}

void UUEMotionScene::RecordFloatKey(UMovieSceneFloatTrack* Track, int32 Frame, float Value)
{
	if (!Track) return;

	UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->GetAllSections().Num() > 0 ? Track->GetAllSections()[0] : nullptr);
	if (!Section) return;

	FFrameNumber FrameNumber(Frame);
	UEMotionCompat::RecordFloatKey(Section, FrameNumber, Value);
	Section->SetRange(TRange<FFrameNumber>::All());
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
		UUEMotionMobject* FadeTarget = FadeAnim->GetTargetMobject();
		if (FadeTarget)
		{
			UMovieSceneFloatTrack* Track = GetOrCreateFloatTrack(FadeTarget, TEXT("Opacity"));
			if (Track)
			{
				float StartOpacity = FadeAnim->IsFadeOut() ? 1.0f : 0.0f;
				float EndOpacity = FadeAnim->IsFadeOut() ? 0.0f : 1.0f;

				RecordFloatKey(Track, StartFrame, StartOpacity);
				RecordFloatKey(Track, EndFrame, EndOpacity);
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
		MovieScene->SetPlaybackRange(TRange<FFrameNumber>(0, TotalFrames), true);
		MovieScene->SetWorkingRange(0.0, CurrentTime + 1.0);
		MovieScene->SetViewRange(0.0, CurrentTime + 1.0);
	}
}

void UUEMotionScene::Wait(float Duration)
{
	CurrentTime += Duration;

	UMovieScene* MovieScene = LevelSequence ? LevelSequence->GetMovieScene() : nullptr;
	if (MovieScene)
	{
		int32 TotalFrames = FMath::RoundToInt(CurrentTime * PlaybackFPS) + 1;
		MovieScene->SetPlaybackRange(TRange<FFrameNumber>(0, TotalFrames), true);
		MovieScene->SetWorkingRange(0.0, CurrentTime + 1.0);
		MovieScene->SetViewRange(0.0, CurrentTime + 1.0);
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
	UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::RenderFrames: Rendering is temporarily disabled"));
}

void UUEMotionScene::RenderSingleFrame(const FString& FilePath)
{
	UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::RenderSingleFrame: Rendering is temporarily disabled"));
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
		UPackage* SeqPackage = LevelSequence->GetOutermost();
		if (SeqPackage)
		{
			UEMotionCompat::SavePackageToDisk(SeqPackage, TEXT(".uasset"));
		}
	}

	if (SceneWorld)
	{
		FString MapPath = GetMapPath();
		UEditorLoadingAndSavingUtils::SaveMap(SceneWorld, MapPath);
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
	if (Renderer)
	{
		Renderer->OnRenderFinishedDelegate.RemoveDynamic(this, &UUEMotionScene::OnRendererFinished);
	}

	OnRenderFinished.Broadcast(this, bSuccess);
}
