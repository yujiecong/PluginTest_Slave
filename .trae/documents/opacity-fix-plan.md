# Opacity Bug 修复计划：从 MPC 全局方案改为 Per-Object MID 方案

## 问题根因分析

### 当前架构（有根本性缺陷）

```
M_UEMotionBaseTranslucent 材质定义 (AssetFactory.cpp:91-95):
  Opacity 引脚 ← UMaterialExpressionCollectionParameter (MPC_UEMotionFade.Opacity)
                ↑ 这是全局参数！所有共享此材质的对象都受影响！

数据流:
  SetOpacity(value) → MPC.Opacity = value  →  所有对象同时变透明/不透明 ❌
  Reset()          → SetOpacity(0)        →  所有对象瞬间全透明! ❌
  Sequencer Key    → MPC Track 0→1         →  驱动全局参数（但 Reset 已把值归零）
```

**致命缺陷**：Material 的 Opacity 引脚连接的是 **CollectionParameter（MPC）**，不是 **ScalarParameter**。

这意味着：

* `MID->SetScalarParameterValue("Opacity", x)` → **完全无效**！（材质没有 ScalarParameter 名为 Opacity）

* 只有 `SetScalarParameterValue(World, MPC, "Opacity", x)` 能改变透明度

* 但 MPC 是**全局的**——一个值控制所有对象

* `Reset()` 调用 `SetOpacity(0)` → 瞬间让**所有对象**透明

### 为什么手动调整"看起来正常"

手动调 Opacity → PostEditChangeProperty → SetOpacity → 设 MPC=新值。此时场景中通常只有一个 Cube，所以"看起来"是那个 Cube 变了。实际上改的是全局参数。

***

## 新方案：Per-Object MID（每对象独立动态材质）

### 核心改动

将材质的 Opacity 数据源从 **CollectionParameter（全局 MPC）** 改为 **ScalarParameter（每对象独立）**：

```
新架构:
  M_UEMotionBaseTranslucent 材质:
    Opacity 引脚 ← UMaterialExpressionScalarParameter("Opacity")  ← 每个MID独立控制 ✅

数据流:
  Object A 的 MID.Opacity = 0.5   →  只有 A 半透明 ✅
  Object B 的 MID.Opacity = 1.0   →  B 完全不透明 ✅
  Reset() → Object A MID.Opacity=0 → 只有 A 透明，B 不受影响 ✅
```

***

## 实施步骤

### Step 1: 修改材质定义 — CollectionParameter → ScalarParameter

**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionAssetFactory.cpp`

**位置**: 第 91-95 行，`EnsureBaseTranslucentMaterial()` 函数

**修改前**:

```cpp
UMaterialExpressionCollectionParameter* MPCNode = NewObject<UMaterialExpressionCollectionParameter>(Material);
MPCNode->Collection = MPC;
MPCNode->ParameterName = FName("Opacity");
MPCNode->ParameterId = OpacityParamId;
Material->GetExpressionCollection().AddExpression(MPCNode);
```

**修改后**:

```cpp
UMaterialExpressionScalarParameter* OpacityScalar = NewObject<UMaterialExpressionScalarParameter>(Material);
OpacityScalar->ParameterName = FName("Opacity");
OpacityScalar->DefaultValue = 1.0f;
Material->GetExpressionCollection().AddExpression(OpacityScalar);
```

**同时**: 第 110 行 `EditorData->Opacity.Expression` 的赋值目标从 `MPCNode` 改为 `OpacityScalar`

**注意**: 此修改需要**删除已生成的材质资产**（M\_UEMotionBaseTranslucent.uasset），让它重新生成。或者在编辑器中手动重建材质。

### Step 2: 简化 SetOpacity() — 只操作 MID，去掉 MPC

**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionMobjectActor.cpp`

**位置**: 第 42-61 行，`SetOpacity()` 函数

**修改后**:

```cpp
void AUEMotionMobjectActor::SetOpacity(float InOpacity)
{
    Opacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);

    EnsureDynamicMaterial();
    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
    }
}
```

**删除**:

* MPC 相关的 `#include "Materials/MaterialParameterCollection"`

* `#include "Kismet/KismetMaterialLibrary"`

* `static const FString FadeMPCPath` 常量

* `SetOpacity()` 中整个 `UWorld* World / LoadObject<MPC> / UKismetMaterialLibrary` 块

### Step 3: 修改 Sequencer Play() — 从 MPC Track 改为 Per-Object Material Track

