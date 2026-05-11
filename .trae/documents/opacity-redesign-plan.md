# Opacity 参数重新设计方案：从 MPC 迁移到 Per-Instance MID 控制

## 📋 问题分析

### 当前实现缺陷

当前 opacity 控制使用 **Material Parameter Collection (MPC)** 实现，存在以下严重问题：

1. **全局共享问题**: MPC 是全局参数集合，所有 actor 共享同一个 opacity 值

   * 修改一个 actor 的 opacity → 所有使用该 MPC 的 actor 都会生效

   * 无法实现独立的透明度控制

2. **代码位置**:

   * [UEMotionMobject.cpp:44-61](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionMobject.cpp#L44-L61) - 基础材质使用 MPC 节点

   * [UEMotionMobject.cpp:229-238](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionMobject.cpp#L229-L238) - SetOpacity 同时设置 MPC 全局值

   * [UEMotionScene.cpp:886-952](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp#L886-L952) - Fade 动画录制到 MPC 轨道

   * [UEMotionAssetFactory.cpp:37-91](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionAssetFactory.cpp#L37-L91) - MPC 创建和材质初始化

### 根本原因

虽然代码已经为每个 actor 创建了 `UMaterialInstanceDynamic` (MID)，但基础材质的 **Opacity 输入连接的是 MPC 节点**，导致所有 MID 从同一个全局源读取 opacity 值。

***

## 🎯 解决方案：Per-Instance Material Instance Dynamic (MID)

### 技术原理

使用 **Scalar Parameter** 替代 **Collection Parameter (MPC)**，让每个 actor 的 MID 独立控制自己的 opacity 参数。

**核心优势**:

* ✅ 每个 actor 拥有独立的材质实例

* ✅ 运行时可独立修改每个 actor 的透明度

* ✅ 支持不同 actor 同时显示不同的透明度

* ✅ 性能优秀（无需重新编译 shader）

* ✅ UE5 标准 API，稳定可靠

**技术对比**:

| 特性   | MPC (当前)   | MID + Scalar Parameter (新方案) |
| ---- | ---------- | ---------------------------- |
| 作用域  | 全局共享       | Per-Instance 独立              |
| 参数修改 | 影响所有 actor | 仅影响目标 actor                  |
| 内存占用 | 低（单一实例）    | 稍高（每 actor 一个实例）             |
| 性能影响 | 极低         | 低（GPU 友好）                    |
| 动画支持 | MPC Track  | Property Track / 自定义         |

***

## 🔧 实施计划

### Phase 1: 修改基础材质创建逻辑（核心改动）

#### 文件: `UEMotionMobject.cpp`

**1.1 修改** **`GetOrCreateBaseMaterial()`** **方法**

**当前代码 (L44-61)**:

```cpp
static const FString MPCPath = TEXT("/Game/UEMotion/Materials/MPC_UEMotionFade");
UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, *MPCPath);

UMaterialExpression* OpacityOutput = nullptr;

if (MPC)
{
    FGuid OpacityParamId = MPC->GetParameterId(FName("Opacity"));
    if (OpacityParamId.IsValid())
    {
        UMaterialExpressionCollectionParameter* MPCNode = NewObject<UMaterialExpressionCollectionParameter>(Material);
        MPCNode->Collection = MPC;
        MPCNode->ParameterName = FName("Opacity");
        MPCNode->ParameterId = OpacityParamId;
        Material->GetExpressionCollection().AddExpression(MPCNode);
        OpacityOutput = MPCNode;
    }
}
```

**新代码**:

```cpp
// 使用 Scalar Parameter 替代 MPC，实现 per-instance 控制
UMaterialExpressionScalarParameter* OpacityParam = NewObject<UMaterialExpressionScalarParameter>(Material);
OpacityParam->ParameterName = FName("Opacity");
OpacityParam->DefaultValue = 1.0f;  // 默认完全不透明
Material->GetExpressionCollection().AddExpression(OpacityParam);
UMaterialExpression* OpacityOutput = OpacityParam;
```

**需要的额外头文件**:

```cpp
#include "Materials/MaterialExpressionScalarParameter.h"
```

**1.2 修改** **`SetOpacity()`** **方法**

**当前代码 (L225-243)**:

```cpp
void UUEMotionMobject::SetOpacity(float InOpacity)
{
    CurrentOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);

    static const FString MPCPath = TEXT("/Game/UEMotion/Materials/MPC_UEMotionFade");
    UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, *MPCPath);
    if (MPC && InternalActor.IsValid())
    {
        UWorld* World = InternalActor->GetWorld();
        if (World)
        {
            UKismetMaterialLibrary::SetScalarParameterValue(World, MPC, FName("Opacity"), CurrentOpacity);
        }
    }

    if (MaterialInstance.IsValid())
    {
        MaterialInstance->SetScalarParameterValue(FName("Opacity"), CurrentOpacity);
    }
    // ... visibility logic
}
```

**新代码**:

```cpp
void UUEMotionMobject::SetOpacity(float InOpacity)
{
    CurrentOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);

    // 仅通过 MID 设置 per-instance opacity，不再使用 MPC
    if (MaterialInstance.IsValid())
    {
        MaterialInstance->SetScalarParameterValue(FName("Opacity"), CurrentOpacity);
    }

    // Visibility logic remains the same
    if (CurrentOpacity <= 0.0f)
    {
        bVisible = false;
        if (MeshComponent.IsValid())
        {
            MeshComponent->SetVisibility(false);
        }
    }
    else
    {
        bVisible = true;
        if (MeshComponent.IsValid())
        {
            MeshComponent->SetVisibility(true);
        }
    }
}
```

**移除的头文件** (如果不再需要):

```cpp
// #include "Materials/MaterialExpressionCollectionParameter.h"  // 不再需要
// #include "Materials/MaterialParameterCollection.h"  // 不再需要
```

***

### Phase 2: 修改材质工厂逻辑

#### 文件: `UEMotionAssetFactory.cpp`

**2.1 移除或简化** **`EnsureFadeMPC()`** **方法**

**选项 A - 完全移除** (推荐):

* 删除 `EnsureFadeMPC()` 方法 (L37-53)

* 删除所有对 MPC 的引用

* 如果项目中其他地方不再使用 MPC，可以删除 MPC 资产文件

**选项 B - 保留但标记废弃**:

* 保留方法但不调用

* 添加注释说明已弃用

**2.2 修改材质创建逻辑 (L66-91)**

**当前代码** (假设):

```cpp
UMaterialParameterCollection* MPC = EnsureFadeMPC();
// ... 使用 MPC 创建 CollectionParameter 节点
```

**新代码**:

```cpp
// 直接使用 Scalar Parameter，不依赖 MPC
UMaterialExpressionScalarParameter* OpacityParam = NewObject<UMaterialExpressionScalarParameter>(Material);
OpacityParam->ParameterName = FName("Opacity");
OpacityParam->DefaultValue = 1.0f;
Material->GetExpressionCollection().AddExpression(OpacityParam);
// 连接到 Opacity 输入
EditorData->Opacity.Expression = OpacityParam;
EditorData->Opacity.OutputIndex = 0;
```

***

### Phase 3: 重新设计 Fade 动画系统（关键挑战）

#### 文件: `UEMotionScene.cpp`

**问题**: 当前的 Fade 动画通过 `UMovieSceneMaterialParameterCollectionTrack` 录制到 MPC，需要改为 per-instance 方式。

**解决方案**: 为每个 mobject 的 fade 动画创建独立的材质参数轨道

**3.1 修改 Fade 动画录制逻辑 (L884-952)**

**当前实现**:

```cpp
else if (UUEMotionFadeAnimation* FadeAnim = Cast<UUEMotionFadeAnimation>(Animation))
{
    // 加载 MPC
    static const FString MPCPath = TEXT("/Game/UEMotion/Materials/MPC_UEMotionFade");
    UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, *MPCPath);
    
    // 创建/获取 MPC Track
    UMovieSceneMaterialParameterCollectionTrack* MPCTrack = ...;
    
    // 在 MPC Track 上录制 Opacity 关键帧
    Section->AddScalarParameterKey(FName("Opacity"), StartTick, StartOpacity);
    Section->AddScalarParameterKey(FName("Opacity"), EndTick, EndOpacity);
}
```

**新方案 A - 使用 Actor's Material Track** (推荐用于简单 fade):

```cpp
else if (UUEMotionFadeAnimation* FadeAnim = Cast<UUEMotionFadeAnimation>(Animation))
{
    // 获取目标 mobject
    UUEMotionMobject* TargetMobject = FadeAnim->GetTarget();  // 需要在 FadeAnimation 中添加 target 引用
    
    if (TargetMobject && TargetMobject->GetInternalActor().IsValid())
    {
        AActor* TargetActor = TargetMobject->GetInternalActor().Get();
        
        // 查找或创建该 Actor 的 Material Track
        UMovieScene* MovieScene = LevelSequence->GetMovieScene();
        FGuid ActorGuid = ...;  // 获取 actor 在 sequence 中的 GUID
        
        // 使用 Property Track 或自定义 Track 录制材质参数
        // 方案 1: 通过 UPropertyTrack 录制（如果支持）
        // 方案 2: 自定义 Track 类型
        
        // 录制关键帧到该 actor 独立的 track
        RecordMaterialOpacityKey(MovieScene, ActorGuid, StartFrame, StartOpacity);
        RecordMaterialOpacityKey(MovieScene, ActorGuid, EndFrame, EndOpacity);
    }
}
```

**新方案 B - 手动插值并在 Tick 中应用** (最灵活):

在动画播放时，手动计算 opacity 插值并调用 `mobject.SetOpacity()`:

```cpp
// 在 Animation Tick/Update 中
if (UUEMotionFadeAnimation* FadeAnim = ...)
{
    float Alpha = GetAnimationAlpha();  // 0.0 -> 1.0
    float CurrentOpacity = FMath::Lerp(StartOpacity, EndOpacity, Alpha);
    
    // 直接调用 mobject 的 SetOpacity（per-instance）
    TargetMobject->SetOpacity(CurrentOpacity);
}
```

**建议**: 对于 UEMotion 的数学可视化场景，**方案 B 更实用**，因为：

* 不依赖 Sequencer 的复杂 track 系统

* 代码更简单、更可控

* 可以在同一帧内精确控制多个 mobject 的 opacity

* 与现有的 Move/Rotate/Scale 动画架构一致

***

### Phase 4: 清理 MPC 相关代码

#### 文件: `UEMotionMobjectActor.cpp`

**4.1 移除 MPC 引用 (L7, L49)**

检查并移除 `UEMotionMobjectActor.cpp` 中对 MPC 的引用（如果仅用于 opacity）。

#### 文件: 头文件清理

**4.2 清理头文件**

**UEMotionMobject.h**:

```cpp
// 移除 (如果存在)
// #include "MaterialParameterCollection.h"
```

**UEMotionAssetFactory.h**:

```cpp
// 可选移除
// class UMaterialParameterCollection;
// UMaterialParameterCollection* EnsureFadeMPC();
```

***

### Phase 5: 测试与验证

#### 5.1 单元测试用例

创建测试脚本验证 per-instance opacity 控制：

```python
# test_opacity_per_instance.py
def test_multiple_objects_independent_opacity():
    """测试多个对象可以独立控制透明度"""
    scene = Scene("opacity_test")
    
    # 创建 3 个 cube
    cube1 = scene.cube(location=(-2, 0, 0), color="red")
    cube2 = scene.cube(location=(0, 0, 0), color="green")
    cube3 = scene.cube(location=(2, 0, 0), color="blue")
    
    # 设置不同的 opacity
    cube1.set_opacity(1.0)   # 完全不透明
    cube2.set_opacity(0.5)   # 半透明
    cube3.set_opacity(0.25)  # 接近透明
    
    # 渲染并验证每个 cube 的透明度独立
    scene.render()
    
    # 验证：cube1 最明显，cube3 最透明
    assert validate_opacity_independence(cube1, cube2, cube3)

def test_fade_animation_per_instance():
    """测试 fade 动画不影响其他对象"""
    scene = Scene("fade_test")
    
    box_a = scene.box(location=(-1, 0, 0))
    box_b = scene.box(location=(1, 0, 0))
    
    # 只对 box_a 执行 fade out 动画
    box_a.fade(duration=2.0, from_alpha=1.0, to_alpha=0.0)
    
    # box_b 应该保持完全不透明
    assert box_b.get_opacity() == 1.0
```

#### 5.2 回归测试

运行完整的 smoke test 确保：

* [ ] 基本流程测试通过 (`run_full_pipeline_test.bat`)

* [ ] 所有现有动画类型正常工作

* [ ] 材质渲染质量无明显变化

* [ ] 性能在可接受范围内

#### 5.3 视觉验证

手动验证场景：

1. 创建 3+ 个不同颜色的几何体
2. 分别设置不同的 opacity 值（0.25, 0.5, 0.75, 1.0）
3. 对其中一个执行 fade 动画
4. **预期结果**: 其他对象的透明度不受影响

***

## 📊 改动影响范围

### 需要修改的文件清单

| 文件                          | 改动类型          | 优先级 | 风险等级 |
| --------------------------- | ------------- | --- | ---- |
| `UEMotionMobject.cpp`       | 核心逻辑重构        | P0  | 中    |
| `UEMotionMobject.h`         | 头文件清理         | P1  | 低    |
| `UEMotionAssetFactory.cpp`  | 工厂逻辑修改        | P1  | 中    |
| `UEMotionAssetFactory.h`    | 接口声明更新        | P1  | 低    |
| `UEMotionScene.cpp`         | 动画系统重构        | P0  | 高    |
| `UEMotionMobjectActor.cpp`  | MPC 引用清理      | P2  | 低    |
| Python API (`animation.py`) | Fade 动画接口调整   | P1  | 中    |
| 测试脚本                        | 新增 opacity 测试 | P1  | 低    |

### 可选清理项（非必需）

* [ ] 删除 `/Game/UEMotion/Materials/MPC_UEMotionFade` 资产文件（如果不再使用）

* [ ] 清理 `AGENTS.md` 中关于 MPC 的文档

* [ ] 更新材质编辑器文档

***

## ⚠️ 注意事项与风险

### 兼容性风险

1. **已有项目迁移**: 如果有旧项目保存的 Sequence 包含 MPC Track，需要提供迁移工具
2. **材质实例兼容**: 已有的 `MaterialInstanceConstant` 资产可能需要重新生成

### 性能考虑

1. **内存开销**: 每 actor 一个 MID vs 全局一个 MPC

   * 典型场景 (<100 objects): 内存增加 <10MB，可忽略

   * 大规模场景 (>1000 objects): 需要关注 GPU 内存
2. **渲染性能**: MID 的 per-instance 参数在 GPU 端处理效率高，通常无性能损失

### 动画系统限制

1. **Sequencer 原生支持**: UE5 Sequencer 对 MID 参数的原生支持有限

   * 不像 MPC 有专门的 `MaterialParameterCollectionTrack`

   * 需要自定义 Track 或使用属性动画
2. **替代方案**: 如前所述，推荐在动画 tick 中手动插值

***

## 🔄 回滚方案

如果新方案出现问题，可以快速回滚：

1. 保留 MPC 相关代码在分支中
2. 通过 feature flag 切换: `bUsePerInstanceOpacity = true/false`
3. 回滚步骤:

   * 恢复 `UEMotionMobject.cpp` 中的 MPC 代码

   * 恢复 `UEMotionScene.cpp` 中的 MPC Track 逻辑

   * 重新编译插件

***

## ✅ 成功标准

1. **功能正确性**:

   * [x] 每个 mobject 可以独立设置 opacity (0.0 - 1.0)

   * [x] 修改 object A 的 opacity 不影响 object B

   * [x] Fade 动画只作用于目标对象

   * [x] 多个对象可以同时执行不同的 fade 动画

2. **性能指标**:

   * [x] 渲染帧率下降 < 5% (相比 MPC 实现)

   * [x] 内存开销增长 < 15% (典型场景)

3. **代码质量**:

   * [x] MPC 相关代码完全移除或隔离

   * [x] 新代码符合 UE5 编码规范

   * [x] 所有测试通过

   * [x] 文档更新完成

***

## 📅 实施时间线（预估）

| 阶段      | 任务                 | 预计时间         |
| ------- | ------------------ | ------------ |
| Phase 1 | 修改基础材质和 SetOpacity | 2-3 小时       |
| Phase 2 | 修改 Asset Factory   | 1-2 小时       |
| Phase 3 | 重构 Fade 动画系统       | 4-6 小时       |
| Phase 4 | 清理 MPC 代码          | 1 小时         |
| Phase 5 | 测试与调试              | 2-3 小时       |
| **总计**  | <br />             | **10-15 小时** |

***

## 🎓 参考资料

### UE5 官方文档

* [Material Instances](https://docs.unrealengine.com/5.4/en-US/instanced-materials-in-unreal-engine)

* [Using Transparency in Materials](https://docs.unrealengine.com/5.1/en-US/using-transparency-in-unreal-engine-materials)

* [UMaterialInstanceDynamic API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Materials/UMaterialInstanceDynamic)

### 技术文章

* [UE5 Glass Material Tutorial](https://www.worldofleveldesign.com/categories/ue5/standard-material-glass-simple.php)

* [Per-Instance Material Control Best Practices](https://subscription.packtpub.com/book/business-and-other/9781837633081/2/ch02lvl1sec13/instancing-a-material)

### API 参考

* **Python**: `unreal.MaterialInstanceDynamic.set_scalar_parameter_value()`

* **C++**: `UMaterialInstanceDynamic::SetScalarParameterValue(FName, float)`

