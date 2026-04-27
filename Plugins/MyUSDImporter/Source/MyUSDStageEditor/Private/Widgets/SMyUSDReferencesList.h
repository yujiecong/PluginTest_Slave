// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/Views/SListView.h"

#if USE_USD_SDK

#include "MyUSDReferencesViewModel.h"
#include "UsdWrappers/ForwardDeclarations.h"

class SMyUsdReferenceRow : public SMultiColumnTableRow<TSharedPtr<FMyUsdReference>>
{
public:
	SLATE_BEGIN_ARGS(SMyUsdReferenceRow)
	{
	}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FMyUsdReference> InReference, const TSharedRef<STableViewBase>& OwnerTable);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	FString PrimPath;
	TSharedPtr<FMyUsdReference> Reference;
};

class SMyUsdReferencesList : public SListView<TSharedPtr<FMyUsdReference>>
{
	SLATE_BEGIN_ARGS(SMyUsdReferencesList)
	{
	}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	void SetPrimPath(const UE::FUsdStageWeak& UsdStage, const TCHAR* PrimPath);

protected:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FMyUsdReference> InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedPtr<SWidget> ConstructLayerContextMenu();

	bool CanRemoveReference() const;
	void RemoveReference();

	bool CanReloadReference() const;
	void ReloadReference();

private:
	FMyUsdReferencesViewModel ViewModel;
	TSharedPtr<SHeaderRow> HeaderRowWidget;
};

#endif	  // #if USE_USD_SDK
