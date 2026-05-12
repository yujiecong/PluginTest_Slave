#include "UEMotionScene.h"
#include "UEMotionMobject.h"
#include "Actors/UEMotionSceneActor.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlayer.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Utils/UEMotionSequencerCompat.h"

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
