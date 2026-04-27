// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MyUsdWrappers/ForwardDeclarations.h"
#include "MyUsdWrappers/SdfLayer.h"
#include "UsdWrappers/MyUsdStage.h"

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AMyUsdStageActor;
class ITableRow;
class STableViewBase;

struct FMyUsdSaveDialogRowData
{
	bool bSaveLayer = true;
	UE::FSdfLayerWeak Layer;
	TArray<UE::FUsdStageWeak> ConsumerStages;
	TSet<AMyUsdStageActor*> ConsumerActors;
};

/** Dialog shown to ask if the user wishes to save any dirty USD layer to disk */
class SMyUsdSaveDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMyUsdSaveDialog)
	{
	}
	SLATE_ARGUMENT(TArray<FMyUsdSaveDialogRowData>, Rows)
	SLATE_ARGUMENT(FText, DescriptionText)
	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_END_ARGS()

	// Returns the data describing which layers should be saved
	static TArray<FMyUsdSaveDialogRowData> ShowDialog(
		const TArray<FMyUsdSaveDialogRowData>& InRows,
		const FText& WindowTitle,
		const FText& DescriptionText,
		bool* OutShouldSave = nullptr,
		bool* OutShouldPromptAgain = nullptr
	);

	void Construct(const FArguments& InArgs);
	bool ShouldSave() const
	{
		return bProceed;
	}
	bool ShouldPromptAgain() const
	{
		return bPromptAgain;
	}

private:
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	TSharedRef<ITableRow> OnGenerateListRow(TSharedPtr<FMyUsdSaveDialogRowData> Item, const TSharedRef<STableViewBase>& OwnerTable);

	FReply Close(bool bProceed);

private:
	TWeakPtr<SWindow> Window;
	TArray<TSharedPtr<FMyUsdSaveDialogRowData>> Rows;

	bool bPromptAgain = true;
	bool bProceed = false;
};
