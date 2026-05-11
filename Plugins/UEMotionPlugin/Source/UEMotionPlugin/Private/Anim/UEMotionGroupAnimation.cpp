#include "UEMotionGroupAnimation.h"

void UUEMotionGroupAnimation::AddAnimation(UUEMotionAnimation* Animation)
{
	if (!Animation) return;
	Animations.Add(Animation);
	RecalculateDuration();
}

void UUEMotionGroupAnimation::SetPlayMode(bool bSequential)
{
	bIsSequential = bSequential;
	RecalculateDuration();
}

void UUEMotionGroupAnimation::RecalculateDuration()
{
	if (bIsSequential)
	{
		float TotalDuration = 0.0f;
		for (UUEMotionAnimation* Anim : Animations)
		{
			if (Anim) TotalDuration += Anim->GetDuration();
		}
		SetDuration(TotalDuration);
	}
	else
	{
		float MaxDuration = 0.0f;
		for (UUEMotionAnimation* Anim : Animations)
		{
			if (Anim && Anim->GetDuration() > MaxDuration)
			{
				MaxDuration = Anim->GetDuration();
			}
		}
		SetDuration(MaxDuration);
	}
}

void UUEMotionGroupAnimation::Reset()
{
	Super::Reset();
	for (UUEMotionAnimation* Anim : Animations)
	{
		if (Anim)
		{
			Anim->Reset();
		}
	}
}

void UUEMotionGroupAnimation::UpdateAnimation(float DeltaTime, float EasedProgress)
{
	if (bIsSequential)
	{
		float GroupElapsed = Elapsed;
		float Accumulated = 0.0f;

		for (int32 i = 0; i < Animations.Num(); ++i)
		{
			UUEMotionAnimation* Anim = Animations[i];
			if (!Anim) continue;

			float AnimDuration = Anim->GetDuration();
			float AnimStartTime = Accumulated;

			if (GroupElapsed < AnimStartTime)
			{
				break;
			}

			float AnimElapsed = FMath::Min(GroupElapsed - AnimStartTime, AnimDuration);
			float CurrentAnimElapsed = Anim->GetProgress() * AnimDuration;
			float DeltaToAdvance = AnimElapsed - CurrentAnimElapsed;

			if (DeltaToAdvance > 0.0f)
			{
				Anim->Advance(DeltaToAdvance);
			}

			Accumulated += AnimDuration;
		}
	}
	else
	{
		for (UUEMotionAnimation* Anim : Animations)
		{
			if (Anim && !Anim->IsFinished())
			{
				Anim->Advance(DeltaTime);
			}
		}
	}
}
