// Copyright Epic Games, Inc. All Rights Reserved.

#include "SMyUSDStageTreeView.h"

#include "SMyUSDStageEditorStyle.h"
#include "UnrealUSDWrapper.h"
#include "MyUSDAttributeUtils.h"
#include "MyUSDConversionUtils.h"
#include "MyUSDDuplicateType.h"
#include "MyUSDLayerUtils.h"
#include "MyUSDMemory.h"
#include "MyUSDOptionsWindow.h"
#include "MyUSDPrimViewModel.h"
#include "MyUSDProjectSettings.h"
#include "MyUSDReferenceOptions.h"
#include "MyUSDTypesConversion.h"
#include "MyUsdWrappers/SdfChangeBlock.h"
#include "MyUsdWrappers/SdfPath.h"
#include "UsdWrappers/MyUsdPrim.h"
#include "UsdWrappers/MyUsdStage.h"

#include "Editor.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Templates/Function.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "UsdStageTreeView"

#if USE_USD_SDK

#include "MyUSDIncludesStart.h"
#include "pxr/usd/usdPhysics/tokens.h"
#include "pxr/usd/usdSkel/tokens.h"
#include "MyUSDIncludesEnd.h"

namespace UE::MyUSDStageTreeView::Private
{
	static const FText NoSpecOnLocalLayerStack = LOCTEXT(
		"NoLocalSpecToolTip",
		"This prim needs at least one spec on the stage's local layer stack for this option to be usable"
	);
}

enum class EPayloadsTrigger
{
	Load,
	Unload,
	Toggle
};

class FMyUsdStageNameColumn
	: public FMyUsdTreeViewColumn
	, public TSharedFromThis<FMyUsdStageNameColumn>
{
public:
	DECLARE_DELEGATE_TwoParams(FOnPrimNameCommitted, const FMyUsdPrimViewModelRef&, const FText&);
	FOnPrimNameCommitted OnPrimNameCommitted;

	DECLARE_DELEGATE_ThreeParams(FOnPrimNameUpdated, const FMyUsdPrimViewModelRef&, const FText&, FText&);
	FOnPrimNameUpdated OnPrimNameUpdated;

	TWeakPtr<SMyUsdStageTreeView> OwnerTree;

	virtual TSharedRef<SWidget> GenerateWidget(const TSharedPtr<IMyUsdTreeViewItem> InTreeItem, const TSharedPtr<ITableRow> TableRow) override
	{
		if (!InTreeItem)
		{
			return SNullWidget::NullWidget;
		}

		TSharedPtr<FMyUsdPrimViewModel> TreeItem = StaticCastSharedPtr<FMyUsdPrimViewModel>(InTreeItem);

		// clang-format off
		TSharedRef<SInlineEditableTextBlock> Item =
			SNew(SInlineEditableTextBlock)
			.Text(TreeItem->RowData, &FMyUsdPrimModel::GetName)
			.ColorAndOpacity(this, &FMyUsdStageNameColumn::GetTextColor, TreeItem)
			.OnTextCommitted(this, &FMyUsdStageNameColumn::OnTextCommitted, TreeItem)
			.OnVerifyTextChanged(this, &FMyUsdStageNameColumn::OnTextUpdated, TreeItem)
			.IsReadOnly_Lambda([TreeItem]()
			{
				return !TreeItem->bIsRenamingExistingPrim && (!TreeItem || TreeItem->UsdPrim);
			});

		TreeItem->RenameRequestEvent.BindSP(&Item.Get(), &SInlineEditableTextBlock::EnterEditingMode);

		return SNew(SBox)
			.VAlign(VAlign_Center)
			[
				Item
			];
		// clang-format on
	}

protected:
	void OnTextCommitted(const FText& InPrimName, ETextCommit::Type InCommitInfo, TSharedPtr<FMyUsdPrimViewModel> TreeItem)
	{
		if (!TreeItem)
		{
			return;
		}

		OnPrimNameCommitted.ExecuteIfBound(TreeItem.ToSharedRef(), InPrimName);
	}

	bool OnTextUpdated(const FText& InPrimName, FText& ErrorMessage, TSharedPtr<FMyUsdPrimViewModel> TreeItem)
	{
		if (!TreeItem)
		{
			return false;
		}

		OnPrimNameUpdated.ExecuteIfBound(TreeItem.ToSharedRef(), InPrimName, ErrorMessage);

		return ErrorMessage.IsEmpty();
	}

	FSlateColor GetTextColor(TSharedPtr<FMyUsdPrimViewModel> TreeItem) const
	{
		FSlateColor TextColor = FSlateColor::UseForeground();
		if (!TreeItem)
		{
			return TextColor;
		}

		if (TreeItem->RowData->HasCompositionArcs())
		{
			const TSharedPtr<SMyUsdStageTreeView> OwnerTreePtr = OwnerTree.Pin();
			if (OwnerTreePtr && OwnerTreePtr->IsItemSelected(TreeItem.ToSharedRef()))
			{
				TextColor = FMyUsdStageEditorStyle::Get()->GetColor("UsdStageEditor.HighlightPrimCompositionArcColor");
			}
			else
			{
				TextColor = FMyUsdStageEditorStyle::Get()->GetColor("UsdStageEditor.PrimCompositionArcColor");
			}
		}

		return TextColor;
	}
};

class FMyUsdStagePayloadColumn
	: public FMyUsdTreeViewColumn
	, public TSharedFromThis<FMyUsdStagePayloadColumn>
{
public:
	ECheckBoxState IsChecked(const FMyUsdPrimViewModelPtr InTreeItem) const
	{
		ECheckBoxState CheckedState = ECheckBoxState::Unchecked;

		if (InTreeItem && InTreeItem->RowData->HasPayload())
		{
			CheckedState = InTreeItem->RowData->IsLoaded() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}

		return CheckedState;
	}

	void OnCheckedPayload(ECheckBoxState NewCheckedState, const FMyUsdPrimViewModelPtr TreeItem)
	{
		if (!TreeItem)
		{
			return;
		}

		switch (NewCheckedState)
		{
			case ECheckBoxState::Checked:
				TreeItem->UsdPrim.Load();
				break;
			case ECheckBoxState::Unchecked:
				TreeItem->UsdPrim.Unload();
				break;
			default:
				break;
		}
	}

	virtual TSharedRef<SWidget> GenerateWidget(const TSharedPtr<IMyUsdTreeViewItem> InTreeItem, const TSharedPtr<ITableRow> TableRow) override
	{
		const TWeakPtr<FMyUsdPrimViewModel> TreeItem = StaticCastSharedPtr<FMyUsdPrimViewModel>(InTreeItem);

		// clang-format off
		TSharedRef<SWidget> Item = SNew(SCheckBox)
		.Visibility_Lambda([TreeItem]() -> EVisibility
		{
			if (const TSharedPtr<FMyUsdPrimViewModel> PinnedItem = TreeItem.Pin())
			{
				return PinnedItem->RowData->HasPayload() ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Collapsed;
		})
		.ToolTipText(LOCTEXT("TogglePayloadToolTip", "Toggle payload"))
		.IsChecked(this, &FMyUsdStagePayloadColumn::IsChecked, StaticCastSharedPtr<FMyUsdPrimViewModel>(InTreeItem))
		.OnCheckStateChanged(this, &FMyUsdStagePayloadColumn::OnCheckedPayload, StaticCastSharedPtr<FMyUsdPrimViewModel>(InTreeItem));
		// clang-format on

		return Item;
	}
};

class FMyUsdStageVisibilityColumn
	: public FMyUsdTreeViewColumn
	, public TSharedFromThis<FMyUsdStageVisibilityColumn>
{
public:
	FReply OnToggleVisibility(const FMyUsdPrimViewModelPtr TreeItem)
	{
		FScopedTransaction Transaction(
			FText::Format(LOCTEXT("VisibilityTransaction", "Toggle visibility of prim '{0}'"), FText::FromName(TreeItem->UsdPrim.GetName()))
		);

		TreeItem->ToggleVisibility();

		return FReply::Handled();
	}

	const FSlateBrush* GetBrush(const FMyUsdPrimViewModelPtr TreeItem, const TSharedPtr<SButton> Button) const
	{
		const bool bIsButtonHovered = Button.IsValid() && Button->IsHovered();

		if (TreeItem->RowData->IsVisible())
		{
			return bIsButtonHovered ? FAppStyle::GetBrush("Level.VisibleHighlightIcon16x") : FAppStyle::GetBrush("Level.VisibleIcon16x");
		}
		else
		{
			return bIsButtonHovered ? FAppStyle::GetBrush("Level.NotVisibleHighlightIcon16x") : FAppStyle::GetBrush("Level.NotVisibleIcon16x");
		}
	}

	FSlateColor GetForegroundColor(const FMyUsdPrimViewModelPtr TreeItem, const TSharedPtr<ITableRow> TableRow, const TSharedPtr<SButton> Button) const
	{
		if (!TreeItem.IsValid() || !TableRow.IsValid() || !Button.IsValid())
		{
			return FSlateColor::UseForeground();
		}

		const bool bIsRowHovered = TableRow->AsWidget()->IsHovered();
		const bool bIsButtonHovered = Button->IsHovered();
		const bool bIsRowSelected = TableRow->IsItemSelected();
		const bool bIsPrimVisible = TreeItem->RowData->IsVisible();

		if (bIsPrimVisible && !bIsRowHovered && !bIsRowSelected)
		{
			return FLinearColor::Transparent;
		}
		else if (bIsButtonHovered && !bIsRowSelected)
		{
			return FAppStyle::GetSlateColor(TEXT("Colors.ForegroundHover"));
		}

		return FSlateColor::UseForeground();
	}

	virtual TSharedRef<SWidget> GenerateWidget(const TSharedPtr<IMyUsdTreeViewItem> InTreeItem, const TSharedPtr<ITableRow> TableRow) override
	{
		if (!InTreeItem)
		{
			return SNullWidget::NullWidget;
		}

		const TSharedPtr<FMyUsdPrimViewModel> TreeItem = StaticCastSharedPtr<FMyUsdPrimViewModel>(InTreeItem);
		const float ItemSize = FMyUsdStageEditorStyle::Get()->GetFloat("UsdStageEditor.ListItemHeight");

		// clang-format off
		if (!TreeItem->HasVisibilityAttribute())
		{
			return SNew(SBox)
				.HeightOverride(ItemSize)
				.WidthOverride(ItemSize)
				.Visibility(EVisibility::Visible)
				.ToolTip(SNew(SToolTip).Text(LOCTEXT("NoGeomImageable", "Only prims with the GeomImageable schema (or derived) have the visibility attribute!")));
		}

		TSharedPtr<SButton> Button = SNew(SButton)
			.ContentPadding(0)
			.ButtonStyle(FMyUsdStageEditorStyle::Get(), TEXT("NoBorder"))
			.OnClicked(this, &FMyUsdStageVisibilityColumn::OnToggleVisibility, TreeItem)
			.ToolTip(SNew(SToolTip).Text(LOCTEXT("GeomImageable", "Toggle the visibility of this prim")))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center);

		TSharedPtr<SImage> Image = SNew(SImage)
			.Image(this, &FMyUsdStageVisibilityColumn::GetBrush, TreeItem, Button)
			.ColorAndOpacity(this, &FMyUsdStageVisibilityColumn::GetForegroundColor, TreeItem, TableRow, Button);

		Button->SetContent(Image.ToSharedRef());

		return SNew(SBox)
			.HeightOverride(ItemSize)
			.WidthOverride(ItemSize)
			.Visibility(EVisibility::Visible)
			[
				Button.ToSharedRef()
			];
		// clang-format on
	}
};

