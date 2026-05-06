#include "UEMotionGroupAnimation.h"

bool UUEMotionGroupAnimation::IsFinished() const
{
	if (Animations.Num() == 0)
	{
		return true;
	}

	if (bIsSequential)
	{
		for (const UUEMotionAnimation* Anim : Animations)
		{
			if (Anim && !Anim->IsFinished())
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		for (const UUEMotionAnimation* Anim : Animations)
		{
			if (Anim && !Anim->IsFinished())
			{
				return false;
			}
		}
		return true;
	}
}

void UUEMotionGroupAnimation::AddAnimation(UUEMotionAnimation* Animation)
{
	if (!Animation) return;
	Animations.Add(Animation);

	if (bIsSequential)
	{
		float TotalDuration = 0.0f;
		for (UUEMotionAnimation* Anim : Animations)
		{
			TotalDuration += Anim->GetDuration();
		}
		Duration = TotalDuration;
	}
	else
	{
		float MaxDuration = 0.0f;
		for (UUEMotionAnimation* Anim : Animations)
		{
			MaxDuration = FMath::Max(MaxDuration, Anim->GetDuration());
		}
		Duration = MaxDuration;
	}
}

void UUEMotionGroupAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (bIsSequential)
	{
		SequentialElapsed += DeltaTime;
		float Accumulated = 0.0f;
		for (int32 i = 0; i < Animations.Num(); ++i)
		{
			UUEMotionAnimation* Anim = Animations[i];
			if (!Anim) continue;

			float AnimDuration = Anim->GetDuration();
			if (SequentialElapsed >= Accumulated + AnimDuration)
			{
				if (!Anim->IsFinished())
				{
					Anim->Advance(AnimDuration - (Accumulated + AnimDuration - SequentialElapsed + DeltaTime));
				}
			}
			else if (SequentialElapsed > Accumulated)
			{
				float AnimDelta = SequentialElapsed - Accumulated;
				if (AnimDelta > DeltaTime)
				{
					AnimDelta = DeltaTime;
				}
				Anim->Advance(AnimDelta);
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
