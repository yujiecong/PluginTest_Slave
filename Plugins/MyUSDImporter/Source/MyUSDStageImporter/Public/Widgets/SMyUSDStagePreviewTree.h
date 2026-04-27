// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UsdWrappers/ForwardDeclarations.h"
#include "Widgets/IMyUSDTreeViewItem.h"
#include "Widgets/SMyUSDTreeView.h"

#include "Templates/SharedPointer.h"

#define UE_API MYUSDSTAGEIMPORTER_API

using FUsdPrimPreviewModelViewRef = TSharedRef<struct FMyUsdPrimPreviewModelView>;
using FUsdPrimPreviewModelViewPtr = TSharedPtr<struct FMyUsdPrimPreviewModelView>;
using FUsdPrimPreviewModelViewWeak = TWeakPtr<struct FMyUsdPrimPreviewModelView>;

struct FMyUsdPrimPreviewModel
{
	// Prim's name
	FText Name;

	// Prim's type name (like "Mesh" or "Camera")
	FText TypeName;

	// Whether this prim should be imported or not
	bool bShouldImport = true;

	// Only used to make filtering items with SMyUsdStagePreviewTree::CurrentFilterText a bit faster: This is not
	// actually used to display anything
	bool bPassesFilter = true;

	// Our source of truth for expansion state. Whenever expansion changes on the views we write to this,
	// and whenever we rebuild our views we write this to the tree's sparse infos about its items
	bool bIsExpanded = true;

	int32 ParentIndex = INDEX_NONE;
	TArray<int32> ChildIndices;
};

struct FMyUsdPrimPreviewModelView : public IMyUsdTreeViewItem
{
	int32 ModelIndex = INDEX_NONE;

	FUsdPrimPreviewModelViewWeak Parent;
	TArray<FUsdPrimPreviewModelViewRef> Children;
};

class SMyUsdStagePreviewTree : public SMyUsdTreeView<FUsdPrimPreviewModelViewRef>
{
public:
	SLATE_BEGIN_ARGS(SMyUsdStagePreviewTree)
	{
	}
	SLATE_END_ARGS()

	UE_API void Construct(const FArguments& InArgs, const UE::FUsdStage& InStage);

	UE_API TArray<FString> GetSelectedFullPrimPaths() const;

	UE_API void SetFilterText(const FText& NewText);
	const FText& GetFilterText() const
	{
		return CurrentFilterText;
	}

	FMyUsdPrimPreviewModel& GetModel(int32 ModelIndex)
	{
		return ItemModels[ModelIndex];
	}
	const FMyUsdPrimPreviewModel& GetModel(int32 ModelIndex) const
	{
		return ItemModels[ModelIndex];
	}

	UE_API void CheckItemsRecursively(const TArray<FUsdPrimPreviewModelViewRef>& Items, bool bCheck);
	UE_API void ExpandItemsRecursively(const TArray<FUsdPrimPreviewModelViewRef>& Items, bool bExpand);

private:
	// Begin SMyUsdTreeView interface
	UE_API virtual TSharedRef<ITableRow> OnGenerateRow(FUsdPrimPreviewModelViewRef InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable) override;

	UE_API virtual void OnGetChildren(FUsdPrimPreviewModelViewRef InParent, TArray<FUsdPrimPreviewModelViewRef>& OutChildren) const override;

	UE_API virtual void SetupColumns() override;
	// End SMyUsdTreeView interface

	// Returns all selected model views, excluding child views of any other selected view.
	// i.e. if both a child and an ancestor view are selected, the returned array will contain only the ancestor.
	UE_API TArray<FUsdPrimPreviewModelViewRef> GetAncestorSelectedViews();

	UE_API TSharedPtr<SWidget> ConstructPrimContextMenu();

	// Rebuilds our RootItems array of FUsdPrimPreviewModelViewRefs based on CurrentFilterText and the ItemModels that
	// pass the filter
	UE_API void RebuildModelViews();

private:
	FText CurrentFilterText;

	TArray<FMyUsdPrimPreviewModel> ItemModels;
	TMap<int32, FUsdPrimPreviewModelViewRef> ItemModelsToViews;
};

// Custom row so that we can fetch the owning SMyUsdStagePreviewTree from the columns
class SMyUsdStagePreviewTreeRow : public SMyUsdTreeRow<FUsdPrimPreviewModelViewRef>
{
public:
	SLATE_BEGIN_ARGS(SMyUsdStagePreviewTreeRow)
	{
	}
	SLATE_END_ARGS()

	UE_API void Construct(
		const FArguments& InArgs,
		FUsdPrimPreviewModelViewRef InTreeItem,
		const TSharedRef<STableViewBase>& OwnerTable,
		TSharedPtr<FSharedUsdTreeData> InSharedData
	);

	UE_API TSharedPtr<SMyUsdStagePreviewTree> GetOwnerTree() const;
};

#undef UE_API