class FMyUsdStagePrimTypeColumn : public FMyUsdTreeViewColumn
{
public:
	virtual TSharedRef<SWidget> GenerateWidget(const TSharedPtr<IMyUsdTreeViewItem> InTreeItem, const TSharedPtr<ITableRow> TableRow) override
	{
		TSharedPtr<FMyUsdPrimViewModel> TreeItem = StaticCastSharedPtr<FMyUsdPrimViewModel>(InTreeItem);

		// clang-format off
		return SNew(SBox)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(TreeItem->RowData, &FMyUsdPrimModel::GetType)
			];
		// clang-format on
	}
};

void SMyUsdStageTreeView::Construct(const FArguments& InArgs)
{
	SMyUsdTreeView::Construct(SMyUsdTreeView::FArguments());

	OnContextMenuOpening = FOnContextMenuOpening::CreateSP(this, &SMyUsdStageTreeView::ConstructPrimContextMenu);

	OnSelectionChanged = FOnSelectionChanged::CreateLambda(
		[this](FMyUsdPrimViewModelPtr UsdStageTreeItem, ESelectInfo::Type SelectionType)
		{
			FString SelectedPrimPath;

			if (UsdStageTreeItem)
			{
				SelectedPrimPath = UsdToUnreal::ConvertPath(UsdStageTreeItem->UsdPrim.GetPrimPath());
			}

			TArray<FString> SelectedPrimPaths = GetSelectedPrimPaths();
			this->OnPrimSelectionChanged.ExecuteIfBound(SelectedPrimPaths);
		}
	);

	OnExpansionChanged = FOnExpansionChanged::CreateLambda(
		[this](const FMyUsdPrimViewModelPtr& UsdPrimViewModel, bool bIsExpanded)
		{
			if (!UsdPrimViewModel)
			{
				return;
			}

			const UE::FMyUsdPrim& Prim = UsdPrimViewModel->UsdPrim;
			if (!Prim)
			{
				return;
			}

			UsdPrimViewModel->SetIsExpanded(bIsExpanded);

			// We have a special handling for the root because we'll want to manually expand it by
			// default at first but also remember if the user collapsed it or not. For all
			// other prims we truly just leave the nodes at default collapsed unless we have
			// recorded that the node should be expanded
			if (Prim.IsPseudoRoot())
			{
				RootWasExpanded = bIsExpanded;
			}
			else
			{
				const FString& PrimPath = Prim.GetPrimPath().GetString();
				if (bIsExpanded)
				{
					ExpandedPrimPaths.Add(PrimPath);
				}
				else
				{
					ExpandedPrimPaths.Remove(PrimPath);
				}
			}
		}
	);

	OnPrimSelectionChanged = InArgs._OnPrimSelectionChanged;

	PostUndoRedoHandle = FEditorDelegates::PostUndoRedo.AddLambda(
		[this]()
		{
			// This is in charge of restoring our expansion states after we undo/redo a prim rename
			RequestExpansionStateRestore();
		}
	);

	UICommandList = MakeShared<FUICommandList>();
	UICommandList->MapAction(
		FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnCutPrim),
		FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimHaveSpecOnLocalLayerStack)
	);
	UICommandList->MapAction(
		FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnCopyPrim),
		FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimExistOnStage)
	);
	UICommandList->MapAction(
		FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnPastePrim),
		FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::CanPastePrim)
	);
	UICommandList->MapAction(
		FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnDuplicatePrim, EMyUsdDuplicateType::AllLocalLayerSpecs),
		FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimExistOnStage)
	);
	UICommandList->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnDeletePrim),
		FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimHaveSpecOnLocalLayerStack)
	);
	UICommandList->MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnRenamePrim),
		FCanExecuteAction::CreateLambda(
			[this]()
			{
				return SMyUsdStageTreeView::DoesPrimHaveSpecOnLocalLayerStack() && GetSelectedItems().Num() == 1;
			}
		)
	);
}

SMyUsdStageTreeView::~SMyUsdStageTreeView()
{
	FEditorDelegates::PostUndoRedo.Remove(PostUndoRedoHandle);
}

TSharedRef<ITableRow> SMyUsdStageTreeView::OnGenerateRow(FMyUsdPrimViewModelRef InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SMyUsdTreeRow<FMyUsdPrimViewModelRef>, InDisplayNode, OwnerTable, SharedData);
}

void SMyUsdStageTreeView::OnGetChildren(FMyUsdPrimViewModelRef InParent, TArray<FMyUsdPrimViewModelRef>& OutChildren) const
{
	for (const FMyUsdPrimViewModelRef& Child : InParent->UpdateChildren())
	{
		OutChildren.Add(Child);
	}
}

void SMyUsdStageTreeView::Refresh(const UE::FMyUsdStageWeak& NewStage)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SMyUsdStageTreeView::Refresh);

	UE::FMyUsdStageWeak OldStage = RootItems.Num() > 0 ? RootItems[0]->UsdStage : UE::FMyUsdStageWeak();

	RootItems.Empty();

	UsdStage = NewStage;

	if (OldStage != NewStage)
	{
		RootWasExpanded.Reset();
		ExpandedPrimPaths.Reset();
	}

	if (NewStage)
	{
		if (UE::FMyUsdPrim RootPrim = NewStage.GetPseudoRoot())
		{
			RootItems.Add(MakeShared<FMyUsdPrimViewModel>(nullptr, UsdStage, RootPrim));
		}

		RequestExpansionStateRestore();
	}
}

