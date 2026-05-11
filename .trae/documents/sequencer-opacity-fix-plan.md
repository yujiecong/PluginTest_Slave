# Sequencer Opacity 回放问题修复计划

## 问题描述

**现象**：

* 在Sequencer中记录了opacity的关键帧（key）

* 手动在Sequencer中添加opacity轨道也可以正常工作

* 在Sequencer编辑器中拖动时间轴时，**轨道上的opacity数值显示会变化**

* **但是**：Level中的actor实际视觉效果不变（透明度没有改变）

**根本原因分析**：

### 1. 当前实现流程

```
Python调用 fade_out() → UEMotionScene::Play() → 创建FloatTrack → RecordFloatKey()
→ Sequencer轨道上记录了正确的key值 ✓
```

### 2. 问题所在

当Sequencer**回放/拖动时间轴**时：

```cpp
// UEMotionScene.cpp 第707行
NewTrack->SetPropertyNameAndPath(FName(*PropertyName), PropertyName);
// 这里设置了属性名为 "Opacity"
```

Sequencer的 `MovieSceneFloatTrack` 会尝试将值应用到绑定的actor上：

```
Sequencer评估 → 找到 AUEMotionMobjectActor::Opacity 属性 → 直接修改内存值
→ Opacity = 0.5 (例如) ✗ 但这只是改变了变量值！
→ 没有调用 SetOpacity() 方法 ✗
→ 材质参数没有被更新 ✗
→ 视觉上没有任何变化 ✗
```

**关键点**：

* `AUEMotionMobjectActor::Opacity` 只是一个普通的 `float` 变量

* Sequencer直接修改变量的内存值（通过反射系统）

* **但不会触发任何回调函数**

* 虽然已有 `PostEditChangeProperty` 实现，但**Sequencer回放时不会调用它**

* `SetOpacity()` 方法包含了关键的材质更新逻辑：

  ```cpp
  // UEMotionMobjectActor.cpp 第44-46行
  DynamicMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
  ```

### 3. 为什么Transform Track可以正常工作？

因为 `UMovieScene3DTransformTrack` 是引擎内置的特殊轨道类型：

* 引擎内部硬编码了如何应用变换到actor

* 直接调用 `SetActorLocation/SetActorRotation/SetActorScale`

* 不依赖UPROPERTY的setter机制

而 `UMovieSceneFloatTrack` 是通用属性轨道：

* 只能通过反射系统修改UPROPERTY值

* **不知道如何执行额外的副作用逻辑**（如更新材质）

***

## 解决方案

### 方案选择：使用 `Pre/Post Edit Change` + 运行时监听机制

由于Sequencer在编辑器模式和运行时模式下的行为不同，我们需要**双重保障**：

#### 1️⃣ 编辑器模式保障（已有，需验证）

