#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionColorAnimation.generated.h"

class UUEMotionMobject;

UCLASS(BlueprintType)
class UUEMotionColorAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetStartColor(const FLinearColor& Color) { StartColor = Color; bStartSet = true; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetEndColor(const FLinearColor& Color) { EndColor = Color; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetTargetMobject(UUEMotionMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UUEMotionMobject* TargetMobject = nullptr;

	FLinearColor StartColor = FLinearColor::White;
	FLinearColor EndColor = FLinearColor::Blue;
	bool bStartSet = false;
};
