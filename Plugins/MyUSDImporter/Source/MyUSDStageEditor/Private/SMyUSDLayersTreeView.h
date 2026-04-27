// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MyUsdWrappers/ForwardDeclarations.h"
#include "UsdWrappers/MyUsdStage.h"
#include "Widgets/SMyUSDTreeView.h"

#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STreeView.h"

class AMyUsdStageActor;

using FMyUsdLayerViewModelRef = TSharedRef<class FMyUsdLayerViewModel>;
using FMyUsdLayerViewModelWeak = TWeakPtr<class FMyUsdLayerViewModel>;

DECLARE_DELEGATE_OneParam(FOnLayerIsolated, const UE::FSdfLayer&);

class SMyUsdLayersTreeView : public SMyUsdTreeView<FMyUsdLayerViewModelRef>
{
public:
	SLATE_BEGIN_ARGS(SMyUsdLayersTreeView)
	{
	}
	SLATE_EVENT(FOnLayerIsolated, OnLayerIsolated)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Refresh(const UE::FMyUsdStageWeak& NewStage, const UE::FMyUsdStageWeak& IsolatedStage = {}, bool bResync = false);

	// Drag and drop interface for our rows
	FReply OnRowDragDetected(const FGeometry& Geometry, const FPointerEvent& PointerEvent);
	void OnRowDragLeave(const FDragDropEvent& Event);
	TOptional<EItemDropZone> OnRowCanAcceptDrop(const FDragDropEvent& Event, EItemDropZone Zone, FMyUsdLayerViewModelRef Item);
	FReply OnRowAcceptDrop(const FDragDropEvent& Event, EItemDropZone Zone, FMyUsdLayerViewModelRef Item);
	// End drag and drop interface

	TArray<UE::FSdfLayer> GetSelectedLayers() const;
	void SetSelectedLayers(const TArray<UE::FSdfLayer>& NewSelection);

	void ExportSelectedLayers(const FString& OutputDirectory = {}) const;

	const UE::FMyUsdStageWeak& GetStage() const
	{
		return UsdStage;
	}

private:
	virtual TSharedRef<ITableRow> OnGenerateRow(FMyUsdLayerViewModelRef InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable) override;
	virtual void OnGetChildren(FMyUsdLayerViewModelRef InParent, TArray<FMyUsdLayerViewModelRef>& OutChildren) const override;

	virtual void SetupColumns() override;

	void BuildUsdLayersEntries();

	TSharedPtr<SWidget> ConstructLayerContextMenu();

	bool CanIsolateSelectedLayer() const;
	void OnIsolateSelectedLayer();

	bool CanEditSelectedLayer() const;
	void OnEditSelectedLayer();

	bool CanMuteSelectedLayer() const;
	void OnMuteSelectedLayer();

	void OnClearSelectedLayers();
	bool CanClearSelectedLayers() const;

	void OnReloadSelectedLayers();
	bool CanReloadSelectedLayers() const;

	void OnSaveSelectedLayers();
	bool CanSaveSelectedLayers() const;

	bool CanInsertSubLayer() const;
	void OnAddSubLayer();
	void OnNewSubLayer();

	bool CanRemoveLayer(FMyUsdLayerViewModelRef LayerItem) const;
	bool CanRemoveSelectedLayers() const;
	void OnRemoveSelectedLayers();

	void RestoreExpansionStates();

public:
	// We update only on slate tick, but can receive a USD notice at any point, from any thread.
	// These variables are used to control what needs to be refreshed on the next tick

	enum class ELayersTreeViewState : uint8
	{
		NoRefreshNeeded = 0,
		RefreshNeeded = 1,
		ResyncNeeded = 2
	};
	ELayersTreeViewState bUpdateState = ELayersTreeViewState::NoRefreshNeeded;

	// Use a lock because some of the notices that will update these refresh variables can be
	// sent from TBB threads
	mutable FRWLock RefreshStateLock;

private:
	// Should always be valid, we keep the one we're given on Refresh()
	UE::FMyUsdStageWeak UsdStage;

	// A stage we create based on one of the sublayers of UsdStage
	UE::FMyUsdStageWeak IsolatedStage;

	// So that we can store these across refreshes
	TMap<FString, bool> TreeItemExpansionStates;

	// Used so that we can isolate the new layer without coupling to the SMyUSDStage widget too much
	FOnLayerIsolated LayerIsolatedDelegate;
};
