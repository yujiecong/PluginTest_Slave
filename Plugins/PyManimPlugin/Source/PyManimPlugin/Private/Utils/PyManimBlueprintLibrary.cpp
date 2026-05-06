#include "UEMotionBlueprintLibrary.h"

float UUEMotionBlueprintLibrary::EaseLinear(float T)
{
	return FMath::Clamp(T, 0.0f, 1.0f);
}

float UUEMotionBlueprintLibrary::EaseInQuad(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T;
}

float UUEMotionBlueprintLibrary::EaseOutQuad(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * (2.0f - T);
}

float UUEMotionBlueprintLibrary::EaseInOutQuad(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T < 0.5f) return 2.0f * T * T;
	return -1.0f + (4.0f - 2.0f * T) * T;
}

float UUEMotionBlueprintLibrary::EaseInCubic(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * T;
}

float UUEMotionBlueprintLibrary::EaseOutCubic(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	float T1 = T - 1.0f;
	return T1 * T1 * T1 + 1.0f;
}

float UUEMotionBlueprintLibrary::EaseInOutCubic(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T < 0.5f) return 4.0f * T * T * T;
	float T1 = 2.0f * T - 2.0f;
	return 0.5f * T1 * T1 * T1 + 1.0f;
}

float UUEMotionBlueprintLibrary::EaseInQuart(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * T * T;
}

float UUEMotionBlueprintLibrary::EaseOutQuart(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	float T1 = T - 1.0f;
	return 1.0f - T1 * T1 * T1 * T1;
}

float UUEMotionBlueprintLibrary::EaseInOutQuart(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T < 0.5f) return 8.0f * T * T * T * T;
	float T1 = T - 1.0f;
	return 1.0f - 8.0f * T1 * T1 * T1 * T1;
}

float UUEMotionBlueprintLibrary::EaseInQuint(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * T * T * T;
}

float UUEMotionBlueprintLibrary::EaseOutQuint(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	float T1 = T - 1.0f;
	return 1.0f + T1 * T1 * T1 * T1 * T1;
}

float UUEMotionBlueprintLibrary::EaseInOutQuint(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T < 0.5f) return 16.0f * T * T * T * T * T;
	float T1 = 2.0f * T - 2.0f;
	return 1.0f + 0.5f * T1 * T1 * T1 * T1 * T1;
}

float UUEMotionBlueprintLibrary::EaseInExpo(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T <= 0.0f) return 0.0f;
	return FMath::Pow(2.0f, 10.0f * (T - 1.0f));
}

float UUEMotionBlueprintLibrary::EaseOutExpo(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T >= 1.0f) return 1.0f;
	return 1.0f - FMath::Pow(2.0f, -10.0f * T);
}

float UUEMotionBlueprintLibrary::EaseInOutExpo(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T <= 0.0f) return 0.0f;
	if (T >= 1.0f) return 1.0f;
	if (T < 0.5f) return FMath::Pow(2.0f, 20.0f * T - 10.0f) * 0.5f;
	return (2.0f - FMath::Pow(2.0f, -20.0f * T + 10.0f)) * 0.5f;
}

float UUEMotionBlueprintLibrary::EaseInCirc(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return 1.0f - FMath::Sqrt(1.0f - T * T);
}

float UUEMotionBlueprintLibrary::EaseOutCirc(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	float T1 = T - 1.0f;
	return FMath::Sqrt(1.0f - T1 * T1);
}

float UUEMotionBlueprintLibrary::EaseInOutCirc(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T < 0.5f) return (1.0f - FMath::Sqrt(1.0f - 4.0f * T * T)) * 0.5f;
	float T1 = 2.0f * T - 2.0f;
	return (FMath::Sqrt(1.0f - T1 * T1) + 1.0f) * 0.5f;
}

float UUEMotionBlueprintLibrary::EaseInBack(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	constexpr float S = 1.70158f;
	return T * T * ((S + 1.0f) * T - S);
}

float UUEMotionBlueprintLibrary::EaseOutBack(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	constexpr float S = 1.70158f;
	float T1 = T - 1.0f;
	return T1 * T1 * ((S + 1.0f) * T1 + S) + 1.0f;
}

float UUEMotionBlueprintLibrary::EaseInOutBack(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	constexpr float S = 1.70158f * 1.525f;
	if (T < 0.5f)
	{
		float T1 = 2.0f * T;
		return 0.5f * (T1 * T1 * ((S + 1.0f) * T1 - S));
	}
	float T1 = 2.0f * T - 2.0f;
	return 0.5f * (T1 * T1 * ((S + 1.0f) * T1 + S) + 2.0f);
}

float UUEMotionBlueprintLibrary::EaseOutBounce(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T < 1.0f / 2.75f) return 7.5625f * T * T;
	if (T < 2.0f / 2.75f)
	{
		T -= 1.5f / 2.75f;
		return 7.5625f * T * T + 0.75f;
	}
	if (T < 2.5f / 2.75f)
	{
		T -= 2.25f / 2.75f;
		return 7.5625f * T * T + 0.9375f;
	}
	T -= 2.625f / 2.75f;
	return 7.5625f * T * T + 0.984375f;
}

float UUEMotionBlueprintLibrary::EaseInBounce(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return 1.0f - EaseOutBounce(1.0f - T);
}

float UUEMotionBlueprintLibrary::EaseInOutBounce(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T < 0.5f) return 0.5f * EaseInBounce(T * 2.0f);
	return 0.5f * EaseOutBounce(T * 2.0f - 1.0f) + 0.5f;
}

