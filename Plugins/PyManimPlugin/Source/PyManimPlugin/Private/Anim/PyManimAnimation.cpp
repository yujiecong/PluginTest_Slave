#include "PyManimAnimation.h"

float UPyManimAnimation::ApplyEasing(const FString& Type, float t)
{
	t = FMath::Clamp(t, 0.0f, 1.0f);

	if (Type == TEXT("linear"))
		return t;
	if (Type == TEXT("ease_in") || Type == TEXT("ease_in_quad"))
		return t * t;
	if (Type == TEXT("ease_out") || Type == TEXT("ease_out_quad"))
		return t * (2.0f - t);
	if (Type == TEXT("ease_in_out") || Type == TEXT("ease_in_out_quad"))
	{
		if (t < 0.5f)
			return 2.0f * t * t;
		return -1.0f + (4.0f - 2.0f * t) * t;
	}
	if (Type == TEXT("ease_in_cubic"))
		return t * t * t;
	if (Type == TEXT("ease_out_cubic"))
	{
		float t1 = t - 1.0f;
		return t1 * t1 * t1 + 1.0f;
	}
	if (Type == TEXT("ease_in_out_cubic"))
	{
		if (t < 0.5f)
			return 4.0f * t * t * t;
		float t1 = 2.0f * t - 2.0f;
		return 0.5f * t1 * t1 * t1 + 1.0f;
	}
	if (Type == TEXT("bounce"))
	{
		if (t < 1.0f / 2.75f)
			return 7.5625f * t * t;
		if (t < 2.0f / 2.75f)
		{
			t -= 1.5f / 2.75f;
			return 7.5625f * t * t + 0.75f;
		}
		if (t < 2.5f / 2.75f)
		{
			t -= 2.25f / 2.75f;
			return 7.5625f * t * t + 0.9375f;
		}
		t -= 2.625f / 2.75f;
		return 7.5625f * t * t + 0.984375f;
	}

	return t;
}