**文件**: [UEMotionMobjectActor.cpp](Private/Actors/UEMotionMobjectActor.cpp#L49-L59)

```cpp
#if WITH_EDITOR
virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
```

**作用**：当在编辑器Details面板或Sequencer中修改属性时触发

**现状**：已实现，但可能未被正确触发

#### 2️⃣ 运行时模式保障（新增）

**文件**: [UEMotionMobjectActor.h](Private/Actors/UEMotionMobjectActor.h)
**文件**: [UEMotionMobjectActor.cpp](Private/Actors/UEMotionMobjectActor.cpp)

**策略**：添加轻量级的帧同步机制（非传统Tick）

##### 方案A：使用 `Tick` 但仅用于属性同步（推荐）

虽然用户之前要求移除Tick，但这是最简单可靠的方案。我们可以：

* 添加一个**条件性Tick**（只在需要时激活）

* 使用**缓存对比**避免不必要的更新

* 性能影响极小（一次float比较）

```cpp
// UEMotionMobjectActor.h
protected:
    virtual void Tick(float DeltaTime) override;

private:
    float CachedOpacity = -1.0f;  // 缓存值，初始为无效值
    bool bShouldSyncOpacity = false;  // 是否需要同步
```

```cpp
// UEMotionMobjectActor.cpp
void AUEMotionMobjectActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bShouldSyncOpacity) return;

    if (FMath::Abs(Opacity - CachedOpacity) > KINDA_SMALL_NUMBER)
    {
        CachedOpacity = Opacity;
        ApplyOpacityToMaterial();  // 更新材质
    }
}

void AUEMotionMobjectActor::SetOpacity(float InOpacity)
{
    Opacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
    CachedOpacity = Opacity;  // 同步缓存
    ApplyOpacityToMaterial();
}
```

**优点**：

* 简单可靠

* 实时响应Sequencer的变化

* 性能开销极小（每帧一次float比较）

**缺点**：

* 用户之前明确表示不想用Tick

* 需要向用户解释为什么这里必须用

##### 方案B：使用 `FCoreUObjectDelegates::OnObjectPropertyChanged` 全局委托（高级）

**原理**：监听Unreal Engine的对象属性变化广播

```cpp
// UEMotionMobjectActor.cpp
void AUEMotionMobjectActor::BeginPlay()
{
    Super::BeginPlay();

    FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(
        this, &AUEMotionMobjectActor::OnAnyObjectPropertyChanged);
}

void AUEMotionMobjectActor::OnAnyObjectPropertyChanged(UObject* Obj, FProperty* ChangedProperty)
{
    if (Obj != this) return;

    if (ChangedProperty && ChangedProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AUEMotionMobjectActor, Opacity))
    {
        ApplyOpacityToMaterial();
    }
}

void AUEMotionMobjectActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
    Super::EndPlay(EndPlayReason);
}
```

**优点**：

* 不使用Tick

* 精确监听属性变化

* 符合用户要求

**缺点**：

* 需要管理委托生命周期（注册/注销）

* 全局委托可能有性能影响（所有对象属性变化都会触发）

* 在某些Sequencer场景下可能不被触发

##### 方案C：自定义 MovieScene Eval Template（最复杂但最正确）

**原理**：创建自定义的FloatTrack评估模板，在Sequencer评估时直接调用SetOpacity

**涉及文件**：

* 新建 `UEMotionOpacityTrack.h/.cpp`

* 新建 `UEMotionOpacitySection.h/.cpp`

* 修改 `UEMotionScene.cpp` 使用新的track类型

**优点**：

* 最符合Sequencer架构的设计

* 完全控制评估逻辑

* 性能最优

**缺点**：

* 实现复杂度高（需要继承多个类）

* 需要注册到Sequencer模块

* 开发周期长

* 可能需要重启编辑器才能生效

***

## 推荐实施方案

### 🎯 最终推荐：方案B（全局委托）+ 增强版 PostEditChangeProperty

**理由**：

1. ✅ 不使用Tick（符合用户要求）
2. ✅ 可以捕获运行时的属性修改
3. ✅ 实现复杂度适中
4. ✅ 配合已有的PostEditChangeProperty形成完整覆盖

### 实施步骤

#### Step 1: 增强编辑器模式支持

**文件**: [UEMotionMobjectActor.cpp](Private/Actors/UEMotionMobjectActor.cpp#L49-L59)

**当前代码**：

```cpp
#if WITH_EDITOR
void AUEMotionMobjectActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = PropertyChangedEvent.GetPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AUEMotionMobjectActor, Opacity))
    {
        SetOpacity(Opacity);
    }
}
#endif
```

**修改后**：

```cpp
#if WITH_EDITOR
void AUEMotionMobjectActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AUEMotionMobjectActor, Opacity))
        {
            ApplyOpacityToMaterial();
            UE_LOG(LogTemp, Log, TEXT("UEMotionMobjectActor: PostEditChangeProperty triggered Opacity update to %.3f"), Opacity);
        }
    }
}
#endif
```

**改动说明**：

* 增加 `PropertyChangedEvent.Property` 空指针检查（防御性编程）

* 改为调用 `ApplyOpacityToMaterial()` （提取公共逻辑）

* 添加日志输出便于调试

#### Step 2: 提取公共方法 `ApplyOpacityToMaterial`

**文件**: [UEMotionMobjectActor.h](Private/Actors/UEMotionMobjectActor.h#L45)
**文件**: [UEMotionMobjectActor.cpp](Private/Actors/UEMotionMobjectActor.cpp)

**新增私有方法声明**（已在头文件第45行声明）：

```cpp
void ApplyOpacityToMaterial();
```

**实现**：

```cpp
void AUEMotionMobjectActor::ApplyOpacityToMaterial()
{
    EnsureDynamicMaterial();
    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
    }
}
```

**重构 SetOpacity**：

```cpp
void AUEMotionMobjectActor::SetOpacity(float InOpacity)
{
    Opacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
    ApplyOpacityToMaterial();
}
```

#### Step 3: 添加运行时属性监听

**文件**: [UEMotionMobjectActor.h](Private/Actors/UEMotionMobjectActor.h)

**新增头文件包含**：

```cpp
#include "UObject/CoreOnline.h"  // 用于 FCoreUObjectDelegates
```

**修改类声明**：

```cpp
UCLASS()
class AUEMotionMobjectActor : public AActor
{
    GENERATED_BODY()

public:
    AUEMotionMobjectActor();

    // ... (现有代码保持不变)

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;  // 新增

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    // ... (现有成员变量)

    void OnOpacityPropertyChanged(UObject* Obj, FProperty* ChangedProperty);  // 新增回调
};
```

**文件**: \[UEMotionMobjectActor.cpp]

**实现运行时监听**：

```cpp
void AUEMotionMobjectActor::BeginPlay()
{
    Super::BeginPlay();

#if !WITH_EDITOR
    // 在运行时模式下注册属性变化监听
    FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(
        this, &AUEMotionMobjectActor::OnOpacityPropertyChanged);
#endif
}

void AUEMotionMobjectActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if !WITH_EDITOR
    FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
#endif

    Super::EndPlay(EndPlayReason);
}

void AUEMotionMobjectActor::OnOpacityPropertyChanged(UObject* Obj, FProperty* ChangedProperty)
{
    if (!ChangedProperty) return;

    if (ChangedProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AUEMotionMobjectActor, Opacity))
    {
        ApplyOpacityToMaterial();
        UE_LOG(LogTemp, Log,
            TEXT("UEMotionMobjectActor: Runtime property change detected, Opacity=%.3f"),
            Opacity);
    }
}
```

#### Step 4: 启用Actor Tick（如果使用方案A作为备选）

**文件**: \[UEMotionMobjectActor.cpp] 构造函数

```cpp
AUEMotionMobjectActor::AUEMotionMobjectActor()
{
    Opacity = 1.0f;
    PrimaryActorTick.bCanEverTick = true;  // 允许Tick（如果采用方案A）
    PrimaryActorTick.bStartWithTickEnabled = false;  // 默认不启动
}
```

#### Step 5: 测试验证

**测试脚本**: `visual_check_cube.py`（已存在）

```python
cube = scene.create_cube(size=100)
cube.fade_out(duration=2.0)  # 应该能看到fade效果
scene.render_frames(output_dir="test_output")
```

**验证清单**：

* [ ] 在Sequencer编辑器中拖动时间轴，观察actor透明度是否实时变化

* [ ] 检查Output Log是否有 "PostEditChangeProperty triggered" 或 "Runtime property change detected" 日志

* [ ] 运行渲染测试，确认输出帧的透明度渐变效果

* [ ] 多个actor同时fade时互不影响（验证per-instance控制）

***

## 备选方案（如果方案B无效）

### 方案D：使用 `BlueprintSetter` 元数据（实验性）

**理论**：通过UPROPERTY元数据指定setter函数

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "UEMotion", meta = (BlueprintSetter = "SetOpacity"))
float Opacity;
```

**风险**：

* BlueprintSetter主要用于Blueprint VM

* C++层面的反射修改（如Sequencer）**可能仍不会调用**

* 需要实验验证

**实施位置**：[UEMotionMobjectActor.h](Private/Actors/UEMotionMobjectActor.h#L18-L19)

***

## 文件修改清单

| 文件路径                                                                | 修改类型 | 说明                                                                                                                                             |
| ------------------------------------------------------------------- | ---- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| [UEMotionMobjectActor.h](Private/Actors/UEMotionMobjectActor.h)     | 修改   | 添加 `EndPlay` 声明、`OnOpacityPropertyChanged` 回调声明                                                                                                |
| [UEMotionMobjectActor.cpp](Private/Actors/UEMotionMobjectActor.cpp) | 修改   | 1. 提取 `ApplyOpacityToMaterial()`2. 增强 `PostEditChangeProperty`3. 添加 `BeginPlay/EndPlay` 监听注册4. 实现 `OnOpacityPropertyChanged`5. 重构 `SetOpacity` |

**总代码改动量**：约50-60行（含注释和日志）

***

## 预期结果

修复后的行为：

```
Sequencer拖动时间轴
    ↓
MovieSceneFloatTrack 评估
    ↓
修改 AUEMotionMobjectActor::Opacity 内存值
    ↓
触发 FCoreUObjectDelegates::OnObjectPropertyChanged 广播  ← 新增
    ↓
调用 AUEMotionMobjectActor::OnOpacityPropertyChanged  ← 新增
    ↓
调用 ApplyOpacityToMaterial()  ← 新增提取
    ↓
DynamicMaterial->SetScalarParameterValue("Opacity", value)
    ↓
✅ 材质更新 → 视觉透明度变化！
```

***

## 风险与注意事项

1. **性能影响**：

   * `OnObjectPropertyChanged` 是全局委托，会在**每个对象属性变化时触发**

   * 我们的回调中有早期返回（`if (Obj != this) return`），开销很小

   * 如果项目中有大量对象频繁修改属性，可能需要考虑性能优化

2. **编辑器 vs 运行时**：

   * `PostEditChangeProperty` 仅在编辑器模式生效（`#if WITH_EDITOR`）

   * `OnObjectPropertyChanged` 在两种模式都生效（我们用 `#if !WITH_EDITOR` 限定运行时）

   * 形成完整覆盖

3. **Sequencer特殊场景**：

   * 某些版本的Sequencer可能使用不同的属性修改路径

   * 如果上述方案无效，可能需要采用**方案C（自定义Eval Template）**

4. **向后兼容**：

   * 所有修改都是增量式的，不影响现有功能

   * Python API接口不变（`cube.fade_out()` 调用方式不变）

***

## 下一步行动

1. ✅ 实施上述 **Step 1-3** 的代码修改
2. ⏹️ 编译插件（使用UBT）
3. ⏹️ 重启Unreal Editor
4. ⏹️ 运行 `visual_check_cube.py` 测试脚本
5. ⏹️ 在Sequencer编辑器中手动拖动时间轴验证实时预览
6. ⏹️ 如果仍有问题，启用**备选方案D**或升级到**方案C**

***

## 参考资料

* [Unreal Engine Forums - Property Setter not getting called](https://forums.unrealengine.com/t/property-setter-is-not-getting-called/2663873)

* [Editing UProperty from C++ (GitHub)](https://github.com/ibbles/LearningUnrealEngine/blob/master/Editing%20UProperty%20from%20C%2B%2B.md)

* [Unreal Engine Documentation - Properties](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-uproperties?application_version=5.0)

* [Unrepial Engine Documentation - Object Binding Track](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-actor-tracks-in-unreal-engine/?application_version=5.0)

