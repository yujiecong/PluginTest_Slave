#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionAnimation.generated.h"

UCLASS(BlueprintType, Abstract)
class UUEMotionAnimation : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetDuration(float InDuration) { Duration = InDuration; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	float GetDuration() const { return Duration; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetEasing(const FString& Type) { EasingType = Type; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	FString GetEasing() const { return EasingType; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	virtual bool IsFinished() const { return Elapsed >= Duration; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	float GetProgress() const { return FMath::Clamp(Elapsed / FMath::Max(Duration, 0.001f), 0.0f, 1.0f); }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void Reset() { Elapsed = 0.0f; }

	void Advance(float DeltaTime)
	{
		Elapsed += DeltaTime;
		float Progress = FMath::Clamp(Elapsed / FMath::Max(Duration, 0.001f), 0.0f, 1.0f);
		float EasedProgress = ApplyEasing(EasingType, Progress);
		TickAnimation(DeltaTime, EasedProgress);
	}

	static float ApplyEasing(const FString& Type, float t);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) {}

	UPROPERTY()
	float Duration = 1.0f;

	UPROPERTY()
	float Elapsed = 0.0f;

	UPROPERTY()
	FString EasingType = TEXT("linear");
};