void SMyUsdStageTreeView::RefreshPrim(const FString& PrimPath, bool bResync)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SMyUsdStageTreeView::RefreshPrim);

	FScopedUnrealAllocs UnrealAllocs;	 // RefreshPrim can be called by a delegate for which we don't know the active allocator

	FMyUsdPrimViewModelPtr FoundItem = GetItemFromPrimPath(PrimPath);

	if (FoundItem.IsValid())
	{
		FoundItem->RefreshData(true);

		// Item doesn't match any prim, needs to be removed
		if (!FoundItem->UsdPrim)
		{
			if (FoundItem->ParentItem)
			{
				FoundItem->ParentItem->RefreshData(true);
			}
			else
			{
				RootItems.Remove(FoundItem.ToSharedRef());
			}
		}
	}
	// We couldn't find the target prim, do a full refresh instead
	else
	{
		FWriteScopeLock StageTreeViewLock{RefreshStateLock};
		bNeedsFullUpdate = true;
	}

	if (bResync)
	{
		FWriteScopeLock StageTreeViewLock{RefreshStateLock};
		bNeedsFullUpdate = true;
	}
}

FMyUsdPrimViewModelPtr SMyUsdStageTreeView::GetItemFromPrimPath(const FString& PrimPath)
{
	FScopedUnrealAllocs UnrealAllocs;

	UE::FSdfPath UsdPrimPath(*PrimPath);
	if (!UsdStage.GetPrimAtPath(UsdPrimPath))
	{
		return nullptr;
	}

	TFunction<FMyUsdPrimViewModelPtr(const UE::FSdfPath&, const FMyUsdPrimViewModelRef&)> FindTreeItemFromPrimPath;
	FindTreeItemFromPrimPath = [&FindTreeItemFromPrimPath,
								this](const UE::FSdfPath& UsdPrimPath, const FMyUsdPrimViewModelRef& ItemRef) -> FMyUsdPrimViewModelPtr
	{
		UE::FSdfPath ItemPath = ItemRef->UsdPrim.GetPrimPath();
		if (ItemPath == UsdPrimPath)
		{
			return ItemRef;
		}
		else if (UsdPrimPath.HasPrefix(ItemPath))
		{
			// If we're past the check at the top of this function we know we have a prim for this path.
			// If we do, then we *must* be able to generate a FMyUsdPrimViewModelPtr for it (if we dig deep enough),
			// so let's expand ItemRef's parent on-demand, so that we generate our children that we can step into
			if (!ItemRef->ShouldGenerateChildren() && ItemRef->ParentItem)
			{
				SetItemExpansion(StaticCastSharedRef<FMyUsdPrimViewModel>(ItemRef->ParentItem->AsShared()), true);
			}

			for (FMyUsdPrimViewModelRef ChildItem : ItemRef->Children)
			{
				if (FMyUsdPrimViewModelPtr ChildValue = FindTreeItemFromPrimPath(UsdPrimPath, ChildItem))
				{
					return ChildValue;
				}
			}
		}

		return {};
	};

	// Search for the corresponding tree item to update
	FMyUsdPrimViewModelPtr FoundItem = nullptr;
	for (FMyUsdPrimViewModelRef RootItem : this->RootItems)
	{
		UE::FSdfPath PrimPathToSearch = UsdPrimPath;

		FoundItem = FindTreeItemFromPrimPath(PrimPathToSearch, RootItem);

		// If we haven't found an item, try finding an item for an ancestor
		while (!FoundItem.IsValid())
		{
			UE::FSdfPath ParentPrimPath = PrimPathToSearch.GetParentPath();
			if (ParentPrimPath == PrimPathToSearch)
			{
				break;
			}
			PrimPathToSearch = MoveTemp(ParentPrimPath);

			FoundItem = FindTreeItemFromPrimPath(PrimPathToSearch, RootItem);
		}

		if (FoundItem.IsValid())
		{
			break;
		}
	}

	return FoundItem;
}

void SMyUsdStageTreeView::SelectItemsInternal(const TArray<FMyUsdPrimViewModelRef>& ItemsToSelect)
{
	if (ItemsToSelect.Num() > 0)
	{
		// Clear selection without emitting events, as we'll emit new events with SetItemSelection
		// anyway. This prevents a UI blink as OnPrimSelectionChanged would otherwise fire for
		// ClearSelection() and then again right away for SetItemSelection()
		Private_ClearSelection();

		const bool bSelected = true;
		SetItemSelection(ItemsToSelect, bSelected);
		ScrollItemIntoView(ItemsToSelect.Last());
	}
	else
	{
		ClearSelection();

		// ClearSelection is not going to fire the OnSelectionChanged event in case we have nothing selected, but we
		// need to do that to refresh the prim properties panel to display the stage properties instead
		OnPrimSelectionChanged.ExecuteIfBound({});
	}
}

void SMyUsdStageTreeView::SetSelectedPrimPaths(const TArray<FString>& PrimPaths)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SetSelectedPrimPaths);

	TArray<FMyUsdPrimViewModelRef> ItemsToSelect;
	ItemsToSelect.Reserve(PrimPaths.Num());

	for (const FString& PrimPath : PrimPaths)
	{
		if (FMyUsdPrimViewModelPtr FoundItem = GetItemFromPrimPath(PrimPath))
		{
			ItemsToSelect.Add(FoundItem.ToSharedRef());
		}
	}

	SelectItemsInternal(ItemsToSelect);
}

void SMyUsdStageTreeView::SetSelectedPrims(const TArray<UE::FMyUsdPrim>& Prims)
{
	TArray<FString> PrimPaths;
	PrimPaths.Reserve(Prims.Num());
	for (const UE::FMyUsdPrim& Prim : Prims)
	{
		PrimPaths.Add(Prim.GetPrimPath().GetString());
	}

	SetSelectedPrimPaths(PrimPaths);
}

TArray<FString> SMyUsdStageTreeView::GetSelectedPrimPaths()
{
	TArray<FString> SelectedPrimPaths;
	SelectedPrimPaths.Reserve(GetNumItemsSelected());

	for (FMyUsdPrimViewModelRef SelectedItem : GetSelectedItems())
	{
		SelectedPrimPaths.Add(SelectedItem->UsdPrim.GetPrimPath().GetString());
	}

	return SelectedPrimPaths;
}

TArray<UE::FMyUsdPrim> SMyUsdStageTreeView::GetSelectedPrims()
{
	TArray<UE::FMyUsdPrim> SelectedPrims;
	SelectedPrims.Reserve(GetNumItemsSelected());

	for (FMyUsdPrimViewModelRef SelectedItem : GetSelectedItems())
	{
		SelectedPrims.Add(SelectedItem->UsdPrim);
	}

	return SelectedPrims;
}

void SMyUsdStageTreeView::SetupColumns()
{
	HeaderRowWidget->ClearColumns();

	SHeaderRow::FColumn::FArguments VisibilityColumnArguments;
	VisibilityColumnArguments.FixedWidth(24.f);

	AddColumn(TEXT("Visibility"), FText(), MakeShared<FMyUsdStageVisibilityColumn>(), VisibilityColumnArguments);

	{
		TSharedRef<FMyUsdStageNameColumn> PrimNameColumn = MakeShared<FMyUsdStageNameColumn>();
		PrimNameColumn->OwnerTree = SharedThis(this);
		PrimNameColumn->bIsMainColumn = true;
		PrimNameColumn->OnPrimNameCommitted.BindRaw(this, &SMyUsdStageTreeView::OnPrimNameCommitted);
		PrimNameColumn->OnPrimNameUpdated.BindRaw(this, &SMyUsdStageTreeView::OnPrimNameUpdated);

		SHeaderRow::FColumn::FArguments PrimNameColumnArguments;
		PrimNameColumnArguments.FillWidth(70.f);

		AddColumn(TEXT("Prim"), LOCTEXT("Prim", "Prim"), PrimNameColumn, PrimNameColumnArguments);
	}

	SHeaderRow::FColumn::FArguments TypeColumnArguments;
	TypeColumnArguments.FillWidth(15.f);
	AddColumn(TEXT("Type"), LOCTEXT("Type", "Type"), MakeShared<FMyUsdStagePrimTypeColumn>(), TypeColumnArguments);

	SHeaderRow::FColumn::FArguments PayloadColumnArguments;
	PayloadColumnArguments.FillWidth(15.f).HAlignHeader(HAlign_Center).HAlignCell(HAlign_Center);
	AddColumn(TEXT("Payload"), LOCTEXT("Payload", "Payload"), MakeShared<FMyUsdStagePayloadColumn>(), PayloadColumnArguments);
}

