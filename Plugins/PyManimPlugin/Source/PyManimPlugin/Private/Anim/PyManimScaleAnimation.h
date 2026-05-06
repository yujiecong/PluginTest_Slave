#pragma once

#include "CoreMinimal.h"
#include "Anim/PyManimAnimation.h"
#include "PyManimScaleAnimation.generated.h"

class UPyManimMobject;

UCLASS(BlueprintType)
class UPyManimScaleAnimation : public UPyManimAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetStartScale(const FVector& Scale) { StartScale = Scale; bStartSet = true; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetEndScale(const FVector& Scale) { EndScale = Scale; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetTargetMobject(UPyManimMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UPyManimMobject* TargetMobject = nullptr;

	FVector StartScale = FVector(0.5f);
	FVector EndScale = FVector(2.0f);
	bool bStartSet = false;
};
