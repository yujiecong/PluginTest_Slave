// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Widgets/Input/SComboBox.h"

class UMyUsdStageImportOptions;
class IDetailLayoutBuilder;

class FMyUsdStageImportOptionsCustomization : public IDetailCustomization
{
public:
	FMyUsdStageImportOptionsCustomization();
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
	// End of IDetailCustomization interface

private:
	void OnComboBoxOpening();
	void OnComboBoxSelectionChanged(TSharedPtr<FString> NewContext, ESelectInfo::Type SelectType);
	FText GetComboBoxSelectedOptionText() const;
	void ForceRefreshDetails();

	UMyUsdStageImportOptions* CurrentOptions;

	TSharedPtr<SComboBox<TSharedPtr<FString>>> RenderContextComboBox;
	TArray<TSharedPtr<FString>> RenderContextComboBoxItems;

	TArray<TSharedPtr<FString>> MaterialPurposeComboBoxItems;
};

#endif	  // WITH_EDITOR
