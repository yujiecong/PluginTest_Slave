#include "UEMotionScene.h"
#include "UEMotionCamera.h"
#include "UEMotionMaterialManager.h"
#include "Actors/UEMotionSceneActor.h"
#include "Utils/UEMotionSequencerCompat.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "UnrealEdGlobals.h"
#include "FileHelpers.h"

FString UUEMotionScene::GetMapPath() const
{
	return FString::Printf(TEXT("/Game/UEMotion/Scenes/%s"), *SceneName);
}

FString UUEMotionScene::GetSequencePath() const
{
	return FString::Printf(TEXT("/Game/UEMotion/Sequences/LS_%s"), *SceneName);
}

void UUEMotionScene::Initialize(const FString& InSceneName, int32 Width, int32 Height, bool bInIs2DView)
{
	if (bInitialized) return;

	bIs2DView = bInIs2DView;

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

		MaterialManager = NewObject<UUEMotionMaterialManager>(this);

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