TSharedPtr<SWidget> SMyUsdStageTreeView::ConstructPrimContextMenu()
{
	TSharedRef<SWidget> MenuWidget = SNullWidget::NullWidget;

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder PrimOptions(bInShouldCloseWindowAfterMenuSelection, UICommandList);
	PrimOptions.BeginSection("Edit", LOCTEXT("EditText", "Edit"));
	{
		PrimOptions.AddMenuEntry(
			TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
				[this]()
				{
					return GetSelectedItems().Num() == 0 ? LOCTEXT("AddTopLevelPrim", "Add Prim") : LOCTEXT("AddPrim", "Add Child");
				}
			)),
			TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
				[this]()
				{
					return GetSelectedItems().Num() == 0 ? LOCTEXT("AddTopPrim_ToolTip", "Adds a new top-level prim")
														 : LOCTEXT("AddPrim_ToolTip", "Adds a new prim as a child of this one");
				}
			)),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.PlaceActors"),
			FUIAction(
				FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnAddChildPrim),
				FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::CanAddChildPrim)
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);

		PrimOptions.AddMenuEntry(
			FGenericCommands::Get().Cut,
			NAME_None,
			LOCTEXT("Cut_Text", "Cut"),
			TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
				[this]()
				{
					return DoesPrimHaveSpecOnLocalLayerStack()
							   ? LOCTEXT("Cut_ToolTip", "Cuts the selected prim's specs from the stage's local layer stack")
							   : UE::MyUSDStageTreeView::Private::NoSpecOnLocalLayerStack;
				}
			))
		);

		PrimOptions.AddMenuEntry(
			FGenericCommands::Get().Copy,
			NAME_None,
			LOCTEXT("Copy_Text", "Copy"),
			LOCTEXT("Copy_ToolTip", "Copies the selected prims")
		);

		PrimOptions.AddMenuEntry(
			FGenericCommands::Get().Paste,
			NAME_None,
			LOCTEXT("Paste_Text", "Paste"),
			LOCTEXT("Paste_ToolTip", "Pastes a flattened representation of the cut/copied prims as children of this prim, on the current edit target")
		);

		const bool bInOpenSubMenuOnClick = false;
		PrimOptions.AddSubMenu(
			LOCTEXT("Duplicate_Text", "Duplicate..."),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateSP(this, &SMyUsdStageTreeView::FillDuplicateSubmenu),
			bInOpenSubMenuOnClick,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Duplicate")
		);

		PrimOptions.AddMenuEntry(
			FGenericCommands::Get().Delete,
			NAME_None,
			LOCTEXT("Delete_Text", "Delete"),
			TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
				[this]()
				{
					return DoesPrimHaveSpecOnLocalLayerStack()
							   ? LOCTEXT("Delete_ToolTip", "Deletes the selected prim's specs from the local layer stack")
							   : UE::MyUSDStageTreeView::Private::NoSpecOnLocalLayerStack;
				}
			))
		);

		PrimOptions.AddMenuEntry(
			FGenericCommands::Get().Rename,
			NAME_None,
			LOCTEXT("Rename_Text", "Rename"),
			TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
				[this]()
				{
					return DoesPrimHaveSpecOnLocalLayerStack()
							   ? LOCTEXT("Rename_ToolTip", "Renames the selected prim's specs on the local layer stack")
							   : UE::MyUSDStageTreeView::Private::NoSpecOnLocalLayerStack;
				}
			))
		);

		PrimOptions.AddSubMenu(
			LOCTEXT("Collapsing_Text", "Collapsing..."),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateSP(this, &SMyUsdStageTreeView::FillCollapsingSubmenu),
			bInOpenSubMenuOnClick,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Merge")
		);
	}
	PrimOptions.EndSection();

	PrimOptions.BeginSection("Payloads", LOCTEXT("Payloads", "Payloads"));
	{
		PrimOptions.AddMenuEntry(
			LOCTEXT("TogglePayloads", "Toggle All Payloads"),
			LOCTEXT("TogglePayloads_ToolTip", "Toggles all payloads for this prim and its children"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnToggleAllPayloads, EPayloadsTrigger::Toggle), FCanExecuteAction()),
			NAME_None,
			EUserInterfaceActionType::Button
		);

		PrimOptions.AddMenuEntry(
			LOCTEXT("LoadPayloads", "Load All Payloads"),
			LOCTEXT("LoadPayloads_ToolTip", "Loads all payloads for this prim and its children"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnToggleAllPayloads, EPayloadsTrigger::Load), FCanExecuteAction()),
			NAME_None,
			EUserInterfaceActionType::Button
		);

		PrimOptions.AddMenuEntry(
			LOCTEXT("UnloadPayloads", "Unload All Payloads"),
			LOCTEXT("UnloadPayloads_ToolTip", "Unloads all payloads for this prim and its children"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnToggleAllPayloads, EPayloadsTrigger::Unload), FCanExecuteAction()),
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}
	PrimOptions.EndSection();

	PrimOptions.BeginSection("Composition", LOCTEXT("Composition", "Composition"));
	{
		PrimOptions.AddMenuEntry(
			LOCTEXT("AddReference", "Add Reference"),
			LOCTEXT("AddReference_ToolTip", "Adds a reference for this prim"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnAddReference),
				FCanExecuteAction::CreateLambda(
					[this]()
					{
						return SMyUsdStageTreeView::DoesPrimExistOnEditTarget() && GetSelectedItems().Num() == 1;
					}
				)
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);

		PrimOptions.AddMenuEntry(
			LOCTEXT("ClearReferences", "Clear References"),
			TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
				[this]()
				{
					return DoesPrimHaveReferenceSpecOnLocalLayerStack()
							   ? LOCTEXT("ClearReferences_ToolTip", "Clears the references for this prim")
							   : LOCTEXT("ClearReferencesNoSpec_ToolTip", "The prim doesn't have any reference spec on the current edit target");
				}
			)),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnClearReferences),
				FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimHaveReferenceSpecOnLocalLayerStack)
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);

		PrimOptions.AddMenuEntry(
			LOCTEXT("AddPayload", "Add Payload"),
			LOCTEXT("AddPayload_ToolTip", "Adds a payload for this prim"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnAddPayload),
				FCanExecuteAction::CreateLambda(
					[this]()
					{
						return SMyUsdStageTreeView::DoesPrimExistOnEditTarget() && GetSelectedItems().Num() == 1;
					}
				)
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);

		PrimOptions.AddMenuEntry(
			LOCTEXT("ClearPayloads", "Clear Payloads"),
			TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
				[this]()
				{
					return DoesPrimHavePayloadSpecOnLocalLayerStack()
							   ? LOCTEXT("ClearPayloads_ToolTip", "Clears the payloads for this prim")
							   : LOCTEXT("ClearPayloadsNoSpec_ToolTip", "The prim doesn't have any payload spec on the current edit target");
				}
			)),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnClearPayloads),
				FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimHavePayloadSpecOnLocalLayerStack)
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}
	PrimOptions.EndSection();

	PrimOptions.BeginSection("Schemas", LOCTEXT("Schemas", "Schemas"));
	{
		const bool bInOpenSubMenuOnClick = false;
		PrimOptions.AddSubMenu(
			LOCTEXT("AddSchemaText", "Add schema..."),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateSP(this, &SMyUsdStageTreeView::FillAddSchemaSubmenu),
			bInOpenSubMenuOnClick
		);

		PrimOptions.AddSubMenu(
			LOCTEXT("RemoveSchemaText", "Remove schema..."),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateSP(this, &SMyUsdStageTreeView::FillRemoveSchemaSubmenu),
			bInOpenSubMenuOnClick
		);
	}
	PrimOptions.EndSection();

	MenuWidget = PrimOptions.MakeWidget();

	return MenuWidget;
}

void SMyUsdStageTreeView::OnAddChildPrim()
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	// Add a new child prim
	if (MySelectedItems.Num() > 0)
	{
		for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
		{
			FMyUsdPrimViewModelRef TreeItem = MakeShared<FMyUsdPrimViewModel>(&SelectedItem.Get(), SelectedItem->UsdStage);
			SelectedItem->Children.Add(TreeItem);

			PendingRenameItem = TreeItem;
			ScrollItemIntoView(TreeItem);
		}
	}
	// Add a new top-level prim (direct child of the pseudo-root prim)
	else
	{
		FMyUsdPrimViewModelRef TreeItem = MakeShared<FMyUsdPrimViewModel>(nullptr, UsdStage);
		RootItems.Add(TreeItem);

		PendingRenameItem = TreeItem;
		ScrollItemIntoView(TreeItem);
	}

	RequestTreeRefresh();
}

