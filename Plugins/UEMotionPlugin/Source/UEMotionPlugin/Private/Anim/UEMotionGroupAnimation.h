#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionGroupAnimation.generated.h"

UCLASS(BlueprintType)
class UUEMotionGroupAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void AddAnimation(UUEMotionAnimation* Animation);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetPlayMode(bool bSequential);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	const TArray<UUEMotionAnimation*>& GetAnimations() const { return Animations; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	int32 GetAnimationCount() const { return Animations.Num(); }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	const TArray<UUEMotionAnimation*>& GetChildAnimations() const { return Animations; }

	virtual void Reset() override;

protected:
	virtual void UpdateAnimation(float DeltaTime, float EasedProgress) override;

private:
	void RecalculateDuration();
	UPROPERTY()
	TArray<UUEMotionAnimation*> Animations;

	bool bIsSequential = false;
};
