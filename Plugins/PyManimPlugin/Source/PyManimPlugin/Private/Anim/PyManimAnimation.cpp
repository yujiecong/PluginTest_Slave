#include "PyManimAnimation.h"
#include "Utils/PyManimBlueprintLibrary.h"

float UPyManimAnimation::ApplyEasing(const FString& Type, float t)
{
	return UPyManimBlueprintLibrary::ApplyEasingByName(Type, t);
}