float UUEMotionBlueprintLibrary::EaseInElastic(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T <= 0.0f) return 0.0f;
	if (T >= 1.0f) return 1.0f;
	return -FMath::Pow(2.0f, 10.0f * T - 10.0f) * FMath::Sin((T * 10.0f - 10.75f) * (2.0f * PI) / 3.0f);
}

float UUEMotionBlueprintLibrary::EaseOutElastic(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T <= 0.0f) return 0.0f;
	if (T >= 1.0f) return 1.0f;
	return FMath::Pow(2.0f, -10.0f * T) * FMath::Sin((T * 10.0f - 0.75f) * (2.0f * PI) / 3.0f) + 1.0f;
}

float UUEMotionBlueprintLibrary::EaseInOutElastic(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	if (T <= 0.0f) return 0.0f;
	if (T >= 1.0f) return 1.0f;
	if (T < 0.5f)
	{
		return -0.5f * FMath::Pow(2.0f, 20.0f * T - 10.0f) * FMath::Sin((20.0f * T - 11.125f) * (2.0f * PI) / 4.5f);
	}
	return FMath::Pow(2.0f, -20.0f * T + 10.0f) * FMath::Sin((20.0f * T - 11.125f) * (2.0f * PI) / 4.5f) * 0.5f + 1.0f;
}

float UUEMotionBlueprintLibrary::ApplyEasingByName(const FString& EasingName, float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);

	if (EasingName == TEXT("linear")) return EaseLinear(T);
	if (EasingName == TEXT("ease_in") || EasingName == TEXT("ease_in_quad")) return EaseInQuad(T);
	if (EasingName == TEXT("ease_out") || EasingName == TEXT("ease_out_quad")) return EaseOutQuad(T);
	if (EasingName == TEXT("ease_in_out") || EasingName == TEXT("ease_in_out_quad")) return EaseInOutQuad(T);
	if (EasingName == TEXT("ease_in_cubic")) return EaseInCubic(T);
	if (EasingName == TEXT("ease_out_cubic")) return EaseOutCubic(T);
	if (EasingName == TEXT("ease_in_out_cubic")) return EaseInOutCubic(T);
	if (EasingName == TEXT("ease_in_quart")) return EaseInQuart(T);
	if (EasingName == TEXT("ease_out_quart")) return EaseOutQuart(T);
	if (EasingName == TEXT("ease_in_out_quart")) return EaseInOutQuart(T);
	if (EasingName == TEXT("ease_in_quint")) return EaseInQuint(T);
	if (EasingName == TEXT("ease_out_quint")) return EaseOutQuint(T);
	if (EasingName == TEXT("ease_in_out_quint")) return EaseInOutQuint(T);
	if (EasingName == TEXT("ease_in_expo")) return EaseInExpo(T);
	if (EasingName == TEXT("ease_out_expo")) return EaseOutExpo(T);
	if (EasingName == TEXT("ease_in_out_expo")) return EaseInOutExpo(T);
	if (EasingName == TEXT("ease_in_circ")) return EaseInCirc(T);
	if (EasingName == TEXT("ease_out_circ")) return EaseOutCirc(T);
	if (EasingName == TEXT("ease_in_out_circ")) return EaseInOutCirc(T);
	if (EasingName == TEXT("ease_in_back")) return EaseInBack(T);
	if (EasingName == TEXT("ease_out_back")) return EaseOutBack(T);
	if (EasingName == TEXT("ease_in_out_back")) return EaseInOutBack(T);
	if (EasingName == TEXT("bounce") || EasingName == TEXT("ease_out_bounce")) return EaseOutBounce(T);
	if (EasingName == TEXT("ease_in_bounce")) return EaseInBounce(T);
	if (EasingName == TEXT("ease_in_out_bounce")) return EaseInOutBounce(T);
	if (EasingName == TEXT("elastic") || EasingName == TEXT("ease_out_elastic")) return EaseOutElastic(T);
	if (EasingName == TEXT("ease_in_elastic")) return EaseInElastic(T);
	if (EasingName == TEXT("ease_in_out_elastic")) return EaseInOutElastic(T);

	return EaseLinear(T);
}

TArray<FString> UUEMotionBlueprintLibrary::GetAllEasingNames()
{
	return {
		TEXT("linear"),
		TEXT("ease_in_quad"), TEXT("ease_out_quad"), TEXT("ease_in_out_quad"),
		TEXT("ease_in_cubic"), TEXT("ease_out_cubic"), TEXT("ease_in_out_cubic"),
		TEXT("ease_in_quart"), TEXT("ease_out_quart"), TEXT("ease_in_out_quart"),
		TEXT("ease_in_quint"), TEXT("ease_out_quint"), TEXT("ease_in_out_quint"),
		TEXT("ease_in_expo"), TEXT("ease_out_expo"), TEXT("ease_in_out_expo"),
		TEXT("ease_in_circ"), TEXT("ease_out_circ"), TEXT("ease_in_out_circ"),
		TEXT("ease_in_back"), TEXT("ease_out_back"), TEXT("ease_in_out_back"),
		TEXT("ease_in_bounce"), TEXT("ease_out_bounce"), TEXT("ease_in_out_bounce"),
		TEXT("ease_in_elastic"), TEXT("ease_out_elastic"), TEXT("ease_in_out_elastic")
	};
}
