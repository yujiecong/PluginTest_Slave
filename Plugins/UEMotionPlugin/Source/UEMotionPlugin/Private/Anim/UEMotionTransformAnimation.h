#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionTransformAnimation.generated.h"

class UUEMotionMobject;

UCLASS(BlueprintType)
class UUEMotionTransformAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation|Transform")
	void SetSourceMobject(UUEMotionMobject* Source);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation|Transform")
	UUEMotionMobject* GetSourceMobject() const { return SourceMobject; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation|Transform")
	void SetTargetMobject(UUEMotionMobject* Target);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation|Transform")
	UUEMotionMobject* GetTargetMobject() const { return TargetMobject; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation|Transform")
	float GetMorphProgress() const { return MorphProgress; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation|Transform")
	void SetMorphResolution(int32 InResolution) { MorphResolution = InResolution; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation|Transform")
	int32 GetMorphResolution() const { return MorphResolution; }

	virtual void Reset() override;

protected:
	virtual void UpdateAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UUEMotionMobject* SourceMobject = nullptr;

	UPROPERTY()
	UUEMotionMobject* TargetMobject = nullptr;

	float MorphProgress = 0.0f;

	int32 MorphResolution = 64;
};
