#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableText.h"
#include "Materials/Material.h"

class SShadertoyWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SShadertoyWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnCompileClicked();

	TSharedPtr<SMultiLineEditableText> CodeEditor;
	UMaterial* TargetMaterial;
	TSharedPtr<FSlateBrush> MaterialBrush;
};