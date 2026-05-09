# UEMotion 默认地图机制 — 实施计划

## 目标

创建自定义默认地图，替代 UE 的 `Template_Default`，提供：

* 暗黑色背景（无天空盒/无地板）

* 二维坐标系（使用 `GizmoArrowHandle.GizmoArrowHandle`）

* 默认无光照渲染模式（Unlit）

* 可暴露的参数接口

## 当前问题分析

| 现状                                           | 问题                               |
| -------------------------------------------- | -------------------------------- |
| `CreateSceneMap()` 使用 `Template_Default` 模板  | 带天空、地板、光照的完整场景                   |
| `SetupDefaultLighting()` 创建 DirectionalLight | 不需要光照（Unlit 模式）                  |
| 无坐标轴                                         | 缺少数学可视化参考系                       |
| MRQ 渲染使用 DeferredPass                        | Unlit 应该用 ForwardShading 或直接关闭光照 |

## 实施步骤

### Step 1: 创建自定义 Map Template 资产 (`UEMotionScene.cpp`)

**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp`

修改 `CreateSceneMap()` 方法：

1. **不再使用** `Template_Default`
2. **创建空 World** 后手动配置：

   * 设置 `WorldSettings` → `bEnableWorldBoundsChecks = false`

   * 设置背景色为纯黑（通过 PostProcessVolume 或 WorldSettings）

   * **不添加** SkyAtmosphere / SkySphere / Floor / DirectionalLight
3. **保存为持久化资产** 到 `/Game/UEMotion/Maps/M_UEMotionDefault`（首次创建后复用）
4. **后续调用** 直接加载已存在的地图而非重建

新增配置属性到 `UUEMotionScene`：

```cpp
// UEMotionScene.h 新增
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Scene")
FLinearColor BackgroundColor = FLinearColor(0.01f, 0.01f, 0.02f, 1.0f);  // 近黑深蓝

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Scene")
bool bShowCoordinateAxes = true;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Scene")
float CoordinateAxisLength = 200.0f;  // UE units

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Scene")
bool bUseUnlitMode = true;
```

### Step 2: 创建坐标系 Actor (`UEMotionScene.cpp`)

新增方法 `SetupCoordinateAxes()`：

* 加载 `/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle` StaticMesh

* **X轴（红色）**: Spawn at `(0, 0, 0)` → 沿 +X 方向放置

* **Y轴（绿色）**: Spawn at `(0, 0, 0)` → 沿 +Y 方向放置

* 使用 `UMaterialInstanceDynamic` 设置颜色（X=红 R, Y=绿 G）

* Scale Z 轴方向使箭头变细（2D 效果）

* Axis Mesh 不加入 Sequencer（纯装饰参考）

坐标系结构：

```
Origin (0,0,0)
    ├── X-axis: GizmoArrowHandle (Red)   → (+Length, 0, 0)
    └── Y-axis: GizmoArrowHandle (Green)  → (0, +Length, 0)
```

### Step 3: 移除默认光照 / 切换 Unlit 模式

**修改** **`SetupDefaultLighting()`**：

* 当 `bUseUnlitMode == true` 时，**不创建任何光源**

* 当 `bUseUnlitMode == false` 时，保留现有光照逻辑（为未来开发预留）

**修改材质系统**（`UEMotionAssetFactory.cpp` / `UEMotionMobject.cpp`）：

* Unlit 模式下，材质 BlendMode 改为 `BLEND_Opaque`（不需要 Translucent 的光照交互）

* 但仍需支持 Opacity（用于 fade 动画），所以保持 `BLEND_Translucent` + 在材质中设置 `ShadingModel=MSM_Unlit`

具体做法：在材质创建时增加判断：

```cpp
if (bUseUnlitMode) {
    Material->ShadingModels.SetByEnum(MSM_Unlit);
}
```

### Step 4: 修改 MRQ 渲染配置 (`UEMotionRenderer.cpp`)

**修改** **`RenderSequence()`**：

* Unlit 模式下，不添加 `UMoviePipelineDeferredPassBase`

* 改用 Forward Rendering 或确保光照不影响输出

* 设置 `bOverrideWorldBackground` 为 true，背景色使用 `BackgroundColor`

```cpp
// Renderer 中根据 unlit 模式调整
if (!Config->FindOrAddSettingByClass(UMoviePipelineDeferredPassBase::StaticClass())) {
    // Unlit 模式跳过 deferred passes
}
// 替代方案：使用 UMoviePipelineInertialTransformBlend
```

### Step 5: Python API 暴露 (`scene.py`)

在 `Scene.__init__` 和新增方法中暴露参数：

```python
class Scene:
    def __init__(self, name="default", width=1920, height=1080,
                 background_color="#050508",  # 近黑深蓝
                 show_axes=True,
                 axis_length=4.0,           # UEMotion Units
                 unlit=True):
        ...
        self._ue.set_background_color(resolve_color(background_color))
        self._ue.set_show_coordinate_axes(show_axes)
        self._ue.set_coordinate_axis_length(axis_length * SCALE_FACTOR)
        self._ue.set_use_unlit(unlit)
```

### Step 6: C++ API 暴露 (`UEMotionScene.h`)

新增 BlueprintCallable 方法：

```cpp
UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
void SetBackgroundColor(const FLinearColor& Color);

UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
FLinearColor GetBackgroundColor() const;

UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
void SetShowCoordinateAxes(bool bShow);

UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
void SetCoordinateAxisLength(float Length);

UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
void SetUseUnlit(bool bUnlit);
```

## 文件修改清单

| 文件                          | 操作 | 说明                                                                                                |
| --------------------------- | -- | ------------------------------------------------------------------------------------------------- |
| `UEMotionScene.h`           | 修改 | 新增 5 个 UPROPERTY + 5 个 UFUNCTION                                                                  |
| `UEMotionScene.cpp`         | 修改 | 重写 `CreateSceneMap()`, `SetupDefaultLighting()`, 新增 `SetupCoordinateAxes()`, 修改 `Initialize()` 流程 |
| `UEMotionAssetFactory.cpp`  | 小改 | 材质 ShadingModel 条件设置                                                                              |
| `UEMotionMobject.cpp`       | 小改 | 同上（瞬态材质路径）                                                                                        |
| `UEMotionRenderer.cpp`      | 修改 | Unlit 模式下跳过 DeferredPass                                                                          |
| `Scripts/uemotion/scene.py` | 修改 | 构造函数 + 参数暴露                                                                                       |

## 数据流总览

```
Python: Scene(unlit=True, bg="#050508", show_axes=True)
  ↓
C++: Initialize()
  ├─ CreateSceneMap()     → 空 World + 黑色背景（无天空/地板）
  ├─ SetupCoordinateAxes() → X红/Y绿箭头 (GizmoArrowHandle)
  ├─ SetupDefaultLighting()→ Unlit模式跳过（不创建光源）
  └─ CreateLevelSequenceAsset()
  ↓
MRQ Render:
  └─ 无 DeferredPass → 纯颜色输出（不受光照影响）
```

## 验证方式

1. 运行 `cube_basic_animations.py`，确认：

   * 背景为暗黑色（非天空蓝）

   * 无地板反射

   * 有红色 X 轴 + 绿色 Y 轴

   * Cube 颜色纯净无光照影响

   * Fade 动画正常工作

