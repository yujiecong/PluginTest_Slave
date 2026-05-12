#include "UEMotionScene.h"
#include "UEMotionMobject.h"
#include "Anim/UEMotionMoveAnimation.h"
#include "Anim/UEMotionRotateAnimation.h"
#include "Anim/UEMotionScaleAnimation.h"
#include "Anim/UEMotionFadeAnimation.h"
#include "Anim/UEMotionColorAnimation.h"
#include "Anim/UEMotionWaitAnimation.h"
#include "Anim/UEMotionGroupAnimation.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Utils/UEMotionSequencerCompat.h"

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
