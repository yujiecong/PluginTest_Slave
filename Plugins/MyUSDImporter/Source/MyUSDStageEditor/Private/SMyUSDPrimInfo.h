// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

#include "UsdWrappers/ForwardDeclarations.h"

#if USE_USD_SDK

class SMyUsdPrimInfo : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMyUsdPrimInfo)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetPrimPath(const UE::FUsdStageWeak& UsdStage, const TCHAR* PrimPath);

private:
	friend class SMyUsdStage;
	TSharedPtr<class SMyUsdObjectFieldList> PropertiesList;
	TSharedPtr<class SMyUsdObjectFieldList> PropertyMetadataPanel;
	TSharedPtr<class SMyUsdIntegrationsPanel> IntegrationsPanel;
	TSharedPtr<class SVariantsList> VariantsList;
	TSharedPtr<class SMyUsdReferencesList> ReferencesList;
};

#endif	  // #if USE_USD_SDK