void SMyUsdStageTreeView::OnCutPrim()
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(LOCTEXT("CutPrimTransaction", "Cut prims"));

	UE::FSdfChangeBlock Block;

	TArray<UE::FMyUsdPrim> Prims;
	Prims.Reserve(MySelectedItems.Num());

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (SelectedItem->UsdPrim)
		{
			Prims.Add(SelectedItem->UsdPrim);
		}
	}

	UsdUtils::CutPrims(Prims);
}

void SMyUsdStageTreeView::OnCopyPrim()
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	TArray<UE::FMyUsdPrim> Prims;
	Prims.Reserve(MySelectedItems.Num());

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (SelectedItem->UsdPrim)
		{
			Prims.Add(SelectedItem->UsdPrim);
		}
	}

	UsdUtils::CopyPrims(Prims);
}

void SMyUsdStageTreeView::OnPastePrim()
{
	if (!UsdStage)
	{
		return;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(LOCTEXT("PastePrimTransaction", "Paste prims"));

	UE::FSdfChangeBlock Block;

	TArray<UE::FMyUsdPrim> ParentPrims;

	// This happens when right-clicking the background area without selecting any prim
	if (MySelectedItems.IsEmpty())
	{
		ParentPrims.Add(UsdStage.GetPseudoRoot());
	}
	else
	{
		ParentPrims.Reserve(MySelectedItems.Num());

		// A bit unusual that we can paste to multiple locations at the same time, but why not?
		for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
		{
			ParentPrims.Add(SelectedItem->UsdPrim);
		}
	}

	for (const UE::FMyUsdPrim& ParentPrim : ParentPrims)
	{
		// Preemptively mark the parent prims as expanded so that we can always see what we pasted
		ExpandedPrimPaths.Add(ParentPrim.GetPrimPath().GetString());

		UsdUtils::PastePrims(ParentPrim);
	}
}

void SMyUsdStageTreeView::OnDuplicatePrim(EMyUsdDuplicateType DuplicateType)
{
	if (!UsdStage)
	{
		return;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(LOCTEXT("DuplicatePrimTransaction", "Duplicate prims"));

	TArray<UE::FMyUsdPrim> Prims;
	Prims.Reserve(MySelectedItems.Num());

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (SelectedItem->UsdPrim)
		{
			Prims.Add(SelectedItem->UsdPrim);
		}
	}

	UsdUtils::DuplicatePrims(Prims, DuplicateType, UsdStage.GetEditTarget());
}

void SMyUsdStageTreeView::OnDeletePrim()
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(LOCTEXT("DeletePrimTransaction", "Delete prims"));

	UE::FSdfChangeBlock Block;

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		UsdUtils::RemoveAllLocalPrimSpecs(SelectedItem->UsdPrim);
	}
}

void SMyUsdStageTreeView::OnRenamePrim()
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	if (MySelectedItems.Num() > 0)
	{
		FMyUsdPrimViewModelRef TreeItem = MySelectedItems[0];

		TreeItem->bIsRenamingExistingPrim = true;
		PendingRenameItem = TreeItem;
		RequestScrollIntoView(TreeItem);
	}
}

void SMyUsdStageTreeView::OnSetCollapsingPreference(UsdUtils::ECollapsingPreference Preference)
{
	if (!UsdStage)
	{
		return;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(LOCTEXT("SetCollapsingPreferenceTransaction", "Set collapsing preference"));

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		UsdUtils::SetCollapsingPreference(SelectedItem->UsdPrim, Preference);
	}
}

void SMyUsdStageTreeView::OnAddReference()
{
	if (!UsdStage || !UsdStage.IsEditTargetValid())
	{
		return;
	}

	TStrongObjectPtr<UMyUsdReferenceOptions> Options = TStrongObjectPtr<UMyUsdReferenceOptions>{NewObject<UMyUsdReferenceOptions>()};
	UMyUsdReferenceOptions* OptionsPtr = Options.Get();
	if (!OptionsPtr)
	{
		return;
	}

	bool bContinue = SMyUsdOptionsWindow::ShowOptions(*OptionsPtr, LOCTEXT("AddReferenceTitle", "Add reference"), LOCTEXT("AddReferenceAccept", "OK"));
	if (!bContinue)
	{
		return;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	if (MySelectedItems.Num() != 1)
	{
		return;
	}
	UE::FMyUsdPrim Referencer = MySelectedItems[0]->UsdPrim;
	if (UsdUtils::NotifyIfInstanceProxy(Referencer))
	{
		return;
	}

	// This transaction is important as adding a reference may trigger the creation of new unreal assets, which need to be
	// destroyed if we spam undo afterwards. Undoing won't remove the actual reference from the stage yet though, sadly...
	FScopedTransaction Transaction(
		FText::Format(LOCTEXT("AddReferenceTransaction", "Add reference from prim '{0}'"), FText::FromString(Referencer.GetPrimPath().GetString()))
	);

	// AddReference expects absolute file paths, so let's try ensuring that
	FString ReferencedLayerPath = OptionsPtr->TargetFile.FilePath;
	if (OptionsPtr->bInternalReference)
	{
		ReferencedLayerPath = TEXT("");
	}
	else if (FPaths::IsRelative(ReferencedLayerPath))
	{
		FString AbsoluteLayerFromBinary = FPaths::ConvertRelativePathToFull(ReferencedLayerPath);
		if (!FPaths::IsRelative(AbsoluteLayerFromBinary) && FPaths::FileExists(AbsoluteLayerFromBinary))
		{
			ReferencedLayerPath = AbsoluteLayerFromBinary;
		}
	}

	UsdUtils::AddReference(
		Referencer,
		*ReferencedLayerPath,
		TOptional<EReferencerTypeHandling>{},	 // Explicit unset optional to check project settings
		OptionsPtr->bUseDefaultPrim ? UE::FSdfPath{} : UE::FSdfPath{*OptionsPtr->TargetPrimPath},
		OptionsPtr->TimeCodeOffset,
		OptionsPtr->TimeCodeScale
	);
}

void SMyUsdStageTreeView::OnClearReferences()
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(LOCTEXT("ClearReferenceTransaction", "Clear references to USD layers"));

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		SelectedItem->ClearReferences();
	}
}

void SMyUsdStageTreeView::OnAddPayload()
{
	if (!UsdStage || !UsdStage.IsEditTargetValid())
	{
		return;
	}

	TStrongObjectPtr<UMyUsdReferenceOptions> Options = TStrongObjectPtr<UMyUsdReferenceOptions>{NewObject<UMyUsdReferenceOptions>()};
	UMyUsdReferenceOptions* OptionsPtr = Options.Get();
	if (!OptionsPtr)
	{
		return;
	}

	bool bContinue = SMyUsdOptionsWindow::ShowOptions(*OptionsPtr, LOCTEXT("AddPayloadTitle", "Add payload"), LOCTEXT("AddPayloadAccept", "OK"));
	if (!bContinue)
	{
		return;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	if (MySelectedItems.Num() != 1)
	{
		return;
	}
	UE::FMyUsdPrim Referencer = MySelectedItems[0]->UsdPrim;
	if (UsdUtils::NotifyIfInstanceProxy(Referencer))
	{
		return;
	}

	// This transaction is important as adding a payload may trigger the creation of new unreal assets, which need to be
	// destroyed if we spam undo afterwards. Undoing won't remove the actual payload from the stage yet though, sadly...
	FScopedTransaction Transaction(
		FText::Format(LOCTEXT("AddPayloadTransaction", "Add payload from prim '{0}'"), FText::FromString(Referencer.GetPrimPath().GetString()))
	);

	// AddPayload expects absolute file paths too, so let's try ensuring that
	FString ReferencedLayerPath = OptionsPtr->TargetFile.FilePath;
	if (OptionsPtr->bInternalReference)
	{
		ReferencedLayerPath = TEXT("");
	}
	else if (FPaths::IsRelative(ReferencedLayerPath))
	{
		FString AbsoluteLayerFromBinary = FPaths::ConvertRelativePathToFull(ReferencedLayerPath);
		if (!FPaths::IsRelative(AbsoluteLayerFromBinary) && FPaths::FileExists(AbsoluteLayerFromBinary))
		{
			ReferencedLayerPath = AbsoluteLayerFromBinary;
		}
	}

	UsdUtils::AddPayload(
		Referencer,
		*ReferencedLayerPath,
		TOptional<EReferencerTypeHandling>{},	 // Explicit unset optional to check project settings
		OptionsPtr->bUseDefaultPrim ? UE::FSdfPath{} : UE::FSdfPath{*OptionsPtr->TargetPrimPath},
		OptionsPtr->TimeCodeOffset,
		OptionsPtr->TimeCodeScale
	);
}

void SMyUsdStageTreeView::OnClearPayloads()
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(LOCTEXT("ClearPayloadTransaction", "Clear payloads to USD layers"));

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		SelectedItem->ClearPayloads();
	}
}

