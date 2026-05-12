# UEMotion Axis 蓝图、材质和黑背景修复计划

## 问题分析

### 问题 1: Axis BP 中的 StaticMesh 为空
**现状**: 当前 `SetupCoordinateAxes()` 使用 `UBlueprintFactory` 创建蓝图时，没有配置 SCS (SimpleConstructionScript)，导致蓝图中 StaticMeshComponent 没有默认的 mesh 和材质配置。

**根因**: 缺少类似 `UEMotionAssetFactory::CreateAndSaveBlueprintAsset()` 中的 SCS 配置代码（第 224-253 行）。

### 问题 2: 未生成 Axis 材质 UAsset
**现状**: `CreateStaticAxisMaterial()` 方法存在且被调用，但材质可能未正确保存或路径问题。

**需验证**: 
- 材质保存路径 `/Game/UEMotion/Materials/M_Axis_X` 是否正确
- `FEditorFileUtils::PromptForCheckoutAndSave()` 是否成功执行
- 材质实例参数设置是否生效

### 问题 3: 缺少纯黑背景 Floor
**需求**: 类似 Manim 的纯黑背景，需要一个大的黑色平面作为场景底部/背景板。

---

## 实施步骤

### 步骤 1: 修复 Axis 蓝图生成 - 配置 SCS
**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp`

**修改位置**: `SetupCoordinateAxes()` 方法中的蓝图创建部分（第 235-260 行）

**实施方案**:
```cpp
// 在创建蓝图后、保存前，添加 SCS 配置：
if (NewBP)
{
    // === 新增: 配置 SimpleConstructionScript ===
    USimpleConstructionScript* SCS = NewBP->SimpleConstructionScript;
    if (SCS)
    {
        // 1. 创建 Root 节点
        USCS_Node* RootNode = SCS->CreateNode(USceneComponent::StaticClass(), TEXT("DefaultSceneRoot"));
        SCS->AddNode(RootNode);
        
        // 2. 创建 Mesh 节点
        USCS_Node* MeshNode = SCS->CreateNode(UStaticMeshComponent::StaticClass(), TEXT("Mesh"));
        RootNode->AddChildNode(MeshNode);
        
        // 3. 配置 Mesh 模板
        UStaticMeshComponent* MeshTemplate = Cast<UStaticMeshComponent>(MeshNode->ComponentTemplate);
        if (MeshTemplate)
        {
            // 设置 GizmoArrowHandle 网格
            UStaticMesh* GizmoMesh = LoadObject<UStaticMesh>(
                nullptr, TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle"));
            if (GizmoMesh)
            {
                MeshTemplate->SetStaticMesh(GizmoMesh);
            }
            
            // 设置材质（使用已创建的轴材质）
            FString MaterialPath = FString::Printf(TEXT("/Game/UEMotion/Materials/M_Axis_%s"), 
                *BPName.RightChop(7)); // BP_Axis_X -> M_Axis_X
            UMaterialInterface* AxisMat = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
            if (AxisMat)
            {
                MeshTemplate->SetMaterial(0, AxisMat);
            }
            
            // 设置碰撞
            MeshTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            MeshTemplate->CastShadow = false;
        }
    }
    
    // 编译蓝图
    FKismetEditorUtilities::CompileBlueprint(NewBP);
    
    // ... 原有的保存逻辑 ...
}
```

**关键点**:
- 参考 `UEMotionAssetFactory::CreateAndSaveBlueprintAsset()` 第 224-255 行的实现
- 必须调用 `FKismetEditorUtilities::CompileBlueprint(NewBP)` 编译蓝图
- 需要在材质创建完成后才能引用材质到蓝图中

### 步骤 2: 确保 Axis 材质正确生成并保存
**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionAxisActor.cpp`

**修改位置**: `CreateStaticAxisMaterial()` 方法（第 180-236 行）

**验证与修复**:
1. **检查材质保存逻辑**: 当前使用 `FEditorFileUtils::PromptForCheckoutAndSave()` 可能会弹出对话框导致失败
2. **改用直接保存方式**:
```cpp
// 替换第 228-230 行:
TArray<UPackage*> PackagesToSave;
PackagesToSave.Add(MaterialPackage);
FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, true);

// 改为:
FString FilePath = FPaths::Combine(
    FPaths::ProjectContentDir(), 
    TEXT("UEMotion/Materials/"), 
    MaterialName + TEXT(".uasset"));

UPackage::SavePackage(
    MaterialPackage, 
    nullptr, 
    ESaveFlags::RF_Public | RF_Standalone, 
    *FilePath);

UE_LOG(LogTemp, Log, TEXT("UEMotionAxisActor: Material saved to '%s'"), *FilePath);
```

3. **确保材质目录存在**: 在创建前检查/创建目录
4. **验证材质参数**: 确认 `BaseColor` 参数名在 `BasicShapeMaterial` 中存在

### 步骤 3: 创建黑色背景 Floor
**新增方法**: `SetupBlackBackgroundFloor()`

**文件**: 
- `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.h` - 声明方法
- `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp` - 实现方法

**实施方案**:
```cpp
void UUEMotionScene::SetupBlackBackgroundFloor()
{
    if (!SceneWorld.IsValid()) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bNoFail = true;

    // 1. 创建 Floor Actor
    AActor* FloorActor = SceneWorld->SpawnActor<AActor>(
        FVector(0, 0, -0.01), // 略低于原点避免 z-fighting
        FRotator(90, 0, 0),   // 平躺
        SpawnParams);

    if (FloorActor)
    {
        // 2. 添加 StaticMeshComponent
        UStaticMeshComponent* FloorMesh = NewObject<UStaticMeshComponent>(FloorActor);
        if (FloorMesh)
        {
            FloorMesh->RegisterComponent();
            FloorMesh->AttachToComponent(FloorActor->GetRootComponent(), 
                FAttachmentTransformRules::KeepRelativeTransform);

            // 3. 使用 Plane 网格
            UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(
                nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
            
            if (PlaneMesh)
            {
                FloorMesh->SetStaticMesh(PlaneMesh);
                
                // 4. 缩放以覆盖整个视野 (8x8 units * SCALE_FACTOR)
                float FloorSize = FRAME_WIDTH * SCALE_FACTOR; // 400 cm
                FloorMesh->SetWorldScale3D(FVector(FloorSize / 200.0f)); // Plane 默认 200x200
                
                // 5. 设置无碰撞、不投射阴影
                FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                FloorMesh->CastShadow = false;
                FloorMesh->bReceivesDecals = false;

                // 6. 应用纯黑材质
                UMaterialInterface* BlackMaterial = CreateOrLoadBlackMaterial();
                if (BlackMaterial)
                {
                    FloorMesh->SetMaterial(0, BlackMaterial);
                }

                UE_LOG(LogTemp, Log, 
                    TEXT("UEMotionScene: Created black background floor (%.0f x %.0f cm)"),
                    FloorSize, FloorSize);
            }
        }
    }
}
```

**辅助方法**: `CreateOrLoadBlackMaterial()`
```cpp
UMaterialInterface* UUEMotionScene::CreateOrLoadBlackMaterial()
{
    const FString MaterialPath = TEXT("/Game/UEMotion/Materials/M_BlackBackground");
    
    // 尝试加载已存在的材质
    if (UEditorAssetLibrary::DoesAssetExist(MaterialPath))
    {
        return LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    }

    // 创建新的黑色材质实例
    UPackage* Package = CreatePackage(*MaterialPath);
    if (!Package) return nullptr;

    UMaterialInstanceConstant* BlackMIC = NewObject<UMaterialInstanceConstant>(
        Package, TEXT("M_BlackBackground"), RF_Public | RF_Standalone);
    
    if (!BlackMIC) return nullptr;

    // 基于 BasicShapeMaterial 创建
    UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    
    if (BaseMat)
    {
        BlackMIC->SetParentEditorOnly(BaseMat);
        
#if WITH_EDITOR
        BlackMIC->SetVectorParameterValueEditorOnly(FName("BaseColor"), FLinearColor::Black);
        BlackMIC->SetScalarParameterValueEditorOnly(FName("Opacity"), 1.0f);
#endif
    }

    // 保存材质
    FAssetRegistryModule::AssetCreated(BlackMIC);
    Package->MarkPackageDirty();

    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add(Package);
    FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, true);

    return BlackMIC;
}
```

### 步骤 4: 更新 Scene 初始化流程
**文件**: `UEMotionScene.cpp` - `Initialize()` 方法

**当前顺序**:
```cpp
SetupDefaultLighting();      // 光照
SetupCoordinateAxes();       // 坐标轴
SetupSkyEnvironment();       // 天空环境
```

**新顺序**:
```cpp
SetupDefaultLighting();      // 1. 光照
SetupSkyEnvironment();       // 2. 天空环境（可选）
SetupBlackBackgroundFloor(); // 3. 黑色背景地板
SetupCoordinateAxes();       // 4. 坐标轴（最上层）
```

**理由**: 
- 先创建底层背景
- 再创建坐标轴覆盖在上面
- 确保渲染层级正确

### 步骤 5: 头文件声明更新
**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.h`

**新增声明**:
```cpp
private:
    void SetupBlackBackgroundFloor();
    UMaterialInterface* CreateOrLoadBlackMaterial();
```

**新增头文件包含** (UEMotionScene.cpp):
```cpp
#include "Kismet2/BlueprintEditorLibrary.h"  // FKismetEditorUtilities
```

---

## 文件修改清单

| 文件 | 修改类型 | 内容 |
|------|---------|------|
| `UEMotionScene.h` | 修改 | 新增 `SetupBlackBackgroundFloor()` 和 `CreateOrLoadBlackMaterial()` 声明 |
| `UEMotionScene.cpp` | 修改 | 1. 完善 Axis BP 的 SCS 配置<br>2. 修复材质保存逻辑<br>3. 实现 `SetupBlackBackgroundFloor()`<br>4. 实现 `CreateOrLoadBlackMaterial()`<br>5. 更新 Initialize() 调用顺序 |
| `UEMotionAxisActor.cpp` | 修改 | 修复 `CreateStaticAxisMaterial()` 的保存方式 |

---

## 预期结果

### ✅ 问题 1 解决
- Axis 蓝图中包含完整的 StaticMeshComponent 配置
- 默认使用 GizmoArrowHandle 网格 + 对应颜色的材质
- 蓝图可在编辑器中正常预览和编辑

### ✅ 问题 2 解决
- 三个轴材质 (M_Axis_X/Y/Z) 作为 `.uasset` 保存在 `/Game/UEMotion/Materials/`
- 材质基于 BasicShapeMaterial，颜色分别为红/绿/蓝
- 后续运行可复用已有材质

### ✅ 问题 3 解决
- 场景底部出现纯黑色平面作为背景
- 平面尺寸为 8x8 UEMotion Units (400x400 cm)
- 不参与碰撞、不投射阴影、不接收贴花
- 类似 Manim 的 DarkTheme 效果

---

## 测试验证

编译通过后运行 `cube_basic_animations.py`，验证：
1. Outliner 中显示 `BP_Axis_X`, `BP_Axis_Y`, `BP_Axis_Z` actor（非空 mesh）
2. Content Browser 中 `/Game/UEMotion/Materials/` 下有 `M_Axis_X.uasset` 等 3 个材质
3. 场景中可见黑色背景平面
4. 坐标轴颜色鲜艳（红 X / 绿 Y / 蓝 Z）且位于背景之上
