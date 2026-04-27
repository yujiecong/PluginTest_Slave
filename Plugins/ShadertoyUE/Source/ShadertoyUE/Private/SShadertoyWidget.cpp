#include "SShadertoyWidget.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCompiler.h"
#include "ShaderCore.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTime.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"

void SShadertoyWidget::Construct(const FArguments& InArgs)
{
	// Load default shader text
	FString ShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ShadertoyUE"))->GetBaseDir(), TEXT("Shaders"));
	FString ShaderPath = FPaths::Combine(ShaderDir, TEXT("ShadertoyMain.ush"));
	FString ShaderText;
	FFileHelper::LoadFileToString(ShaderText, *ShaderPath);

	// Load target material
	TargetMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/M_ShadertoyBridge.M_ShadertoyBridge"));
	
	if (!MaterialBrush.IsValid())
	{
		MaterialBrush = MakeShareable(new FSlateBrush());
		MaterialBrush->ImageSize = FVector2D(512.f, 512.f);
	}

	if (TargetMaterial)
	{
		MaterialBrush->SetResourceObject(TargetMaterial);
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.f)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Compile")))
			.OnClicked(this, &SShadertoyWidget::OnCompileClicked)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				SAssignNew(CodeEditor, SMultiLineEditableText)
				.Text(FText::FromString(ShaderText))
			]
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				SNew(SImage)
				.Image(MaterialBrush.IsValid() ? MaterialBrush.Get() : nullptr)
			]
		]
	];
}

FReply SShadertoyWidget::OnCompileClicked()
{
	if (CodeEditor.IsValid())
	{
		FString Code = CodeEditor->GetText().ToString();
		FString ShaderPath = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ShadertoyUE"))->GetBaseDir(), TEXT("Shaders"), TEXT("ShadertoyMain.ush"));
		
		FFileHelper::SaveStringToFile(Code, *ShaderPath);

		// Flush shader cache
		FlushShaderFileCache();

		if (!TargetMaterial)
		{
			TargetMaterial = EnsureTargetMaterialExists();
		}

		if (TargetMaterial)
		{
			if (MaterialBrush.IsValid() && MaterialBrush->GetResourceObject() != TargetMaterial)
			{
				MaterialBrush->SetResourceObject(TargetMaterial);
			}

			// Force recompile
			TargetMaterial->ForceRecompileForRendering();
		}
	}

	return FReply::Handled();
}

UMaterial* SShadertoyWidget::EnsureTargetMaterialExists()
{
	FString PackageName = TEXT("/Game/M_ShadertoyBridge");
	FString MaterialName = TEXT("M_ShadertoyBridge");

	UMaterial* ExistingMaterial = LoadObject<UMaterial>(nullptr, *(PackageName + TEXT(".") + MaterialName));
	if (ExistingMaterial)
	{
		return ExistingMaterial;
	}

	// Create new material
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	
	UObject* NewAsset = AssetTools.CreateAsset(MaterialName, TEXT("/Game"), UMaterial::StaticClass(), MaterialFactory);
	UMaterial* NewMaterial = Cast<UMaterial>(NewAsset);

	if (NewMaterial)
	{
		// Set Domain to User Interface
		NewMaterial->MaterialDomain = MD_UI;

		// Create Custom Expression
		UMaterialExpressionCustom* CustomExpr = NewObject<UMaterialExpressionCustom>(NewMaterial);
		CustomExpr->Code = TEXT("return mainImage(UV, Time);");
		CustomExpr->IncludeFilePaths.Add(TEXT("/ProjectShaders/ShadertoyMain.ush"));
		CustomExpr->Inputs.SetNum(2);
		CustomExpr->Inputs[0].InputName = TEXT("UV");
		CustomExpr->Inputs[1].InputName = TEXT("Time");
		
		NewMaterial->GetExpressionCollection().AddExpression(CustomExpr);

		// Create TextureCoordinate Expression
		UMaterialExpressionTextureCoordinate* TexCoordExpr = NewObject<UMaterialExpressionTextureCoordinate>(NewMaterial);
		NewMaterial->GetExpressionCollection().AddExpression(TexCoordExpr);

		// Create Time Expression
		UMaterialExpressionTime* TimeExpr = NewObject<UMaterialExpressionTime>(NewMaterial);
		NewMaterial->GetExpressionCollection().AddExpression(TimeExpr);

		// Connect TexCoord -> Custom.UV
		CustomExpr->Inputs[0].Input.Expression = TexCoordExpr;
		
		// Connect Time -> Custom.Time
		CustomExpr->Inputs[1].Input.Expression = TimeExpr;

		// Connect Custom -> FinalColor (EmissiveColor for UI)
		NewMaterial->GetEditorOnlyData()->EmissiveColor.Expression = CustomExpr;

		// Save and Compile
		NewMaterial->PreEditChange(nullptr);
		NewMaterial->PostEditChange();
		NewMaterial->MarkPackageDirty();
	}

	return NewMaterial;
}