void SMyUsdStageTreeView::OnApplySchema(FName SchemaName)
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(
		FText::Format(LOCTEXT("ApplySchemaTransaction", "Apply the '{0}' schema onto selected prims"), FText::FromName(SchemaName))
	);

	UE::FSdfChangeBlock Block;

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		SelectedItem->ApplySchema(SchemaName);
	}
}

void SMyUsdStageTreeView::OnRemoveSchema(FName SchemaName)
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (UsdUtils::NotifyIfInstanceProxy(SelectedItem->UsdPrim))
		{
			return;
		}
	}

	FScopedTransaction Transaction(
		FText::Format(LOCTEXT("RemoveSchemaTransaction", "Remove the '{0}' schema from selected prims"), FText::FromName(SchemaName))
	);

	UE::FSdfChangeBlock Block;

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		SelectedItem->RemoveSchema(SchemaName);
	}
}

bool SMyUsdStageTreeView::CanApplySchema(FName SchemaName)
{
	if (!UsdStage || !UsdStage.IsEditTargetValid())
	{
		return false;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (SelectedItem->CanApplySchema(SchemaName))
		{
			return true;
		}
	}

	return false;
}

bool SMyUsdStageTreeView::CanRemoveSchema(FName SchemaName)
{
	if (!UsdStage || !UsdStage.IsEditTargetValid())
	{
		return false;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (SelectedItem->CanRemoveSchema(SchemaName))
		{
			return true;
		}
	}

	return false;
}

bool SMyUsdStageTreeView::CanAddChildPrim() const
{
	if (!UsdStage)
	{
		return false;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	// Allow adding a new top-level prim
	if (MySelectedItems.IsEmpty())
	{
		return true;
	}
	else
	{
		// We use the "rename" text input workflow to specify the target name,
		// so this doesn't work very well for multiple prims yet
		if (MySelectedItems.Num() > 1)
		{
			return false;
		}

		// If we have something selected it must be valid
		if (!MySelectedItems[0]->UsdPrim.IsValid())
		{
			return false;
		}
	}

	return true;
}

bool SMyUsdStageTreeView::CanPastePrim() const
{
	if (!UsdStage)
	{
		return false;
	}

	return UsdUtils::CanPastePrims();
}

bool SMyUsdStageTreeView::DoesPrimExistOnStage() const
{
	if (!UsdStage || !UsdStage.IsEditTargetValid())
	{
		return false;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (!SelectedItem->UsdPrim.IsPseudoRoot() && SelectedItem->UsdPrim.IsValid())
		{
			return true;
		}
	}

	return false;
}

bool SMyUsdStageTreeView::DoesPrimExistOnEditTarget() const
{
	if (!UsdStage || !UsdStage.IsEditTargetValid())
	{
		return false;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		UE::FSdfPath SpecPath = UsdUtils::GetPrimSpecPathForLayer(SelectedItem->UsdPrim, UsdStage.GetEditTarget());
		if (!SpecPath.IsAbsoluteRootPath() && !SpecPath.IsEmpty())
		{
			return true;
		}
	}

	return false;
}

bool SMyUsdStageTreeView::DoesPrimHaveSpecOnLocalLayerStack() const
{
	for (FMyUsdPrimViewModelRef SelectedItem : GetSelectedItems())
	{
		if (SelectedItem->HasSpecsOnLocalLayer())
		{
			return true;
		}
	}

	return false;
}

bool SMyUsdStageTreeView::DoesPrimHaveReferenceSpecOnLocalLayerStack() const
{
	for (FMyUsdPrimViewModelRef SelectedItem : GetSelectedItems())
	{
		if (SelectedItem->HasReferencesOnLocalLayer())
		{
			return true;
		}
	}

	return false;
}

bool SMyUsdStageTreeView::DoesPrimHavePayloadSpecOnLocalLayerStack() const
{
	for (FMyUsdPrimViewModelRef SelectedItem : GetSelectedItems())
	{
		if (SelectedItem->HasPayloadsOnLocalLayer())
		{
			return true;
		}
	}

	return false;
}

bool SMyUsdStageTreeView::DoSelectedPrimsHaveCollapsingPreference(UsdUtils::ECollapsingPreference TargetPreference) const
{
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	if (MySelectedItems.Num() == 0)
	{
		return false;
	}

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		UsdUtils::ECollapsingPreference PrimPreference = UsdUtils::GetCollapsingPreference(SelectedItem->UsdPrim);
		if (PrimPreference != TargetPreference)
		{
			return false;
		}
	}

	return true;
}

void SMyUsdStageTreeView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Restore expansion states.
	//
	// We do this on tick so that we only do at most one of these per frame, and also so that we do it as delayed
	// as possible, as during some busy transitions like undo/redo we may end up creating new FMyUsdPrimViewModels,
	// and we only want to try restoring these expansion states after all FMyUsdPrimViewModels have been created
	if (bNeedExpansionStateRefresh)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(SMyUsdStageTreeView::RestoreExpansionStates);

		// We should have only one root item, and it should be the expanded by default unless it was manually collapsed
		if (RootItems.Num() > 0)
		{
			const UE::FMyUsdPrim& RootPrim = RootItems[0]->UsdPrim;
			if (RootPrim.IsPseudoRoot())
			{
				const bool bShouldExpand = true;

				const bool bDefaultValue = true;
				if (RootWasExpanded.Get(bDefaultValue))
				{
					SetItemExpansion(RootItems[0], bShouldExpand);
				}
				else
				{
					SetItemExpansion(RootItems[0], !bShouldExpand);
				}
			}
		}

		TFunction<void(const FMyUsdPrimViewModelRef&)> SetExpansionRecursive = [&](const FMyUsdPrimViewModelRef& Item)
		{
			if (const UE::FMyUsdPrim& Prim = Item->UsdPrim)
			{
				if (ExpandedPrimPaths.Contains(Prim.GetPrimPath().GetString()))
				{
					const bool bShouldExpand = true;
					SetItemExpansion(Item, bShouldExpand);
				}
			}

			for (const FMyUsdPrimViewModelRef& Child : Item->Children)
			{
				SetExpansionRecursive(Child);
			}
		};

		for (const FMyUsdPrimViewModelRef& RootItem : RootItems)
		{
			SetExpansionRecursive(RootItem);
		}

		bNeedExpansionStateRefresh = false;
	}

	SMyUsdTreeView<FMyUsdPrimViewModelRef>::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SMyUsdStageTreeView::RequestExpansionStateRestore()
{
	bNeedExpansionStateRefresh = true;
}

void SMyUsdStageTreeView::OnToggleAllPayloads(EPayloadsTrigger PayloadsTrigger)
{
	if (!UsdStage)
	{
		return;
	}

	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();

	// Ideally we'd just use a UE::FSdfChangeBlock here, but for whatever reason this doesn't seem to affect the
	// notices USD emits when loading/unloading prim payloads, so we must do this via the UsdStage directly

	TSet<UE::FSdfPath> PrimsToLoad;
	TSet<UE::FSdfPath> PrimsToUnload;

	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		if (SelectedItem->UsdPrim)
		{
			TFunction<void(FMyUsdPrimViewModelRef)> RecursiveTogglePayloads;
			RecursiveTogglePayloads = [&RecursiveTogglePayloads, PayloadsTrigger, &PrimsToLoad, &PrimsToUnload](FMyUsdPrimViewModelRef InSelectedItem
									  ) -> void
			{
				UE::FMyUsdPrim& UsdPrim = InSelectedItem->UsdPrim;

				if (UsdPrim.HasAuthoredPayloads())
				{
					bool bPrimIsLoaded = UsdPrim.IsLoaded();

					if (PayloadsTrigger == EPayloadsTrigger::Toggle)
					{
						if (bPrimIsLoaded)
						{
							PrimsToUnload.Add(UsdPrim.GetPrimPath());
						}
						else
						{
							PrimsToLoad.Add(UsdPrim.GetPrimPath());
						}
					}
					else if (PayloadsTrigger == EPayloadsTrigger::Load && !bPrimIsLoaded)
					{
						PrimsToLoad.Add(UsdPrim.GetPrimPath());
					}
					else if (PayloadsTrigger == EPayloadsTrigger::Unload && bPrimIsLoaded)
					{
						PrimsToUnload.Add(UsdPrim.GetPrimPath());
					}
				}
				else
				{
					for (FMyUsdPrimViewModelRef Child : InSelectedItem->UpdateChildren())
					{
						RecursiveTogglePayloads(Child);
					}
				}
			};

			RecursiveTogglePayloads(SelectedItem);
		}
	}

	if (PrimsToLoad.Num() + PrimsToUnload.Num() > 0)
	{
		UE::FSdfChangeBlock GroupNotices;
		UsdStage.LoadAndUnload(PrimsToLoad, PrimsToUnload);
	}
}

