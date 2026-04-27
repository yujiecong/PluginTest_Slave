// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/IMyUSDTreeViewItem.h"

enum class EMyUsdDragDropOpType
{
	None,
	Prims,
	Layers,
	Attributes
};

class FMyUsdDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMyUsdDragDropOp, FDecoratedDragDropOp)

	EMyUsdDragDropOpType OpType;
	TSet<TSharedRef<IMyUsdTreeViewItem>> DraggedItems;
};
