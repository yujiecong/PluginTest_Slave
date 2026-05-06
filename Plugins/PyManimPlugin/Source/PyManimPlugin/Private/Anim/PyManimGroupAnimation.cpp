#include "PyManimGroupAnimation.h"

void UPyManimGroupAnimation::AddAnimation(UPyManimAnimation* Animation)
{
	if (!Animation) return;
	Animations.Add(Animation);

	if (bIsSequential)
	{
		float TotalDuration = 0.0f;
		for (UPyManimAnimation* Anim : Animations)
		{
			TotalDuration += Anim->GetDuration();
		}
		Duration = TotalDuration;
	}
	else
	{
		float MaxDuration = 0.0f;
		for (UPyManimAnimation* Anim : Animations)
		{
			MaxDuration = FMath::Max(MaxDuration, Anim->GetDuration());
		}
		Duration = MaxDuration;
	}
}

void UPyManimGroupAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (bIsSequential)
	{
		SequentialElapsed += DeltaTime;
		float Accumulated = 0.0f;
		for (int32 i = 0; i < Animations.Num(); ++i)
		{
			UPyManimAnimation* Anim = Animations[i];
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
		for (UPyManimAnimation* Anim : Animations)
		{
			if (Anim && !Anim->IsFinished())
			{
				Anim->Advance(DeltaTime);
			}
		}
	}
}
