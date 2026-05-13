#include "Core/UEMotionMorphMaterialManager.h"
#include "Core/UEMotionMobject.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

UUEMotionMorphMaterialManager* UUEMotionMorphMaterialManager::Instance = nullptr;

UUEMotionMorphMaterialManager* UUEMotionMorphMaterialManager::Get()
{
	if (!Instance)
	{
		Instance = NewObject<UUEMotionMorphMaterialManager>();
		Instance->AddToRoot();
	}
	return Instance;
}

UMaterialInterface* UUEMotionMorphMaterialManager::GetOrCreateMorphMaterial(
	UUEMotionMobject* SourceMobject,
	const TArray<FVector>& SourceVertices,
	const TArray<FVector>& TargetVertices)
{
	if (!SourceMobject || SourceVertices.Num() == 0 || TargetVertices.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionMorphMaterialManager: Invalid parameters"));
		return nullptr;
	}

	UMaterialInterface* ParentMaterial = CreateMorphParentMaterial();
	if (!ParentMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionMorphMaterialManager: Failed to create parent material"));
		return nullptr;
	}

	UMaterialInstanceDynamic* MorphMIC = CreateMorphMaterialInstance(
		ParentMaterial,
		SourceVertices,
		TargetVertices
	);

	if (MorphMIC)
	{
		ActiveMorphMaterials.Add(SourceMobject, MorphMIC);
		return MorphMIC;
	}

	return nullptr;
}

bool UUEMotionMorphMaterialManager::ApplyMorphToMobject(
	UUEMotionMobject* Mobject,
	float MorphProgress)
{
	if (!Mobject) return false;

	AActor* Actor = Mobject->GetInternalActor();
	if (!Actor) return false;

	UStaticMeshComponent* MeshComp = Actor->FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp) return false;

	UMaterialInstanceDynamic** ExistingMIC = ActiveMorphMaterials.Find(Mobject);
	if (ExistingMIC && *ExistingMIC)
	{
		MeshComp->SetMaterial(0, *ExistingMIC);
		SetMorphProgress(Mobject, MorphProgress);
		return true;
	}

	return false;
}

void UUEMotionMorphMaterialManager::SetMorphProgress(UUEMotionMobject* Mobject, float Progress)
{
	if (!Mobject) return;

	UMaterialInstanceDynamic** MIC = ActiveMorphMaterials.Find(Mobject);
	if (MIC && *MIC)
	{
		(*MIC)->SetScalarParameterValue(FName("MorphProgress"), FMath::Clamp(Progress, 0.0f, 1.0f));
	}
}

