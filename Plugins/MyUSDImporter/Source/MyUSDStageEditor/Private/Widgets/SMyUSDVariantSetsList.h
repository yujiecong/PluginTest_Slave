// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MyUSDVariantSetsViewModel.h"
#include "Widgets/Views/SListView.h"

#if USE_USD_SDK

class SMyUsdVariantRow : public SMultiColumnTableRow<TSharedPtr<FMyUsdVariantSetViewModel>>
{
public:
	DECLARE_DELEGATE_TwoParams(FOnVariantSelectionChanged, const TSharedRef<FMyUsdVariantSetViewModel>&, const TSharedPtr<FString>&);

public:
	SLATE_BEGIN_ARGS(SMyUsdVariantRow)
		: _OnVariantSelectionChanged()
	{
	}

	SLATE_EVENT(FOnVariantSelectionChanged, OnVariantSelectionChanged)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FMyUsdVariantSetViewModel> InVariantSet, const TSharedRef<STableViewBase>& OwnerTable);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

protected:
	FOnVariantSelectionChanged OnVariantSelectionChanged;

protected:
	void OnSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

private:
	FString PrimPath;
	TSharedPtr<FMyUsdVariantSetViewModel> VariantSet;
};

class SVariantsList : public SListView<TSharedPtr<FMyUsdVariantSetViewModel>>
{
	SLATE_BEGIN_ARGS(SVariantsList)
	{
	}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	void SetPrimPath(const UE::FMyUsdStageWeak& UsdStage, const TCHAR* InPrimPath);

protected:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FMyUsdVariantSetViewModel> InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable);

	void OnVariantSelectionChanged(const TSharedRef<FMyUsdVariantSetViewModel>& VariantSet, const TSharedPtr<FString>& NewValue);

private:
	FMyUsdVariantSetsViewModel ViewModel;
	TSharedPtr<SHeaderRow> HeaderRowWidget;
};

#endif	  // #if USE_USD_SDK
