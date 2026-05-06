#pragma once

#include "CoreMinimal.h"
#include "Anim/PyManimAnimation.h"
#include "PyManimFadeAnimation.generated.h"

class UPyManimMobject;

UCLASS(BlueprintType)
class UPyManimFadeAnimation : public UPyManimAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetFadeOut(bool bOut) { bFadeOut = bOut; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetTargetMobject(UPyManimMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UPyManimMobject* TargetMobject = nullptr;

	bool bFadeOut = false;
};
