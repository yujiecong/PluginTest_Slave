// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"

class IMyUsdTreeViewItem;

class FMyUsdTreeViewColumn
{
public:
	virtual ~FMyUsdTreeViewColumn() = default;

	virtual TSharedRef<SWidget> GenerateWidget(const TSharedPtr<IMyUsdTreeViewItem> TreeItem, const TSharedPtr<ITableRow> TableRow) = 0;

public:
	bool bIsMainColumn = false;
};

struct FSharedUsdTreeData
{
	TMap<FName, TSharedRef<FMyUsdTreeViewColumn>> Columns;
};

template<typename ItemType>
class SMyUsdTreeView : public STreeView<ItemType>
{
public:
	SLATE_BEGIN_ARGS(SMyUsdTreeView)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void AddColumn(
		FName ColumnKey,
		FText ColumnLabel,
		TSharedRef<FMyUsdTreeViewColumn> Column,
		SHeaderRow::FColumn::FArguments ColumnArguments = SHeaderRow::FColumn::FArguments()
	);
	virtual void SetupColumns() = 0;

	virtual TSharedRef<ITableRow> OnGenerateRow(ItemType InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable) = 0;
	virtual void OnGetChildren(ItemType InParent, TArray<ItemType>& OutChildren) const = 0;

	virtual void OnTreeItemScrolledIntoView(ItemType TreeItem, const TSharedPtr<ITableRow>& Widget)
	{
	}

protected:
	TSharedPtr<SHeaderRow> HeaderRowWidget;
	TArray<ItemType> RootItems;

	TSharedPtr<FSharedUsdTreeData> SharedData;
};

template<typename ItemType>
inline void SMyUsdTreeView<ItemType>::AddColumn(
	FName ColumnKey,
	FText ColumnLabel,
	TSharedRef<FMyUsdTreeViewColumn> Column,
	SHeaderRow::FColumn::FArguments ColumnArguments
)
{
	ColumnArguments.ColumnId(ColumnKey);
	ColumnArguments.DefaultLabel(ColumnLabel);

	HeaderRowWidget->AddColumn(ColumnArguments);

	SharedData->Columns.Add(ColumnKey, MoveTemp(Column));
}

template<typename ItemType>
inline void SMyUsdTreeView<ItemType>::Construct(const FArguments& InArgs)
{
	SharedData = MakeShared<FSharedUsdTreeData>();

	HeaderRowWidget = SNew(SHeaderRow).Visibility(EVisibility::Visible);

	SetupColumns();

	STreeView<ItemType>::Construct(typename STreeView<ItemType>::FArguments()
									   .TreeItemsSource(&RootItems)
									   .OnGenerateRow(this, &SMyUsdTreeView<ItemType>::OnGenerateRow)
									   .OnGetChildren(this, &SMyUsdTreeView<ItemType>::OnGetChildren)
									   .OnItemScrolledIntoView(this, &SMyUsdTreeView<ItemType>::OnTreeItemScrolledIntoView)
									   .HeaderRow(HeaderRowWidget));
}

template<typename ItemType>
class SMyUsdTreeRow : public SMultiColumnTableRow<ItemType>
{
public:
	typedef typename STableRow<ItemType>::FOnCanAcceptDrop FMyUsdOnCanAcceptDrop;
	typedef typename STableRow<ItemType>::FOnAcceptDrop FMyUsdOnAcceptDrop;

	SLATE_BEGIN_ARGS(SMyUsdTreeRow)
	{
	}
	SLATE_EVENT(FMyUsdOnCanAcceptDrop, OnCanAcceptDrop)
	SLATE_EVENT(FMyUsdOnAcceptDrop, OnAcceptDrop)
	SLATE_EVENT(FOnDragDetected, OnDragDetected)
	SLATE_EVENT(FOnTableRowDragEnter, OnDragEnter)
	SLATE_EVENT(FOnTableRowDragLeave, OnDragLeave)
	SLATE_EVENT(FOnTableRowDrop, OnDrop)
	SLATE_END_ARGS()

	void Construct(
		const FArguments& InArgs,
		ItemType InTreeItem,
		const TSharedRef<STableViewBase>& OwnerTable,
		TSharedPtr<FSharedUsdTreeData> InSharedData
	)
	{
		TreeItem = InTreeItem;
		SharedData = InSharedData;

		SMultiColumnTableRow<ItemType>::Construct(
			typename SMultiColumnTableRow<ItemType>::FArguments()
				.OnCanAcceptDrop(InArgs._OnCanAcceptDrop)
				.OnAcceptDrop(InArgs._OnAcceptDrop)
				.OnDragDetected(InArgs._OnDragDetected)
				.OnDragEnter(InArgs._OnDragEnter)
				.OnDragLeave(InArgs._OnDragLeave)
				.OnDrop(InArgs._OnDrop),
			OwnerTable
		);
	}

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<typename ItemType::ElementType> TreeItem;
	TSharedPtr<FSharedUsdTreeData> SharedData;
};

template<typename ItemType>
inline TSharedRef<SWidget> SMyUsdTreeRow<ItemType>::GenerateWidgetForColumn(const FName& ColumnName)
{
	TSharedRef<SWidget> ColumnWidget = SNullWidget::NullWidget;

	if (SharedData && SharedData->Columns.Contains(ColumnName))
	{
		ColumnWidget = SharedData->Columns[ColumnName]->GenerateWidget(
			StaticCastSharedPtr<IMyUsdTreeViewItem>(TSharedPtr<typename ItemType::ElementType>(TreeItem)),
			this->SharedThis(this)
		);
	}

	// clang-format off
	if (SharedData->Columns[ColumnName]->bIsMainColumn)
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6, 0, 0, 0)
			[
				SNew(SExpanderArrow, this->SharedThis(this))
				.IndentAmount(12)
			]

			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				ColumnWidget
			];
	}
	else
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				ColumnWidget
			];
	}
	// clang-format on
}
