# UEMotion 材质 UAsset 创建完整流程

> 基于 UE 5.7 + C++ 实践总结 | 2026-05-13

---

## 目录

1. [核心原则](#1-核心原则)
2. [架构概览](#2-架构概览)
3. [Step 1: 创建父材质 (UMaterial)](#3-step-1-创建父材质-umaterial)
4. [Step 2: 创建实例材质 (MaterialInstanceConstant)](#4-step-2-创建实例材质-materialinstanceconstant)
5. [Step 3: 创建蓝图并赋值材质到 SCS](#5-step-3-创建蓝图并赋值材质到-scs)
6. [Step 4: Spawn Actor 并应用材质](#6-step-4-spawn-actor-并应用材质)
7. [UE 5.7 关键 API 差异](#7-ue-57-关键-api-差异)
8. [常见陷阱与解决方案](#8-常见陷阱与解决方案)
9. [完整代码模板](#9-完整代码模板)

---

## 1. 核心原则

### 铁律：一切皆 UAsset，禁止运行时动态生成

```
优先级排序：
1. 🥇 从 Content Browser 加载已有 UAsset（最快最稳）
2. 🥈 创建新 UAsset 并保存到磁盘（可复用）
3. 🉐 运行时动态创建（仅用于测试/临时对象）
4. ❌ 禁止：动态对象不保存就使用（必出 bug）
```

### 为什么必须用 UAsset？

| 特性 | UAsset（静态） | 动态创建 |
|------|---------------|---------|
| 编辑器可见性 | ✅ Content Browser 显示 | ❌ 内存幽灵对象 |
| 蓝图引用 | ✅ SCS 自动记录 | ❌ 每次运行都丢失 |
| 参数调整 | ✅ 双击即可编辑 | ❌ 必须改代码重新编译 |
| 团队协作 | ✅ 提交 Git 即可共享 | ❌ 别人电脑上没有 |
| 调试难度 | 🔴 简单 | 🔴🔴🔴 极难 |

---

## 2. 架构概览

### 材质层级结构

```
┌─────────────────────────────────────────────────────────────┐
│                    材质继承链                                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   M_BlackFloorParent (UMaterial)                            │
│   ├── 类型: 父材质（基础材质）                               │
│   ├── 包含: BaseColor/Metallic/Specular/Roughness 参数节点   │
│   ├── 保存为: /Game/UEMotion/Materials/M_BlackFloorParent    │
│   └── 作用: 定义材质的 PBR 属性模板                          │
│         ↓ 继承                                              │
│   M_BlackFloor (MaterialInstanceConstant)                   │
│   ├── 类型: 材质实例                                        │
│   ├── Parent: M_BlackFloorParent                           │
│   ├── 覆盖: BaseColor = (0,0,0,1) 纯黑                     │
│   ├── 保存为: /Game/UEMotion/Materials/M_BlackFloor          │
│   └── 作用: 具体的颜色/参数实例                             │
│         ↓ 赋值                                              │
│   BP_BlackFloor (UBlueprint)                                │
│   ├── SCS MeshComponent.Material[0] = M_BlackFloor          │
│   └── 保存为: /Game/UEMotion/Blueprints/BP_BlackFloor       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 调用链

```
Python: Scene("demo")
    ↓
C++: UUEMotionScene::Initialize()
    ↓
C++: UUEMotionScene::SetupBlackBackgroundFloor()     // 第 408 行
    ├─→ CreateOrLoadBlackMaterial()                  // 第 546 行
    │   ├─→ CreateOrLoadBlackParentMaterial()        // 第 648 行 [父材质]
    │   └─→ 创建 MaterialInstanceConstant             // [实例材质]
    ├─→ 创建 BP_BlackFloor 蓝图                       // [蓝图 + SCS 赋值]
    └─→ SpawnActor<BP_BlackFloor>()                   // [生成 Actor]
```

---

## 3. Step 1: 创建父材质 (UMaterial)

### 目标
创建一个 **UMaterial** 类型的 `.uasset` 文件，作为材质的基础模板。

### 为什么不用引擎内置的 M_Unlit？

❌ **错误做法**：
```cpp
// 引擎内置材质有隐藏限制！
UMaterialInterface* BaseUnlitMat = LoadObject<UMaterialInterface>(
    nullptr, TEXT("/Engine/EngineMaterials/M_Unlit.M_Unlit"));
// 问题：M_Unlit 不暴露 BaseColor 参数 → 设置无效
```

✅ **正确做法**：自己创建 `UMaterial`，完全控制所有参数。

### 完整代码

```cpp
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"

UMaterialInterface* CreateParentMaterial(const FString& MaterialPath)
{
    // ====== Step 1.1: 检查是否已存在 ======
    if (UEditorAssetLibrary::DoesAssetExist(MaterialPath))
    {
        UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
        if (Existing) return Existing;  // 复用已有 UAsset
        
        // 损坏处理
        UEditorAssetLibrary::DeleteAsset(MaterialPath);
    }

    // ====== Step 1.2: 创建 Package 和 UMaterial 对象 ======
    UPackage* Package = CreatePackage(*MaterialPath);
    
    UMaterial* Material = NewObject<UMaterial>(
        Package,
        TEXT("MyMaterial"),                    // 对象名称
        RF_Public | RF_Standalone | RF_Transactional  // 关键标志！

    if (!Material) return nullptr;

    // ====== Step 1.3: 配置材质属性 ======
    Material->MaterialDomain = EMaterialDomain::MD_Surface;  // 表面材质
    Material->BlendMode = EBlendMode::BLEND_Opaque;          // 不透明
    Material->TwoSided = true;                              // 双面渲染

    // ====== Step 1.4: 添加参数节点（UE 5.7 正确方式）=====
    
    // Vector Parameter (颜色类: BaseColor, EmissiveColor)
    UMaterialExpressionVectorParameter* BaseColorParam = 
        NewObject<UMaterialExpressionVectorParameter>(Material);
    BaseColorParam->ParameterName = FName("BaseColor");
    BaseColorParam->DefaultValue = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    Material->GetExpressionCollection().AddExpression(BaseColorParam);  // ★ UE 5.7 API

    // Scalar Parameter (数值类: Metallic, Specular, Roughness)
    UMaterialExpressionScalarParameter* SpecularParam = 
        NewObject<UMaterialExpressionScalarParameter>(Material);
    SpecularParam->ParameterName = FName("Specular");
    SpecularParam->DefaultValue = 0.0f;  // ★ 关键：消除默认 4% 反射
    Material->GetExpressionCollection().AddExpression(SpecularParam);

    UMaterialExpressionScalarParameter* RoughnessParam = 
        NewObject<UMaterialExpressionScalarParameter>(Material);
    RoughnessParam->ParameterName = FName("Roughness");
    RoughnessParam->DefaultValue = 1.0f;  // 完全哑光
    Material->GetExpressionCollection().AddExpression(RoughnessParam);

    UMaterialExpressionScalarParameter* MetallicParam = 
        NewObject<UMaterialExpressionScalarParameter>(Material);
    MetallicParam->ParameterName = FName("Metallic");
    MetallicParam->DefaultValue = 0.0f;
    Material->GetExpressionCollection().AddExpression(MetallicParam);

    // ====== Step 1.5: 连接到输出引脚（EditorOnlyData）=====
#if WITH_EDITORONLY_DATA
    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    if (EditorData)
    {
        EditorData->BaseColor.Expression = BaseColorParam;
        EditorData->BaseColor.OutputIndex = 0;
        
        EditorData->Metallic.Expression = MetallicParam;
        EditorData->Metallic.OutputIndex = 0;
        
        EditorData->Specular.Expression = SpecularParam;
        EditorData->Specular.OutputIndex = 0;
        
        EditorData->Roughness.Expression = RoughnessParam;
        EditorData->Roughness.OutputIndex = 0;
    }
#endif

    // ====== Step 1.6: 触发序列化 ======
    Material->SetFlags(RF_Public | RF_Standalone);
    Package->MarkPackageDirty();

#if WITH_EDITOR
    Material->PostEditChange();  // ★ 必须调用！否则参数不写入磁盘
#endif

    // ====== Step 1.7: 保存到磁盘 ======
    FString FilePath = GetUAssetFilePath(MaterialPath);  // 如: .../M_MyMaterial.uasset
    
    FSavePackageArgs SaveArgs;
    SaveArgs.SaveFlags = RF_Public | RF_Standalone;

    bool bSuccess = UPackage::SavePackage(
        Package,          // Package 对象
        Material,         // ★ 要保存的对象（不是 nullptr！）
        *FilePath,        // 完整文件路径
        SaveArgs
    );

    if (bSuccess)
    {
        FAssetRegistryModule::AssetCreated(Material);  // 注册到资产系统
        
#if WITH_EDITOR
        // 强制同步资产注册表
        FAssetRegistryModule& AssetRegistry = 
            FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FString> Paths;
        Paths.Add(MaterialPath);
        AssetRegistry.Get().ScanPathsSynchronous(Paths);
#endif
        
        return Material;
    }
    
    return nullptr;
}
```

### 关键要点

| 步骤 | API | 注意事项 |
|------|-----|----------|
| 创建对象 | `NewObject<UMaterial>(Package, Name, Flags)` | 必须包含 `RF_Public \| RF_Standalone` |
| 添加节点 | `GetExpressionCollection().AddExpression()` | **UE 5.7 正确方式**，不要用 `Expressions.Add()` |
| 连接输出 | `EditorData->BaseColor.Expression = Param` | 在 `#if WITH_EDITORONLY_DATA` 块内 |
| 触发保存 | `PostEditChange()` | **必须调用**，否则参数不序列化 |
| 写入磁盘 | `SavePackage(Pkg, Obj, Path, Args)` | **第二个参数传对象指针**，不是 nullptr |

---

## 4. Step 2: 创建实例材质 (MaterialInstanceConstant)

### 目标
基于父材质创建 **MaterialInstanceConstant**，覆盖具体颜色/参数。

### 代码

```cpp
UMaterialInterface* CreateInstanceMaterial(
    const FString& InstancePath, 
    UMaterialInterface* ParentMaterial)
{
    // 检查是否已存在
    if (UEditorAssetLibrary::DoesAssetExist(InstancePath))
    {
        UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, *InstancePath);
        if (Existing) return Existing;
        UEditorAssetLibrary::DeleteAsset(InstancePath);
    }

    // 创建 MIC
    UPackage* InstancePackage = CreatePackage(*InstancePath);
    
    UMaterialInstanceConstant* MIC = NewObject<UMaterialInstanceConstant>(
        InstancePackage,
        TEXT("MyMaterialInstance"),
        RF_Public | RF_Standalone | RF_Transactional
    );

    // 设置父材质
    MIC->SetParentEditorOnly(ParentMaterial);

    // 覆盖参数
#if WITH_EDITOR
    MIC->SetVectorParameterValueEditorOnly(FName("BaseColor"), FLinearColor(0, 0, 0, 1));
    MIC->PostEditChange();  // ★ 必须调用
#endif

    // 保存
    MIC->SetFlags(RF_Public | RF_Standalone);
    InstancePackage->MarkPackageDirty();

    FString FilePath = GetUAssetFilePath(InstancePath);
    FSavePackageArgs SaveArgs;
    SaveArgs.SaveFlags = RF_Public | RF_Standalone;

    bool bSuccess = UPackage::SavePackage(InstancePackage, MIC, *FilePath, SaveArgs);
    
    if (bSuccess)
    {
        FAssetRegistryModule::AssetCreated(MIC);
        return MIC;
    }
    return nullptr;
}
```

### 父材质 vs 实例材质对比

| 特性 | UMaterial (父) | MaterialInstanceConstant (实例) |
|------|----------------|-------------------------------|
| **用途** | 基础模板 | 具体变体 |
| **可否有子** | ✅ 可以被继承 | ❌ 叶子节点 |
| **参数定义** | ✅ 添加 Parameter 节点 | ❌ 只能覆盖值 |
| **修改方式** | 改变材质结构 | 改变参数值 |
| **典型数量** | 1 个（每种类型） | N 个（多种颜色/变体） |

---

## 5. Step 3: 创建蓝图并赋值材质到 SCS

### 目标
创建一个 **Blueprint (.uasset)**，在其 **SCS (SimpleConstructionScript)** 中配置 MeshComponent 的材质槽位。

### 核心概念：SCS vs Runtime

```
❌ 错误: 运行时才设置材质
   Actor->Spawn()
   Actor->Mesh->SetMaterial(0, Mat)  // 每次运行都执行，不保存到蓝图

✅ 正确: 在蓝图的 SCS 模板中设置材质
   Blueprint->SCS->MeshTemplate->SetMaterial(0, Mat)  // 写入蓝图，持久化
```

### 完整代码

```cpp
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/KismetEditorUtilities.h"

UClass* CreateBlueprintWithMaterial(
    const FString& BlueprintPath,
    const FString& BlueprintName,
    UMaterialInterface* Material)
{
    // ====== Step 3.1: 检查/加载现有蓝图 ======
    UClass* BPClass = nullptr;
    
    if (UEditorAssetLibrary::DoesAssetExist(BlueprintPath))
    {
        UBlueprint* ExistingBP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
        if (ExistingBP && ExistingBP->GeneratedClass)
        {
            BPClass = ExistingBP->GeneratedClass;
            return BPClass;  // 复用
        }
    }

    // ====== Step 3.2: 创建新蓝图 ======
    UPackage* Package = CreatePackage(*BlueprintPath);
    
    UBlueprintFactory* BPFactory = NewObject<UBlueprintFactory>();
    BPFactory->ParentClass = AActor::StaticClass();

    UBlueprint* NewBP = Cast<UBlueprint>(BPFactory->FactoryCreateNew(
        UBlueprint::StaticClass(),
        Package,
        *BlueprintName,           // 如 "BP_BlackFloor"
        RF_Public | RF_Standalone,
        nullptr,
        GWarn
    ));

    if (!NewBP) return nullptr;

    // ====== Step 3.3: 配置 SCS（关键！）=====
    USimpleConstructionScript* SCS = NewBP->SimpleConstructionScript;
    if (SCS)
    {
        // 3.3.1: 创建根组件
        USCS_Node* RootNode = SCS->CreateNode(
            USceneComponent::StaticClass(), 
            TEXT("DefaultSceneRoot")
        );
        SCS->AddNode(RootNode);

        // 3.3.2: 创建 Mesh 组件节点
        USCS_Node* MeshNode = SCS->CreateNode(
            UStaticMeshComponent::StaticClass(), 
            TEXT("Mesh")  // 如 "FloorMesh"
        );
        RootNode->AddChildNode(MeshNode);

        // 3.3.3: 获取 MeshTemplate 并配置
        UStaticMeshComponent* MeshTemplate = 
            Cast<UStaticMeshComponent>(MeshNode->ComponentTemplate);
        
        if (MeshTemplate)
        {
            // 加载 StaticMesh
            UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(
                nullptr, 
                TEXT("/Engine/BasicShapes/Plane.Plane")
            );
            MeshTemplate->SetStaticMesh(PlaneMesh);

            // 配置变换
            MeshTemplate->SetRelativeScale3D(FVector(10.0f, 10.0f, 1.0f));
            MeshTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            MeshTemplate->CastShadow = false;

            // ★★★ 关键：将材质赋给 SCS Template（不是运行时 Component）
            if (Material)
            {
                MeshTemplate->SetMaterial(0, Material);
                // 这行代码让材质出现在蓝图的 Details 面板中！
            }
        }
    }

    // ====== Step 3.4: 编译蓝图 ======
    FKismetEditorUtilities::CompileBlueprint(NewBP);

    // ====== Step 3.5: 保存蓝图 ======
    NewBP->SetFlags(RF_Public | RF_Standalone);
    Package->MarkPackageDirty();

#if WITH_EDITOR
    NewBP->PostEditChange();
#endif

    FString BPFilename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), 
        FPackageName::GetAssetPackageExtension()
    );

    FSavePackageArgs SaveArgs;
    SaveArgs.SaveFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewBP, *BPFilename, SaveArgs);

    FAssetRegistryModule::AssetCreated(NewBP);

    return NewBP->GeneratedClass;
}
```

### SCS 赋值的关键区别

```cpp
// ❌ 方式 A: 运行时赋值（不保存到蓝图）
AActor* Actor = World->SpawnActor<AActor>(BPClass, ...);
UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(
    Actor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
Mesh->SetMaterial(0, Material);  // 临时生效，不序列化

// ✅ 方式 B: SCS 模板赋值（永久保存到蓝图）
USCS_Node* MeshNode = SCS->CreateNode(UStaticMeshComponent::StaticClass(), ...);
UStaticMeshComponent* MeshTemplate = Cast<UStaticMeshComponent>(MeshNode->ComponentTemplate);
MeshTemplate->SetMaterial(0, Material);  // 写入 .uasset，持久化！
```

---

## 6. Step 4: Spawn Actor 并应用材质

### 代码

```cpp
void SetupBlackBackgroundFloor()
{
    // ====== Step 4.1: 获取或创建蓝图 Class ======
    FString BPPath = TEXT("/Game/UEMotion/Blueprints/BP_BlackFloor");
    UClass* FloorClass = LoadOrCreateBlueprint(BPPath);  // 调用 Step 3
    
    if (!FloorClass) return;

    // ====== Step 4.2: Spawn Actor ======
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = 
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bNoFail = true;

    AActor* FloorActor = SceneWorld->SpawnActor<AActor>(
        FloorClass,
        FVector(0, 0, -0.01),      // 位置：略低于原点
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (!FloorActor) return;

    // ====== Step 4.3: 运行时二次确认材质（可选但推荐）=====
    UStaticMeshComponent* FloorMesh = Cast<UStaticMeshComponent>(
        FloorActor->GetComponentByClass(UStaticMeshComponent::StaticClass())
    );

    if (FloorMesh)
    {
        UMaterialInterface* BlackMaterial = CreateOrLoadBlackMaterial();
        if (BlackMaterial)
        {
            FloorMesh->SetMaterial(0, BlackMaterial);
        }
    }
}
```

---

## 7. UE 5.7 关键 API 差异

### 废弃的 API（UE 4.x → UE 5.x）

| 旧 API (UE 4) | 新 API (UE 5.7) | 说明 |
|----------------|-----------------|------|
| `Material->Expressions.Add()` | `Material->GetExpressionCollection().AddExpression()` | 表达式集合管理 |
| `Material->BaseColor.Expression` | `Material->GetEditorOnlyData()->BaseColor.Expression` | 输出引脚访问 |
| `CreatePackage(Outer, Name)` | `CreatePackage(Name)` | Outer 参数废弃 |
| `ShadingModel` (public) | 通过 `EditorOnlyData` 访问 | 变成 private |

### 必须使用的头文件

```cpp
#include "Materials/Material.h"                        // UMaterial
#include "Materials/MaterialExpressionVectorParameter.h"  // 向量参数
#include "Materials/MaterialExpressionScalarParameter.h"  // 标量参数
#include "Materials/MaterialExpressionConstant.h"         // 常量
// 注意：不需要 #include "Materials/MaterialEditorOnlyData.h" （不存在）
// 使用 #if WITH_EDITORONLY_DATA 包裹 EditorOnlyData 访问
```

---

## 8. 常见陷阱与解决方案

### 陷阱 1: 材质不够黑

**现象**: BaseColor 设为 (0,0,0) 但看起来是深灰

**原因**: PBR 系统 Specular 默认值 = 0.5（约 4% 反射率）

**解决**: 显式设置 Specular = 0
```cpp
UMaterialExpressionScalarParameter* SpecularParam = ...;
SpecularParam->DefaultValue = 0.0f;  // ★ 关键！
EditorData->Specular.Expression = SpecularParam;
```

### 陷阱 2: PostEditChange 未调用

**现象**: 代码执行无误，但参数未写入 .uasset

**原因**: 缺少 `PostEditChange()` 调用

**解决**:
```cpp
#if WITH_EDITOR
    Material->PostEditChange();  // ★ 必须在设置参数后调用
#endif
```

### 陷阱 3: SavePackage 传 nullptr

**现象**: 保存"成功"但资产损坏或无法加载

**原因**: 第二个参数传了 nullptr

**解决**:
```cpp
// ❌ 错误
UPackage::SavePackage(Package, nullptr, *Path, Args);

// ✅ 正确：传入要保存的对象
UPackage::SavePackage(Package, Material, *Path, Args);
UPackage::SavePackage(Package, MIC, *Path, Args);
UPackage::SavePackage(Package, NewBP, *Path, Args);
```

### 陷阱 4: 使用引擎内置材质的限制

**现象**: 设置参数后无效果

**原因**: M_Unlit 等引擎内置材质不暴露某些参数

**解决**: 自己创建 UMaterial 作为父材质

### 陷阱 5: 字符串匹配错误

**现象**: Y 轴被当成 X 轴处理

**原因**: 使用 `Contains()` 做模糊匹配

**解决**:
```cpp
// ❌ 错误
if (Name.Contains("X")) { ... }  // "Axis_Y" 也包含 "X"!

// ✅ 正确
if (Name.EndsWith(TEXT("X"))) { ... }  // 精确匹配后缀
```

### 陷阱 6: UMaterialExpressionConstant 用错

**现象**: 编译错误 `error C2039: "G": 不是成员`

**原因**: `UMaterialExpressionConstant` 是标量，只有 `R`

**解决**:
```cpp
// ❌ 错误：用于颜色
UMaterialExpressionConstant* Const = ...;
Const->R = 0; Const->G = 0; Const->B = 0;  // G/B/A 不存在

// ✅ 正确：标量用 Constant，颜色用 VectorParameter
UMaterialExpressionScalarParameter* Scalar = ...;  // 标量
Scalar->DefaultValue = 0.0f;

UMaterialExpressionVectorParameter* Vector = ...;  // 颜色
Vector->DefaultValue = FLinearColor(0, 0, 0, 1);
```

---

## 9. 完整代码模板

### 一键复制：Vantablack 地板材质创建

```cpp
/// @brief 创建吸收所有光线的纯黑地板（Vantablack 效果）
/// @return 成功返回材质指针，失败返回 nullptr
UMaterialInterface* CreateVantablackFloorMaterial()
{
    const FString ParentPath = TEXT("/Game/UEMotion/Materials/M_VantablackParent");
    const FString InstancePath = TEXT("/Game/UEMotion/Materials/M_Vantablack");

    // --- 创建父材质 ---
    UMaterial* ParentMat = nullptr;
    
    if (!UEditorAssetLibrary::DoesAssetExist(ParentPath))
    {
        UPackage* Pkg = CreatePackage(*ParentPath);
        ParentMat = NewObject<UMaterial>(Pkg, TEXT("M_VantablackParent"), 
            RF_Public | RF_Standalone | RF_Transactional);
        
        ParentMat->MaterialDomain = EMaterialDomain::MD_Surface;
        ParentMat->BlendMode = EBlendMode::BLEND_Opaque;
        ParentMat->TwoSided = true;

        // PBR 参数：全零反射
        auto AddVec = [&](FName Name, FLinearColor Val) -> UMaterialExpressionVectorParameter* {
            auto* P = NewObject<UMaterialExpressionVectorParameter>(ParentMat);
            P->ParameterName = Name; P->DefaultValue = Val;
            ParentMat->GetExpressionCollection().AddExpression(P); return P;
        };
        auto AddScalar = [&](FName Name, float Val) -> UMaterialExpressionScalarParameter* {
            auto* P = NewObject<UMaterialExpressionScalarParameter>(ParentMat);
            P->ParameterName = Name; P->DefaultValue = Val;
            ParentMat->GetExpressionCollection().AddExpression(P); return P;
        };

        auto* BC = AddVec("BaseColor", FLinearColor(0,0,0,1));
        auto* Met = AddScalar("Metallic", 0.0f);
        auto* Spec = AddScalar("Specular", 0.0f);      // ★ 零反射
        auto* Rough = AddScalar("Roughness", 1.0f);    // ★ 完全哑光
        auto* Emi = AddVec("EmissiveColor", FLinearColor(0,0,0,1));

#if WITH_EDITORONLY_DATA
        auto* ED = ParentMat->GetEditorOnlyData();
        if (ED) {
            ED->BaseColor.Expression = BC; ED->BaseColor.OutputIndex = 0;
            ED->Metallic.Expression = Met; ED->Metallic.OutputIndex = 0;
            ED->Specular.Expression = Spec; ED->Specular.OutputIndex = 0;
            ED->Roughness.Expression = Rough; ED->Roughness.OutputIndex = 0;
            ED->EmissiveColor.Expression = Emi; ED->EmissiveColor.OutputIndex = 0;
        }
#endif
        ParentMat->PostEditChange();

        FString FP = FPaths::Combine(FPaths::ProjectContentDir(), 
            TEXT("UEMotion/Materials/M_VantablackParent.uasset"));
        FSavePackageArgs SA; SA.SaveFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Pkg, ParentMat, *FP, SA);
        FAssetRegistryModule::AssetCreated(ParentMat);
    }
    else
    {
        ParentMat = Cast<UMaterial>(LoadObject<UMaterial>(nullptr, *ParentPath));
    }

    if (!ParentMat) return nullptr;

    // --- 创建实例材质 ---
    UMaterialInstanceConstant* MIC = nullptr;
    
    if (!UEditorAssetLibrary::DoesAssetExist(InstancePath))
    {
        UPackage* IPkg = CreatePackage(*InstancePath);
        MIC = NewObject<UMaterialInstanceConstant>(IPkg, TEXT("M_Vantablack"),
            RF_Public | RF_Standalone | RF_Transactional);
        MIC->SetParentEditorOnly(ParentMat);
#if WITH_EDITOR
        MIC->SetVectorParameterValueEditorOnly("BaseColor", FLinearColor(0,0,0,1));
        MIC->PostEditChange();
#endif
        MIC->SetFlags(RF_Public | RF_Standalone);
        FString IFP = FPaths::Combine(FPaths::ProjectContentDir(),
            TEXT("UEMotion/Materials/M_Vantablack.uasset"));
        FSavePackageArgs ISA; ISA.SaveFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(IPkg, MIC, *IFP, ISA);
        FAssetRegistryModule::AssetCreated(MIC);
    }
    else
    {
        MIC = LoadObject<UMaterialInstanceConstant>(nullptr, *InstancePath);
    }

    return MIC;
}
```

---

## 附录：文件路径规范

```
PluginTest/
├── Content/
│   └── UEMotion/
│       ├── Materials/           ← 材质 UAssets
│       │   ├── M_VantablackParent.uasset    (父材质)
│       │   └── M_Vantablack.uasset          (实例材质)
│       └── Blueprints/          ← 蓝图 UAssets
│           └── BP_BlackFloor.uasset         (带材质的蓝图)
└── Plugins/
    └── UEMotionPlugin/
        └── Source/
            └── UEMotionPlugin/
                └── Private/
                    └── Core/
                        └── UEMotionSceneSetup.cpp  ← 实现代码
```

---

*文档版本: 1.0*
*最后更新: 2026-05-13*
*基于项目: PluginTest (UE 5.7)*
