#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UEMotionBlueprintLibrary.generated.h"

UCLASS()
class UUEMotionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseLinear(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInQuad(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutQuad(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutQuad(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInCubic(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutCubic(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutCubic(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInQuart(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutQuart(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutQuart(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInQuint(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutQuint(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutQuint(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInExpo(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutExpo(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutExpo(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInCirc(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutCirc(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutCirc(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInBack(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutBack(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutBack(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutBounce(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInBounce(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutBounce(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInElastic(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseOutElastic(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float EaseInOutElastic(float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static float ApplyEasingByName(const FString& EasingName, float T);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Easing")
	static TArray<FString> GetAllEasingNames();
};