void SMyUsdStageTreeView::FillDuplicateSubmenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("DuplicateFlattened_Text", "Flatten composed prim"),
		LOCTEXT("DuplicateFlattened_ToolTip", "Generate a flattened duplicate of the composed prim onto the current edit target"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnDuplicatePrim, EMyUsdDuplicateType::FlattenComposedPrim),
			FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimExistOnStage)
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DuplicateSingle_Text", "Single layer specs"),
		TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
			[this]()
			{
				return DoesPrimExistOnEditTarget()
						   ? LOCTEXT("DuplicateSingleValid_ToolTip", "Duplicate the prim's specs on the current edit target only")
						   : UE::MyUSDStageTreeView::Private::NoSpecOnLocalLayerStack;
			}
		)),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnDuplicatePrim, EMyUsdDuplicateType::SingleLayerSpecs),
			FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimExistOnEditTarget)
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DuplicateAllLocal_Text", "All local layer specs"),
		TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(
			[this]()
			{
				return DoesPrimHaveSpecOnLocalLayerStack()
						   ? LOCTEXT("DuplicateAllLocalValid_ToolTip", "Duplicate each of the prim's specs across the entire stage")
						   : UE::MyUSDStageTreeView::Private::NoSpecOnLocalLayerStack;
			}
		)),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnDuplicatePrim, EMyUsdDuplicateType::AllLocalLayerSpecs),
			FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimHaveSpecOnLocalLayerStack)
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);
}

void SMyUsdStageTreeView::FillCollapsingSubmenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AllowCollapse", "Allow collapsing"),
		LOCTEXT("AllowCollapse_ToolTip", "Allow this prim to be collapsed and to try collapsing its subtree, regardless of its kind"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnSetCollapsingPreference, UsdUtils::ECollapsingPreference::Allow),
			FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimExistOnStage),
			FIsActionChecked::CreateSP(this, &SMyUsdStageTreeView::DoSelectedPrimsHaveCollapsingPreference, UsdUtils::ECollapsingPreference::Allow)
		),
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CollapseOnKind", "Default"),
		LOCTEXT(
			"CollapseOnKind_ToolTip",
			"When 'Use prim kinds for collapsing' is enabled, prims are collapsed according to their kind (default). When disabled, prims won't be collapsed by default."
		),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnSetCollapsingPreference, UsdUtils::ECollapsingPreference::Default),
			FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimExistOnStage),
			FIsActionChecked::CreateSP(this, &SMyUsdStageTreeView::DoSelectedPrimsHaveCollapsingPreference, UsdUtils::ECollapsingPreference::Default)
		),
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NeverCollapse", "Never collapse"),
		LOCTEXT("NeverCollapse_ToolTip", "Never collapse this prim, regardless of its kind"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnSetCollapsingPreference, UsdUtils::ECollapsingPreference::Never),
			FCanExecuteAction::CreateSP(this, &SMyUsdStageTreeView::DoesPrimExistOnStage),
			FIsActionChecked::CreateSP(this, &SMyUsdStageTreeView::DoSelectedPrimsHaveCollapsingPreference, UsdUtils::ECollapsingPreference::Never)
		),
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);
}

void SMyUsdStageTreeView::FillAddSchemaSubmenu(FMenuBuilder& MenuBuilder)
{
	const UMyUsdProjectSettings* ProjectSettings = GetDefault<UMyUsdProjectSettings>();
	if (!ProjectSettings)
	{
		return;
	}

	static TArray<TSharedPtr<FString>> DefaultSchemas = {
		MakeShared<FString>(UsdToUnreal::ConvertToken(UnrealIdentifiers::ControlRigAPI)),
		MakeShared<FString>(UsdToUnreal::ConvertToken(UnrealIdentifiers::GroomAPI)),
		MakeShared<FString>(UsdToUnreal::ConvertToken(UnrealIdentifiers::GroomBindingAPI)),
		MakeShared<FString>(UsdToUnreal::ConvertToken(UnrealIdentifiers::LiveLinkAPI)),
		MakeShared<FString>(UsdToUnreal::ConvertToken(pxr::UsdShadeTokens->MaterialBindingAPI)),
		MakeShared<FString>(UsdToUnreal::ConvertToken(pxr::UsdPhysicsTokens->PhysicsCollisionAPI)),
		MakeShared<FString>(UsdToUnreal::ConvertToken(pxr::UsdPhysicsTokens->PhysicsMeshCollisionAPI)),
		MakeShared<FString>(UsdToUnreal::ConvertToken(UnrealIdentifiers::SparseVolumeTextureAPI)),
		MakeShared<FString>(UsdToUnreal::ConvertToken(pxr::UsdSkelTokens->SkelBindingAPI)),
	};

	static TArray<TSharedPtr<FString>> AllowedKnownSchemas;
	AllowedKnownSchemas.Reset(DefaultSchemas.Num() + ProjectSettings->AdditionalCustomSchemaNames.Num());

	static TSet<FString> SeenSchemas;
	SeenSchemas.Reset();
	SeenSchemas.Reserve(AllowedKnownSchemas.Num());

	// Add default list of known schemas
	for (const TSharedPtr<FString>& KnownSchema : DefaultSchemas)
	{
		SeenSchemas.Add(*KnownSchema);

		// Only show on the list the schemas that can be applied to the selected prim
		if (CanApplySchema(**KnownSchema))
		{
			AllowedKnownSchemas.Add(KnownSchema);
		}
	}

	// Add additional list of custom schemas
	for (const FString& CustomSchema : ProjectSettings->AdditionalCustomSchemaNames)
	{
		if (SeenSchemas.Contains(CustomSchema))
		{
			continue;
		}
		SeenSchemas.Add(*CustomSchema);

		// Only show on the list the schemas that can be applied to the selected prim
		if (CanApplySchema(*CustomSchema))
		{
			AllowedKnownSchemas.Add(MakeShared<FString>(CustomSchema));
		}
	}

	// Sort list for a consistent order
	AllowedKnownSchemas.Sort(
		[](const TSharedPtr<FString>& LHS, const TSharedPtr<FString>& RHS)
		{
			return *LHS < *RHS;
		}
	);

	static TSharedPtr<FString> CurrentKnownSchema = nullptr;
	if (AllowedKnownSchemas.Num() > 0)
	{
		if (!CurrentKnownSchema || !CanApplySchema(**CurrentKnownSchema))
		{
			CurrentKnownSchema = AllowedKnownSchemas[0];
		}
	}
	else
	{
		CurrentKnownSchema = nullptr;
	}

	static FText ManuallyInputText;

	static TFunction<FText()> GetCurrentSchemaNameText = []() -> FText
	{
		return !ManuallyInputText.IsEmpty() ? ManuallyInputText : CurrentKnownSchema ? FText::FromString(*CurrentKnownSchema) : FText::GetEmpty();
	};

	// clang-format off
	TSharedRef<SHorizontalBox> Box =
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(8.0f, 0.0f))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&AllowedKnownSchemas)
			.OnGenerateWidget_Lambda([&](TSharedPtr<FString> Option)
			{
				TSharedPtr<SWidget> Widget = SNullWidget::NullWidget;
				if (Option)
				{
					Widget = SNew(STextBlock)
						.Text(FText::FromString(*Option))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"));
				}

				return Widget.ToSharedRef();
			})
			.OnSelectionChanged_Lambda([](TSharedPtr<FString> ChosenOption, ESelectInfo::Type SelectInfo)
			{
				CurrentKnownSchema = ChosenOption;
				ManuallyInputText = FText::GetEmpty();
			})
			[
				SNew(SEditableTextBox)
				.Text_Lambda([]() -> FText
				{
					return GetCurrentSchemaNameText();
				})
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.OnTextChanged_Lambda([](const FText& NewText)
				{
					ManuallyInputText = NewText;
				})
			]
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(8.0f, 0.0f))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddSchemaButtonText", "Add"))
			.ToolTipText(LOCTEXT("AddSchemaButtonToolTip", "Adds the currently selected schema to the prim"))
			.IsEnabled_Lambda([this]()->bool
			{
				FName SelectedSchema = *GetCurrentSchemaNameText().ToString();
				return CanApplySchema(SelectedSchema);
			})
			.OnClicked_Lambda([this]() -> FReply
			{
				FText SelectedSchemaText = GetCurrentSchemaNameText();
				FString SelectedSchemaString = SelectedSchemaText.ToString();
				FName SelectedSchemaName = *SelectedSchemaString;

				if (CanApplySchema(SelectedSchemaName))
				{
					OnApplySchema(SelectedSchemaName);
				}

				if (!SeenSchemas.Contains(SelectedSchemaString))
				{
					UMyUsdProjectSettings* ProjectSettings = GetMutableDefault<UMyUsdProjectSettings>();
					if (ProjectSettings)
					{
						ProjectSettings->AdditionalCustomSchemaNames.AddUnique(SelectedSchemaText.ToString());
						ProjectSettings->SaveConfig();
					}
				}

				return FReply::Handled();
			})
			.ButtonStyle(&FAppStyle::Get(), "PrimaryButton")
		];
	// clang-format on

	const bool bNoIndent = true;
	MenuBuilder.AddWidget(Box, FText::GetEmpty(), bNoIndent);
}

