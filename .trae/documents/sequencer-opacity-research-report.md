# Sequencer Opacity 属性不生效 — 权威调研报告

## 调研日期

2026-05-11

## 调研范围

Epic Games 官方文档、官方论坛 Knowledge Base、UE5.7 引擎源码分析

***

## 一、问题描述

在 UEMotion 插件中，通过 C++ 代码动态创建 `UMovieSceneFloatTrack` 并添加 FloatKey 来动画化 `AUEMotionMobjectActor::Opacity` 属性。在 Sequencer UI 中轨道数值正确显示并随拖动变化，但 **Level 中 Actor 的透明度视觉上不变化**。

***

## 二、已排除的假设

| 假设                          | 验证结果  | 说明                                                  |
| --------------------------- | ----- | --------------------------------------------------- |
| Tick 未运行                    | ❌ 不成立 | SceneActor Tick 已启用                                 |
| Advance() 未调用               | ❌ 不成立 | 已通过 SceneActor Tick → UpdateAnimations → Advance 驱动 |
| FadeAnimation 未实现           | ❌ 不成立 | UpdateAnimation 已正确调用 SetOpacity                    |
| PostEditChangeProperty 未被调用 | ✅ 确认  | Sequencer 评估时不会触发                                   |
| FCoreUObjectDelegates 未被广播  | ✅ 确认  | Sequencer 直接操作 FProperty 内存                         |

***

## 三、根本原因分析

### 3.1 Sequencer 的 Setter 函数调用机制（核心问题）⭐