**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp`

**位置**: 第 884-950 行，FadeAnimation 的 Play() 分支

**当前逻辑**: 创建 `UMovieSceneMaterialParameterCollectionTrack`，key 全局 MPC 参数（约 60 行代码）

**改为**: 使用 `UMovieSceneMaterialTrack` + `UMovieSceneComponentMaterialParameterSection`（UE 内置 per-object 材质轨道）

**已确认 API**:

* `UMovieSceneMaterialTrack::AddScalarParameterKey(FName, FFrameNumber, float)` ✅

* `UUEMotionMobject::GetInternalActor()` → 获取目标 Actor ✅

* `AActor->FindComponentByClass<UStaticMeshComponent>()` → 获取 MeshComp ✅

**实现代码**:

```cpp
else if (UUEMotionFadeAnimation* FadeAnim = Cast<UUEMotionFadeAnimation>(Animation))
{
    UUEMotionMobject* Target = FadeAnim->GetTargetMobject();
    if (Target)
    {
        AActor* TargetActor = Target->GetInternalActor();
        if (TargetActor && MovieScene)
        {
            // 使用 per-object Material Track 替代全局 MPC Track
            UMovieSceneMaterialTrack* MatTrack = MovieScene->AddTrack<UMovieSceneMaterialTrack>();
            if (MatTrack)
            {
                MatTrack->SetObjectName(TargetActor->GetName());
                MatTrack->SetObjectBindingID(UEMotionCompat::GetObjectBinding(TargetActor));

                UMovieSceneComponentMaterialParameterSection* Section =
                    Cast<UMovieSceneComponentMaterialParameterSection>(MatTrack->CreateNewSection());
                if (Section)
                {
                    Section->SetRange(TRange<FFrameNumber>::All());
                    MatTrack->AddSection(*Section);

                    float StartOpacity = FadeAnim->IsFadeOut() ? 1.0f : 0.0f;
                    float EndOpacity = FadeAnim->IsFadeOut() ? 0.0f : 1.0f;

                    FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, StartFrame);
                    FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, EndFrame);

                    FMaterialParameterInfo OpacityInfo;
                    OpacityInfo.Name = FName("Opacity");
                    OpacityInfo.Index = INDEX_NONE;

                    Section->AddScalarParameterKey(OpacityInfo, StartTick, StartOpacity, FString(), FString());
                    Section->AddScalarParameterKey(OpacityInfo, EndTick, EndOpacity, FString(), FString());

                    UE_LOG(LogTemp, Log,
                        TEXT("UEMotionScene: Recorded per-object material fade keys - %s: %.2f→%.2f"),
                        *TargetActor->GetName(), StartOpacity, EndOpacity);
                }
            }
        }
    }
}
```

**需要新增 include**:

```cpp
#include "Tracks/MovieSceneMaterialTrack.h"
#include "Sections/MovieSceneComponentMaterialParameterSection.h"
```

### Step 4: 更新 Python 层 mobject.py（如有必要）

**文件**: `Scripts/uemotion/mobject.py`

检查 `fade_in()` / `fade_out()` 是否需要调整。当前代码调用 `set_visibility(False)` 作为初始状态——这可能需要保留或移除，取决于 Step 3 的实现方式。

### Step 5: 清理 MPC 相关残留代码

以下位置可能还有 MPC 引用需要清理：

* `UEMotionAssetFactory.cpp`: `EnsureFadeMPC()` 函数（可保留用于其他用途，但不再用于 Opacity）

* `UEMotionMobject.cpp`: 检查是否有 MPC 相关调用

* 测试脚本中的 MPC 检查代码

***

## 方案对比

| <br />           | 当前方案 (MPC 全局)            | 新方案 (Per-Object MID)        |
| ---------------- | ------------------------ | --------------------------- |
| **Opacity 数据源**  | CollectionParameter (全局) | ScalarParameter (每对象)       |
| **对象独立性**        | ❌ 改一个影响全部                | ✅ 各自独立                      |
| **Reset() 副作用**  | ❌ 让所有对象变透明               | ✅ 只影响当前对象                   |
| **Sequencer 集成** | ⚠️ MPC Track（全局）         | ✅ Per-object Material Track |
| **复杂度**          | 高（隐式全局状态）                | 低（标准 UE 做法）                 |
| **符合 UE 最佳实践**   | ❌                        | ✅                           |

***

## 风险与注意事项

1. **材质重建**: 修改 `EnsureBaseTranslucentMaterial()` 后，需要删除旧的 `M_UEMotionBaseTranslucent` 资产让它重新生成，或在编辑器中重新运行资产创建流程
2. **Sequencer Material Track API**: `UMovieSceneMaterialTrack` 在 UE5.7 中的具体用法可能需要进一步验证
3. **向后兼容**: 如果已有保存的 LevelSequence 使用了旧 MPC Track，需要迁移
4. **MPC 可保留**: `MPC_UEMotionFade` 资产本身可以保留（不删除），只是不再用于 Opacity 控制，未来可用于其他全局参数（如全局雾效等）

