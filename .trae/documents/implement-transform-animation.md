# Transform 动画效果实现计划

## 目标

实现类似 Manim 的 `Transform` 效果，支持将一个形状（如 Square）变形为另一个形状（如 Circle）。

## 用户示例代码（期望效果）

```python
class SquareToCircle(Scene):
    def construct(self):
        circle = Circle()
        circle.set_fill(PINK, opacity=0.5)

        square = Square()
        square.rotate(PI / 4)

        self.play(Create(square))      # 创建方形动画
        self.play(Transform(square, circle))  # 方形变形为圆形 ⭐
        self.play(FadeOut(square))     # 淡出
```

---

## 技术方案分析

### 方案对比

| 方案 | 原理 | 优点 | 缺点 | 适用场景 |
|------|------|------|------|----------|
| **A. Shader顶点变形** | World Position Offset | GPU高效、平滑、实时 | 需要相同拓扑 | ✅ **推荐用于UEMotion** |
| B. ProceduralMesh逐帧更新 | CPU修改顶点 | 完全控制 | 性能差、复杂 | 复杂拓扑变化 |
| C. Morph Target预计算 | UE内置变形目标 | 高效 | 需要预生成、不灵活 | 固定形状组合 |
| D. 替换+淡入淡出 | 视觉欺骗 | 最简单 | 不是真正变形 | 快速原型 |

### 推荐方案：Shader-based Vertex Morphing (方案 A)

使用 Unreal 的 **World Position Offset** 材质节点，通过自定义 Shader 实现顶点插值。

**核心原理：**
```
最终位置 = lerp(源形状位置, 目标形状位置, MorphProgress)
```

---

## 详细实现步骤

### 第一阶段：C++ 核心架构（3个文件）

#### 1. 创建 Transform Animation 类

**新文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Anim/UEMotionTransformAnimation.h`

```cpp
UCLASS(BlueprintType)
class UUEMotionTransformAnimation : public UUEMotionAnimation
{
    GENERATED_BODY()

public:
    // 设置源和目标 Mobject
    void SetSourceMobject(UUEMotionMobject* Source);
    void SetTargetMobject(UUEMotionMobject* Target);
    
    // 获取当前插值进度 (0.0 -> 1.0)
    float GetMorphProgress() const;

protected:
    virtual void UpdateAnimation(float DeltaTime, float EasedProgress) override;
    
private:
    UPROPERTY()
    UUEMotionMobject* SourceMobject = nullptr;  // 源形状
    
    UPROPERTY()
    UUEMotionMobject* TargetMobject = nullptr;  // 目标形状
    
    float MorphProgress = 0.0f;
};
```

#### 2. 创建 Morph Material Manager

**新文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionMorphMaterialManager.h/cpp`

职责：
- 创建带有 World Position Offset 的变形材质
- 管理 SourcePositionTexture 和 TargetPositionTexture
- 提供 SetMorphProgress() 接口

**材质结构（HLSL伪代码）：**

```hlsl
// 输入参数
float MorphProgress;           // 0.0 = 源形状, 1.0 = 目标形状
Texture2D SourcePositions;     // 源形状顶点位置纹理
Texture2D TargetPositions;     // 目标形状顶点位置纹理

// World Position Offset 节点
float3 GetWorldPositionOffset(float3 CurrentPosition, float2 UV)
{
    float3 SourcePos = SampleSourcePosition(UV);
    float3 TargetPos = SampleTargetPosition(UV);
    
    return lerp(SourcePos, TargetPos, MorphProgress) - CurrentPosition;
}
```

#### 3. 扩展 SceneAnimation 支持 Transform

**修改文件**: `Private/Core/UEMotionSceneAnimation.cpp`

在 `Play()` 函数中添加 Transform 分支：

```cpp
else if (UUEMotionTransformAnimation* TransformAnim = Cast<UUEMotionTransformAnimation>(Animation))
{
    // 1. 提取源和目标的顶点数据
    // 2. 生成位置纹理
    // 3. 应用变形材质到源对象
    // 4. 记录 MorphProgress 关键帧 (0.0 -> 1.0)
}
```

---

### 第二阶段：顶点数据处理（关键技术）

#### 问题：不同拓扑的形状如何变形？

**解决方案：统一网格重采样**

```
Square (4顶点) ──重采样──> UnifiedMesh (64顶点) ──变形──> Circle (64顶点)
                        ↑                              ↑
                    相同顶点数                      相同顶点数
```

**实现函数**（在 UEMotionAssetFactory 中扩展）：

```cpp
// 为两个形状生成兼容的统一网格
struct FMorphPair
{
    TArray<FVector> SourceVertices;   // 源形状顶点（统一数量）
    TArray<FVector> TargetVertices;   // 目标形状顶点（统一数量）
    int32 VertexCount;                // 统一顶点数（建议64或128）
};

FMorphPair GenerateMorphCompatibleMeshes(
    EShapeType SourceShape,   // e.g., Square
    EShapeType TargetShape,   // e.g., Circle
    float Size,
    int32 Resolution = 64     // 统一分辨率
);
```

**重采样算法（伪代码）：**

```cpp
for (int i = 0; i < Resolution; i++)
{
    float t = (float)i / (float)(Resolution - 1);
    
    // 在源形状边界上采样
    SourceVertices[i] = SampleShapeBoundary(SourceShape, t, Size);
    
    // 在目标形状边界上采样  
    TargetVertices[i] = SampleShapeBoundary(TargetShape, t, Size);
}

// 对于2D形状，使用带状三角形连接顶点形成面
```