void SMyUsdStageTreeView::FillRemoveSchemaSubmenu(FMenuBuilder& MenuBuilder)
{
	TSet<FString> RemovableSchemas;
	TArray<FMyUsdPrimViewModelRef> MySelectedItems = GetSelectedItems();
	for (FMyUsdPrimViewModelRef SelectedItem : MySelectedItems)
	{
		TArray<FName> Schemas = SelectedItem->UsdPrim.GetAppliedSchemas();
		for (const FName& Schema : Schemas)
		{
			RemovableSchemas.Add(Schema.ToString());
		}
	}

	TArray<FString> SortedRemovableSchemas = RemovableSchemas.Array();
	SortedRemovableSchemas.Sort();

	for (const FString& Schema : SortedRemovableSchemas)
	{
		if (!SMyUsdStageTreeView::CanRemoveSchema(*Schema))
		{
			continue;
		}

		MenuBuilder.AddMenuEntry(
			FText::FromString(Schema),
			FText::Format(LOCTEXT("RemoveSchemaToolTip", "Remove schema '{0}'"), FText::FromString(Schema)),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SMyUsdStageTreeView::OnRemoveSchema, FName(*Schema))),
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}
}

FReply SMyUsdStageTreeView::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (UICommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return SMyUsdTreeView::OnKeyDown(MyGeometry, InKeyEvent);
}

void SMyUsdStageTreeView::ScrollItemIntoView(FMyUsdPrimViewModelRef TreeItem)
{
	FMyUsdPrimViewModel* Parent = TreeItem->ParentItem;
	while (Parent)
	{
		SetItemExpansion(StaticCastSharedRef<FMyUsdPrimViewModel>(Parent->AsShared()), true);
		Parent = Parent->ParentItem;
	}

	RequestScrollIntoView(TreeItem);
}

void SMyUsdStageTreeView::OnTreeItemScrolledIntoView(FMyUsdPrimViewModelRef TreeItem, const TSharedPtr<ITableRow>& Widget)
{
	if (TreeItem == PendingRenameItem.Pin())
	{
		PendingRenameItem = nullptr;
		TreeItem->RenameRequestEvent.ExecuteIfBound();
	}
}

void SMyUsdStageTreeView::OnPrimNameCommitted(const FMyUsdPrimViewModelRef& ViewModel, const FText& InPrimName)
{
	// Reset this regardless of how we exit this function
	const bool bRenamingExistingPrim = ViewModel->bIsRenamingExistingPrim;
	ViewModel->bIsRenamingExistingPrim = false;

	// Escaped out of initially setting a prim name
	TFunction<void()> CancelInput = [&ViewModel, this]()
	{
		if (!ViewModel->UsdPrim)
		{
			if (FMyUsdPrimViewModel* Parent = ViewModel->ParentItem)
			{
				ViewModel->ParentItem->Children.Remove(ViewModel);
			}
			else
			{
				RootItems.Remove(ViewModel);
			}

			RequestTreeRefresh();
		}
	};

	if (InPrimName.IsEmptyOrWhitespace())
	{
		CancelInput();
		return;
	}

	if (bRenamingExistingPrim)
	{
		if (UsdUtils::NotifyIfInstanceProxy(ViewModel->UsdPrim))
		{
			CancelInput();
			return;
		}

		FScopedTransaction Transaction(LOCTEXT("RenamePrimTransaction", "Rename a prim"));

		// e.g. "/Root/OldPrim"
		FString OldPath = ViewModel->UsdPrim.GetPrimPath().GetString();

		// e.g. "NewPrim"
		FString NewNameStr = InPrimName.ToString();

		// Preemptively preserve the prim's expansion state because RenamePrim will trigger notices from within itself
		// that will trigger refreshes of the tree view.
		//
		// Note: We don't remove the old paths in here, as that lets us undo the rename and
		// preserve our expansion states
		{
			// e.g. "/Root/NewPrim"
			FString NewPath = FString::Printf(TEXT("%s/%s"), *FPaths::GetPath(OldPath), *NewNameStr);
			TSet<FString> EntriesToAdd;
			for (TSet<FString>::TIterator It(ExpandedPrimPaths); It; ++It)
			{
				// e.g. "/Root/OldPrim/SomeChild"
				TStringView SomePrimPath = *It;
				if (SomePrimPath.StartsWith(OldPath))
				{
					// e.g. "/SomeChild"
					SomePrimPath.RemovePrefix(OldPath.Len());

					// e.g. "/Root/NewPrim/SomeChild"
					EntriesToAdd.Add(NewPath + FString{SomePrimPath});
				}
			}
			ExpandedPrimPaths.Append(EntriesToAdd);
		}

		UsdUtils::RenamePrim(ViewModel->UsdPrim, *NewNameStr);
	}
	else
	{
		if (FMyUsdPrimViewModel* ParentModel = ViewModel->ParentItem)
		{
			if (UsdUtils::NotifyIfInstanceProxy(ParentModel->UsdPrim))
			{
				CancelInput();
				return;
			}
		}

		FScopedTransaction Transaction(LOCTEXT("AddPrimTransaction", "Add a new prim"));

		ViewModel->DefinePrim(*InPrimName.ToString());

		const bool bResync = true;

		// Renamed a child item
		if (ViewModel->ParentItem)
		{
			ViewModel->ParentItem->Children.Remove(ViewModel);

			RefreshPrim(ViewModel->ParentItem->UsdPrim.GetPrimPath().GetString(), bResync);
		}
		// Renamed a root item
		else
		{
			RefreshPrim(ViewModel->UsdPrim.GetPrimPath().GetString(), bResync);
		}
	}
}

void SMyUsdStageTreeView::OnPrimNameUpdated(const FMyUsdPrimViewModelRef& TreeItem, const FText& InPrimName, FText& ErrorMessage)
{
	FString NameStr = InPrimName.ToString();
	IMyUsdPrim::IsValidPrimName(NameStr, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	{
		const UE::FMyUsdStageWeak& Stage = TreeItem->UsdStage;
		if (!Stage)
		{
			return;
		}

		UE::FSdfPath ParentPrimPath;
		if (TreeItem->ParentItem)
		{
			ParentPrimPath = TreeItem->ParentItem->UsdPrim.GetPrimPath();
		}
		else
		{
			ParentPrimPath = UE::FSdfPath::AbsoluteRootPath();
		}

		UE::FSdfPath NewPrimPath = ParentPrimPath.AppendChild(*NameStr);
		const UE::FMyUsdPrim& Prim = Stage.GetPrimAtPath(NewPrimPath);
		if (Prim && Prim != TreeItem->UsdPrim)
		{
			ErrorMessage = LOCTEXT("DuplicatePrimName", "A Prim with this name already exists!");
			return;
		}
	}
}

#endif	  // #if USE_USD_SDK

#undef LOCTEXT_NAMESPACE