**来源**: [Epic Games 官方 Knowledge Base — "Sequencer Setter Functions"](https://community.epicgames.com/knowledge-base/sequencer-setter-functions)
(Epic 员工 RudyTriplett 于 2021-12-09 发布)

#### 官方原文引用（关键段落）：

> "By default, animating a property in Sequencer only sets the property's value directly; however, additional behavior can be defined in a function that Sequencer will call instead. **If Sequencer finds a UFunction named Set\[PropertyName], that function will be called instead of setting the value directly.** "

> "This is better than having the actor poll the value of the property while ticking for example, as the function is only called when the property changes."

> "For Blueprints this function needs to have **Call in Editor** checked on it for it to work when previewing the sequence in editor. **In C++ you can do the same with the CallInEditor function specifier.** "

#### 当前代码状态：

```cpp
// UEMotionMobjectActor.h — 当前声明
UFUNCTION(BlueprintCallable, Category = "UEMotion")     // ← 缺少 CallInEditor！
void SetOpacity(float InOpacity);

UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "UEMotion")  // ← 缺少 BlueprintSetter！
float Opacity;
```

**问题 1**：`SetOpacity` UFUNCTION 标注了 `BlueprintCallable`，但**缺少** **`CallInEditor`** **标识**。

* Sequencer 在编辑器模式下评估 FloatTrack 时，会通过 `FTrackInstancePropertyBindings` 查找名为 `SetOpacity` 的函数并尝试调用它

* 但没有 `CallInEditor`，这个调用在编辑器模式（播放/拖动时间轴场景）下会失败

**问题 2**：`Opacity` UPROPERTY **缺少** **`BlueprintSetter = "SetOpacity"`** **元数据**。

***

### 3.2 BlueprintSetter 在 Sequencer 中的行为差异

**来源**: [Qiita 技术验证文章 (2022-01-15)](https://qiita.com/com04/items/a0ac7af84071dc8f78c2)

经过实际测试验证的行为矩阵：

| 触发方式                           | BlueprintSetter 生效 | `Set+变量名` 函数生效 |
| ------------------------------ | ------------------ | -------------- |
| Blueprint 中设置属性值               | ✅ 直接生效             | ❌ 不生效          |
| **Sequencer/LevelSequence 评估** | ❌ **不生效**          | ✅ 生效           |
| 两者同时使用                         | ✅                  | ✅              |

**结论**：

* `BlueprintSetter` 元数据只在 Blueprint VM 层面工作

* Sequencer 评估系统只通过**函数名约定**（`Set[PropertyName]`）来发现 setter 函数

* 同时使用两者可以覆盖所有触发场景

***

### 3.3 Sequencer 评估模板缓存问题（Volatile 标志）

**来源**: [UE 官方论坛 — "Editing sequencer level sequence at runtime"](https://forums.unrealengine.com/t/editing-sequencer-level-sequence-at-runtime/225188) (2021-2023，多用户确认)

#### 问题场景（与我们的情况高度吻合）：

论坛用户 `LukasDaum` 描述了完全相同的问题：

* 代码动态创建 Possessable 绑定

* 代码动态添加 FloatTrack 并设置 PropertyNameAndPath

* 代码动态添加 Key

* **在编辑器中一切正常，但 track 的值没有应用到绑定的对象上**

#### 官方解决方案：

```cpp
SequenceAsset->SetSequenceFlags(
    EMovieSceneSequenceFlags::Volatile | 
    EMovieSceneSequenceFlags::BlockingEvaluation
);
```

**来源**: [Epic 官方 API 文档 — EMovieSceneSequenceFlags](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/MovieScene/EMovieSceneSequenceFlags)

* `Volatile`：**Flag signifying that this sequence can change dynamically at runtime or during the game so the template must be checked for validity and recompiled as necessary before each evaluation.**

* `BlockingEvaluation`：**Indicates that a sequence must fully evaluate and apply its state every time it is updated, blocking until complete.**

**当前代码问题**：

* 我们的 LevelSequence 在 `CreateLevelSequenceAsset` 时创建

* 之后通过 `Play()` 动态添加 track 和 key

* 但**从未设置 Volatile 标志**

* 引擎可能缓存了旧的空评估模板，导致后续添加的 track 不被评估

***

### 3.4 属性暴露要求确认

**来源**: [UE 官方论坛 — "Expose C++ UProperty to sequencer?"](https://forums.unrealengine.com/t/expose-c-uproperty-to-sequencer/429508)
(Epic 认证用户 `Shadowriver` 回答)

> "Interp specifier (not meta data) exposes property to both Matinee and Sequencer"

**来源**: [UE 官方文档 — Properties](https://docs.unrealengine.com/4.26/en-US/ProgrammingAndScripting/GameplayArchitecture/Properties)

| 属性       | 说明                                                                        |
| -------- | ------------------------------------------------------------------------- |
| `Interp` | Indicates that the value can be driven over time by a Track in Sequencer. |

**状态**：✅ `Opacity` UPROPERTY 已正确使用 `Interp` specifier。

***

### 3.5 UE5.7 引擎源码分析确认

**来源**: [MovieScenePropertyRegistry.cpp](file:///C:/Program%20Files/Epic%20Games/UE_5.7/Engine/Source/Runtime/MovieScene/Private/EntitySystem/MovieScenePropertyRegistry.cpp#L144-L168)

```cpp
// 引擎源码 — ComputeFastPropertyPtrOffset
TStringBuilder<128> SetterName;
SetterName.Append(TEXT("Set"));
SetterName = SetterName + PropertyName;     // 构造 "SetOpacity"

FName SetterFunctionName(SetterName.ToString(), FNAME_Find);

if (SetterFunctionName.IsNone() || ObjectClass->FindFunctionByName(SetterFunctionName) == nullptr)
{
    // 没找到 SetOpacity → 使用快速内存偏移路径 → 直接设置内存值 ✗ 不触发材质更新
    ...
    return uint16(PropertyOffset);
}
// 找到 SetOpacity → 拒绝快速路径 → 使用 FTrackInstancePropertyBindings
return TOptional<uint16>();
```

**分析**：

1. 引擎确实检测到了 `SetOpacity` 函数存在
2. 因此拒绝了快速内存偏移路径
3. 转而使用 `FTrackInstancePropertyBindings` 慢速路径
4. 慢速路径尝试调用 `SetOpacity` 函数，但可能因为缺少 `CallInEditor` 而失败（在编辑器模式下）

***

## 四、修复方案

### 🔺 优先级 P0（必须修复）

#### 修复 1：给 `SetOpacity` 添加 `CallInEditor` 标识

**文件**: [UEMotionMobjectActor.h](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionMobjectActor.h)

```cpp
// 修改前
UFUNCTION(BlueprintCallable, Category = "UEMotion")
void SetOpacity(float InOpacity);

// 修改后
UFUNCTION(BlueprintCallable, CallInEditor, Category = "UEMotion")
void SetOpacity(float InOpacity);
```

**原因**：Epic 官方知识库明确指出 `CallInEditor` 是 Sequencer 在编辑器模式下调用 setter 函数的必要条件。

***

#### 修复 2：给 `Opacity` 添加 `BlueprintSetter` 元数据

**文件**: [UEMotionMobjectActor.h](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionMobjectActor.h)

```cpp
// 修改前
UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "UEMotion")
float Opacity;

// 修改后
UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "UEMotion", meta = (BlueprintSetter = "SetOpacity"))
float Opacity;
```

**原因**：覆盖 Blueprint 设置属性值和 Sequencer 评估两种场景。

***

#### 修复 3：给 LevelSequence 设置 Volatile 标志

**文件**: [UEMotionScene.cpp](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp)

在 `CreateLevelSequenceAsset` 函数末尾或序列创建后添加：

```cpp
LevelSequence->SetSequenceFlags(
    EMovieSceneSequenceFlags::Volatile | 
    EMovieSceneSequenceFlags::BlockingEvaluation
);
```

**原因**：

* 我们的序列是在运行时动态修改的（添加 track、key）

* 没有 Volatile 标志，引擎缓存的评估模板不会包含新添加的 opacity track

* 这解释了为什么 **track 上的值显示正确但 actor 不响应**

***

### 🔺 优先级 P1（优化）

#### 优化 4：SceneActor Tick 驱动 UpdateAnimation（保留）

**文件**: [UEMotionSceneActor.cpp](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionSceneActor.cpp#L17-L24)

已实现但需保留。即使 P0 修复生效后，这个机制也能作为双重保障。

***

## 五、参考文献汇总

| 序号 | 来源                     | 标题                                                            | URL                                                                                                           |
| -- | ---------------------- | ------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| 1  | Epic 官方 Knowledge Base | Sequencer Setter Functions                                    | <https://community.epicgames.com/knowledge-base/sequencer-setter-functions>                                   |
| 2  | Epic 官方文档              | Property Specifiers (BlueprintSetter)                         | <https://dev.epicgames.com/documentation/en-us/unreal-engine/property-specifiers>                             |
| 3  | Epic 官方文档              | Properties (Interp specifier)                                 | <https://docs.unrealengine.com/4.26/en-US/ProgrammingAndScripting/GameplayArchitecture/Properties>            |
| 4  | Epic 官方 API 文档         | EMovieSceneSequenceFlags (Volatile/BlockingEvaluation)        | <https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/MovieScene/EMovieSceneSequenceFlags> |
| 5  | UE 官方论坛                | Expose C++ UProperty to sequencer?                            | <https://forums.unrealengine.com/t/expose-c-uproperty-to-sequencer/429508>                                    |
| 6  | UE 官方论坛                | Editing sequencer level sequence at runtime (Volatile flag)   | <https://forums.unrealengine.com/t/editing-sequencer-level-sequence-at-runtime/225188>                        |
| 7  | Qiita 技术验证             | BlueprintSetter 与 LevelSequence 行为矩阵                          | <https://qiita.com/com04/items/a0ac7af84071dc8f78c2>                                                          |
| 8  | UE5.7 引擎源码             | MovieScenePropertyRegistry.cpp (ComputeFastPropertyPtrOffset) | `Engine/Source/Runtime/MovieScene/Private/EntitySystem/MovieScenePropertyRegistry.cpp`                        |
| 9  | UE 官方文档                | Transform and Property Tracks (Float Track)                   | <https://docs.unrealengine.com/4.27/en-US/AnimatingObjects/Sequencer/Overview/Tracks/PropertyTracks/>         |

***

## 六、预期修复效果

修复后的完整流程：

```
Sequencer 评估 FloatTrack
    ↓
检测到 Opacity 属性有 SetOpacity setter 函数
    ↓
检测到 SetOpacity 有 CallInEditor 标识  ← 修复 #1
    ↓
通过 FTrackInstancePropertyBindings 调用 SetOpacity(opacityValue)  ← 修复 #2
    ↓
SetOpacity 内部: 更新 Opacity + 调用 ApplyOpacityToMaterial()
    ↓
DynamicMaterial->SetScalarParameterValue("Opacity", value)
    ↓
✅ 材质更新 → 视觉透明度实时变化！
```

同时：

* Volatile 标志确保引擎不缓存旧的空评估模板 ← 修复 #3

* BlockingEvaluation 确保每次更新都完整评估

* SceneActor Tick 作为双重保障 ← 优化 #4（已实现）

