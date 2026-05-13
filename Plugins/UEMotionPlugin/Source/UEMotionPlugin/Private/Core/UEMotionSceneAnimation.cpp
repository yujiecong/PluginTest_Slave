#include "UEMotionScene.h"
#include "UEMotionMobject.h"
#include "Anim/UEMotionMoveAnimation.h"
#include "Anim/UEMotionRotateAnimation.h"
#include "Anim/UEMotionScaleAnimation.h"
#include "Anim/UEMotionFadeAnimation.h"
#include "Anim/UEMotionColorAnimation.h"
#include "Anim/UEMotionWaitAnimation.h"
#include "Anim/UEMotionGroupAnimation.h"
#include "Anim/UEMotionTransformAnimation.h"
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
	else if (UUEMotionTransformAnimation* TransformAnim = Cast<UUEMotionTransformAnimation>(Animation))
	{
		UUEMotionMobject* SourceMob = TransformAnim->GetSourceMobject();
		UUEMotionMobject* TargetMob = TransformAnim->GetTargetMobject();

		if (SourceMob && TargetMob)
		{
			FVector SourceLoc = SourceMob->GetLocation();
			FLinearColor SourceColor = SourceMob->GetColor();
			FVector SourceScale = SourceMob->GetScale();

			TargetMob->SetLocation(SourceLoc);
			TargetMob->SetColor(SourceColor);
			TargetMob->SetScale(FVector(0.001f));
			TargetMob->SetOpacity(0.0f);

			if (TargetMob->GetInternalActor())
			{
				UMovieScene3DTransformTrack* TargetTransformTrack = GetOrCreateTransformTrack(TargetMob);
				if (TargetTransformTrack)
				{
					FRotator TargetRot = TargetMob->GetRotation();
					RecordTransformKey(TargetTransformTrack, StartFrame, SourceLoc, TargetRot, FVector(0.001f));
					RecordTransformKey(TargetTransformTrack, EndFrame, SourceLoc, TargetRot, SourceScale);
				}

				UMovieSceneFloatTrack* TargetOpacityTrack = GetOrCreateFloatTrack(TargetMob, TEXT("Opacity"));
				if (TargetOpacityTrack)
				{
					RecordFloatKey(TargetOpacityTrack, StartFrame, 0.0f);
					RecordFloatKey(TargetOpacityTrack, EndFrame, 1.0f);
				}
			}

			UE_LOG(LogTemp, Log,
				TEXT("UEMotionScene: Recorded transform keys for '%s' -> '%s' (Frames %d->%d) [Replace+Crossfade]"),
				*SourceMob->GetName(), *TargetMob->GetName(), StartFrame, EndFrame);
		}
	}
	else if (UUEMotionGroupAnimation* GroupAnim = Cast<UUEMotionGroupAnimation>(Animation))
	{
		float GroupStart = CurrentTime;
		for (UUEMotionAnimation* ChildAnim : GroupAnim->GetChildAnimations())
		{
			CurrentTime = GroupStart;
			Play(ChildAnim);
		}
		CurrentTime = GroupStart + Duration;
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

void UUEMotionScene::GenerateShapeBoundaryVertices(UUEMotionMobject* Mobject, TArray<FVector>& OutVertices, int32 Resolution)
{
	if (!Mobject)
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerateShapeBoundaryVertices: Null mobject"));
		return;
	}

	AActor* Actor = Mobject->GetInternalActor();
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerateShapeBoundaryVertices: No internal actor"));
		return;
	}

	FVector Origin, BoxExtent;
	Mobject->GetBounds(Origin, BoxExtent);

	FString MobName = Mobject->GetName();
	float MaxDim = FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
	if (FMath::IsNearlyZero(MaxDim)) MaxDim = 50.0f;

	OutVertices.SetNum(Resolution);

	if (MobName.Contains(TEXT("Sphere"), ESearchCase::IgnoreCase))
	{
		for (int32 i = 0; i < Resolution; i++)
		{
			float Angle = (float)i / (float)Resolution * UE_TWO_PI;
			OutVertices[i] = FVector(
				FMath::Cos(Angle) * MaxDim,
				FMath::Sin(Angle) * MaxDim,
				0.0f
			);
		}
	}
	else if (MobName.Contains(TEXT("Cube"), ESearchCase::IgnoreCase) ||
			 MobName.Contains(TEXT("Square"), ESearchCase::IgnoreCase))
	{
		int32 SideVerts = Resolution / 4;
		for (int32 i = 0; i < Resolution; i++)
		{
			int32 Side = i / SideVerts;
			float t = (float)(i % SideVerts) / (float)SideVerts;

			switch (Side)
			{
			case 0:
				OutVertices[i] = FVector(
					FMath::Lerp(MaxDim, -MaxDim, t),
					MaxDim,
					0.0f
				);
				break;
			case 1:
				OutVertices[i] = FVector(
					-MaxDim,
					FMath::Lerp(MaxDim, -MaxDim, t),
					0.0f
				);
				break;
			case 2:
				OutVertices[i] = FVector(
					FMath::Lerp(-MaxDim, MaxDim, t),
					-MaxDim,
					0.0f
				);
				break;
			default:
				OutVertices[i] = FVector(
					MaxDim,
					FMath::Lerp(-MaxDim, MaxDim, t),
					0.0f
				);
				break;
			}
		}
	}
	else if (MobName.Contains(TEXT("Cylinder"), ESearchCase::IgnoreCase) ||
			 MobName.Contains(TEXT("Circle"), ESearchCase::IgnoreCase) ||
			 MobName.Contains(TEXT("Torus"), ESearchCase::IgnoreCase))
	{
		for (int32 i = 0; i < Resolution; i++)
		{
			float Angle = (float)i / (float)Resolution * UE_TWO_PI;
			OutVertices[i] = FVector(
				FMath::Cos(Angle) * MaxDim,
				FMath::Sin(Angle) * MaxDim,
				0.0f
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log,
			TEXT("GenerateShapeBoundaryVertices: Unknown shape '%s', using circle approximation"),
			*MobName);

		for (int32 i = 0; i < Resolution; i++)
		{
			float Angle = (float)i / (float)Resolution * UE_TWO_PI;
			OutVertices[i] = FVector(
				FMath::Cos(Angle) * MaxDim,
				FMath::Sin(Angle) * MaxDim,
				0.0f
			);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("GenerateShapeBoundaryVertices: Generated %d vertices for '%s' (MaxDim=%.2f)"),
		Resolution, *MobName, MaxDim);
}
