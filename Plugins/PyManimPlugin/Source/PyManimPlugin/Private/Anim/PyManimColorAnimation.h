#pragma once

#include "CoreMinimal.h"
#include "Anim/PyManimAnimation.h"
#include "PyManimColorAnimation.generated.h"

class UPyManimMobject;

UCLASS(BlueprintType)
class UPyManimColorAnimation : public UPyManimAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetStartColor(const FLinearColor& Color) { StartColor = Color; bStartSet = true; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetEndColor(const FLinearColor& Color) { EndColor = Color; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetTargetMobject(UPyManimMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UPyManimMobject* TargetMobject = nullptr;

	FLinearColor StartColor = FLinearColor::White;
	FLinearColor EndColor = FLinearColor::Blue;
	bool bStartSet = false;
};
