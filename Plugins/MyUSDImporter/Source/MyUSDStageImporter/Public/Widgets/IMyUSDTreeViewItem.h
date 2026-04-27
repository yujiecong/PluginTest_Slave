// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"

class IMyUsdTreeViewItem : public TSharedFromThis<IMyUsdTreeViewItem>
{
public:
	virtual ~IMyUsdTreeViewItem() = default;
};