UMaterialInterface* UUEMotionMorphMaterialManager::CreateMorphParentMaterial()
{
	if (CachedParentMaterial) return CachedParentMaterial;

	UPackage* Package = CreatePackage(TEXT("/Game/UEMotion/Materials/M_MorphParent"));
	if (!Package) return nullptr;

	UMaterial* Material = NewObject<UMaterial>(
		Package, TEXT("M_MorphParent"), RF_Public | RF_Standalone);

	if (!Material) return nullptr;

	Material->BlendMode = EBlendMode::BLEND_Translucent;
	Material->TwoSided = true;

	UMaterialExpressionScalarParameter* MorphProgressParam = NewObject<UMaterialExpressionScalarParameter>(Material);
	MorphProgressParam->ParameterName = FName("MorphProgress");
	MorphProgressParam->DefaultValue = 0.0f;
	Material->GetExpressionCollection().AddExpression(MorphProgressParam);

	UMaterialExpressionVectorParameter* BaseColorParam = NewObject<UMaterialExpressionVectorParameter>(Material);
	BaseColorParam->ParameterName = FName("BaseColor");
	BaseColorParam->DefaultValue = FLinearColor::White;
	Material->GetExpressionCollection().AddExpression(BaseColorParam);

	UMaterialExpressionScalarParameter* OpacityParam = NewObject<UMaterialExpressionScalarParameter>(Material);
	OpacityParam->ParameterName = FName("Opacity");
	OpacityParam->DefaultValue = 1.0f;
	Material->GetExpressionCollection().AddExpression(OpacityParam);

	UMaterialExpressionScalarParameter* SourceRadiusParam = NewObject<UMaterialExpressionScalarParameter>(Material);
	SourceRadiusParam->ParameterName = FName("SourceShapeRadius");
	SourceRadiusParam->DefaultValue = 1.0f;
	Material->GetExpressionCollection().AddExpression(SourceRadiusParam);

	UMaterialExpressionScalarParameter* TargetRadiusParam = NewObject<UMaterialExpressionScalarParameter>(Material);
	TargetRadiusParam->ParameterName = FName("TargetShapeRadius");
	TargetRadiusParam->DefaultValue = 1.0f;
	Material->GetExpressionCollection().AddExpression(TargetRadiusParam);

	UMaterialExpressionScalarParameter* SourceSidesParam = NewObject<UMaterialExpressionScalarParameter>(Material);
	SourceSidesParam->ParameterName = FName("SourceShapeSides");
	SourceSidesParam->DefaultValue = 4.0f;
	Material->GetExpressionCollection().AddExpression(SourceSidesParam);

	UMaterialExpressionConstant* PiConst = NewObject<UMaterialExpressionConstant>(Material);
	PiConst->R = PI;
	Material->GetExpressionCollection().AddExpression(PiConst);

	UMaterialExpressionConstant* TwoConst = NewObject<UMaterialExpressionConstant>(Material);
	TwoConst->R = 2.0f;
	Material->GetExpressionCollection().AddExpression(TwoConst);

#if WITH_EDITORONLY_DATA
	UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
	if (EditorData)
	{
		EditorData->BaseColor.Expression = BaseColorParam;
		EditorData->BaseColor.OutputIndex = 0;
		EditorData->Opacity.Expression = OpacityParam;
		EditorData->Opacity.OutputIndex = 0;
		EditorData->WorldPositionOffset.Expression = MorphProgressParam;
		EditorData->WorldPositionOffset.OutputIndex = 0;
	}
#endif

	Material->SetFlags(RF_Public | RF_Standalone);
	Package->MarkPackageDirty();

#if WITH_EDITOR
	Material->PostEditChange();
#endif

	CachedParentMaterial = Material;

	UE_LOG(LogTemp, Log, TEXT("UEMotionMorphMaterialManager: Created morph parent material"));

	return Material;
}

UMaterialInstanceDynamic* UUEMotionMorphMaterialManager::CreateMorphMaterialInstance(
	UMaterialInterface* ParentMaterial,
	const TArray<FVector>& SourceVertices,
	const TArray<FVector>& TargetVertices)
{
	if (!ParentMaterial) return nullptr;

	UMaterialInstanceDynamic* MIC = UMaterialInstanceDynamic::Create(ParentMaterial, this);
	if (!MIC) return nullptr;

	int32 SourceVertCount = SourceVertices.Num();
	int32 TargetVertCount = TargetVertices.Num();

	FVector SourceCenter = FVector::ZeroVector;
	FVector TargetCenter = FVector::ZeroVector;
	float SourceMaxDist = 0.0f;
	float TargetMaxDist = 0.0f;

	for (const FVector& V : SourceVertices)
	{
		SourceCenter += V;
		float Dist = V.Size();
		if (Dist > SourceMaxDist) SourceMaxDist = Dist;
	}
	if (SourceVertCount > 0) SourceCenter /= (float)SourceVertCount;

	for (const FVector& V : TargetVertices)
	{
		TargetCenter += V;
		float Dist = V.Size();
		if (Dist > TargetMaxDist) TargetMaxDist = Dist;
	}
	if (TargetVertCount > 0) TargetCenter /= (float)TargetVertCount;

	MIC->SetScalarParameterValue(FName("SourceShapeRadius"), FMath::Max(SourceMaxDist, 0.01f));
	MIC->SetScalarParameterValue(FName("TargetShapeRadius"), FMath::Max(TargetMaxDist, 0.01f));
	MIC->SetScalarParameterValue(FName("SourceShapeSides"), (float)SourceVertCount);

	UE_LOG(LogTemp, Log,
		TEXT("UEMotionMorphMaterialManager: Created morph material instance")
		TEXT("\n  Source: %d verts, radius=%.2f")
		TEXT("\n  Target: %d verts, radius=%.2f"),
		SourceVertCount, SourceMaxDist,
		TargetVertCount, TargetMaxDist);

	return MIC;
}
