#pragma once

#include "CoreMinimal.h"
#include "Anim/PyManimAnimation.h"
#include "PyManimMoveAnimation.generated.h"

class UPyManimMobject;

UCLASS(BlueprintType)
class UPyManimMoveAnimation : public UPyManimAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetTarget(const FVector& InTarget) { End = InTarget; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetStart(const FVector& InStart) { Start = InStart; bStartSet = true; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetTargetMobject(UPyManimMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UPyManimMobject* TargetMobject = nullptr;

	FVector Start = FVector::ZeroVector;
	FVector End = FVector(100, 0, 0);
	bool bStartSet = false;
};