---

### 第三阶段：材质与着色器实现

#### 变形材质蓝图结构

```
Material Inputs:
├── Base Color: Parameter (继承自源对象颜色)
├── Opacity: Parameter (继承自源对象透明度)
├── Metallic: 0.0
├── Roughness: 0.5
└── World Position Offset: [Custom Node]
    ├── TextureSample (SourcePositions)
    ├── TextureSample (TargetPositions)
    ├── ScalarParameter (MorphProgress)
    └── Lerp + Subtract (CurrentPosition)
```

**程序化创建材质代码**（在 UEMotionMorphMaterialManager 中）：

```cpp
UMaterialInterface* CreateMorphMaterial(
    const TArray<FVector>& SourceVerts,
    const TArray<FVector>& TargetVerts
);
```

---

### 第四阶段：Python API 集成

#### 扩展 Python Scene 类

**修改文件**: `Scripts/uemotion/scene.py`

添加方法：

```python
def transform(self, source_mobject, target_mobject, duration=1.0, easing="linear"):
    """
    将 source_mobject 变形为 target_mobject 的形状
    
    Args:
        source_mobject: 要变形的对象（会被修改）
        target_mobject: 目标形状（仅作为参考，不会被显示）
        duration: 动画时长
        easing: 缓动函数类型
    """
    transform_anim = Animation.create_transform(source_mobject, target_mobject)
    transform_anim.duration = duration
    transform_anim.easing = easing
    self._pending_animations.append(transform_anim)
```

#### Python 使用示例

```python
from uemotion import Scene, ORIGIN, PINK, PI

s = Scene("square_to_circle", 1920, 1080, mode="2d")

# 创建圆形（目标形状）
circle = s.circle(radius=1.0, color=PINK, opacity=0.5)

# 创建方形（将被变形的对象）
square = s.square(side_length=1.8, color=PINK, opacity=0.5)
square.rotate(PI / 4)  # 旋转45度变成菱形

# 播放动画序列
s.play()  # 显示初始状态

# ★ 核心：方形变形为圆形
s.transform(square, circle, duration=2.0, easing="ease_in_out")
s.play()

s.fade_out(square, duration=1.0)
s.play()

print("[PASS] SquareToCircle complete!")
```

---

### 第五阶段：示例脚本

**新文件**: `Scripts/examples/square_to_circle_demo.py`

完整演示 Transform 功能：
1. 创建菱形（旋转的方形）
2. Create 动画显示菱形
3. **Transform 动画：菱形 → 圆形**
4. FadeOut 动画消失

---

## 文件清单

### 新建文件（6个）

| 文件路径 | 说明 |
|---------|------|
| `Private/Anim/UEMotionTransformAnimation.h` | Transform动画类定义 |
| `Private/Anim/UEMotionTransformAnimation.cpp` | Transform动画类实现 |
| `Private/Core/UEMotionMorphMaterialManager.h` | 变形材质管理器头文件 |
| `Private/Core/UEMotionMorphMaterialManager.cpp` | 变形材质管理器实现 |
| `Scripts/examples/square_to_circle_demo.py` | Transform功能演示脚本 |

### 修改文件（4个）

| 文件路径 | 修改内容 |
|---------|---------|
| `Private/Core/UEMotionSceneAnimation.cpp` | 添加Transform分支处理逻辑 |
| `Private/Core/UEMotionSceneAnimation.h` | 声明新的辅助方法 |
| `Private/Core/UEMotionAssetFactory.cpp` | 添加GenerateMorphCompatibleMeshes() |
| `Scripts/uemotion/scene.py` | 添加transform() Python API |

---

## 技术难点与解决方案

### 难点 1：不同拓扑的顶点对应关系
**解决**：使用参数化边界采样，确保源和目标有相同数量的顶点

### 难点 2：World Position Offset 的坐标系问题
**解决**：所有位置转换为 Local Space，避免世界坐标偏移

### 难点 3：法线和切线在变形过程中的正确性
**解决**：使用 UE 的自动重新计算法线选项，或在 Shader 中计算

### 难点 4：Sequencer 集成（记录关键帧）
**解决**：将 MorphProgress 作为 Scalar Parameter 记录到 Float Track

---

## 实现优先级

1. **P0 - 必须实现**：
   - TransformAnimation 基础类
   - 顶点重采样算法
   - World Position Offset 材质
   - Sequencer 关键帧记录

2. **P1 - 重要优化**：
   - 法线自动重建
   - 支持更多形状组合（Triangle→Circle, Star→Heart等）

3. **P2 - 锦上添花**：
   - 自定义缓动曲线
   - 变形过程中颜色渐变
   - 3D 形状变形支持

---

## 验证标准

- [ ] `s.transform(square, circle)` 能正确执行
- [ ] 变形过程平滑无闪烁
- [ ] 最终形状与目标形状一致
- [ ] Sequencer 时间轴正确显示关键帧
- [ ] Python 示例脚本可运行且输出预期结果

---

## 预计工作量

- C++ 核心代码：~400行
- 材质管理代码：~250行
- Python API：~50行
- 示例脚本：~40行
- **总计：~740行新代码**
