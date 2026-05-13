# 坐标轴可视化升级计划：从 Arrow 模型替换为真正的线条

## 📋 目标

将当前坐标轴使用的 **GizmoArrowHandle（箭头模型）** 替换为 **真正的线条（Line）**，提升视觉效果。

***

## 🔍 当前实现分析

### 当前方案

```
文件: UEMotionAxisActor.cpp (L109-L124)

static const FString GizmoMeshPath = TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle");
UStaticMesh* GizmoMesh = LoadObject<UStaticMesh>(nullptr, *GizmoMeshPath);
MeshComponent->SetStaticMesh(GizmoMesh);
```

### 问题

* ❌ 使用的是 **3D 箭头模型**（有体积、有箭头头部）

* ❌ 视觉上不够简洁、不够"数学化"

* ❌ 与 Manim 风格的坐标系不匹配

***

## ✅ 推荐方案：ProceduralMeshComponent 细圆柱体

### 技术选型

| 方案                          | 优点                  | 缺点                  | 推荐度   |
| --------------------------- | ------------------- | ------------------- | ----- |
| **ProceduralMeshComponent** | 完全控制几何体、真正线条形状、可调粗细 | 需要手动计算顶点            | ⭐⭐⭐⭐⭐ |
| ULineSetComponent           | API 简洁              | 依赖 ModelingTools 插件 | ⭐⭐⭐   |
| 细长 Box Mesh                 | 简单                  | 不是真正的线条             | ⭐⭐    |
| 自定义 Cylinder UAsset         | 可复用                 | 需要预创建资产             | ⭐⭐⭐   |

**最终选择：ProceduralMeshComponent + 圆柱体线条**

### 视觉效果对比

```
【当前 - Arrow 模型】          【目标 - 真正线条】
     ↗ X 轴                      ━━━ X 轴（红色）
    /                            │
   /                             │ Y 轴（绿色）
  ↙ Z 轴                        ┃ Z 轴（蓝色）
                                
  （3D箭头，有体积）              （细圆柱体，真正的线）
```

***

## 📝 实施步骤

### Step 1: 修改 UEMotionAxisActor.h

**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionAxisActor.h`

**修改内容**:

```cpp
// 替换组件类型
// 旧: UPROPERTY() UStaticMeshComponent* MeshComponent;
// 新:
UPROPERTY() UProceduralMeshComponent* LineMeshComponent;

// 添加线条配置参数
UPROPERTY(EditAnywhere, Category = "Axis")
float LineThickness{ 2.0f };      // 线条粗细（半径）
UPROPERTY(EditAnywhere, Category = "Axis")
float AxisLength{ 400.0f };       // 轴长度（UE单位）
UPROPERTY(EditAnywhere, Category = "Axis")
int32 LineSegments{ 8 };           // 圆柱体分段数（越大越圆滑）
```

### Step 2: 重写 SetupMesh() 函数

**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionAxisActor.cpp`

**核心逻辑** - 生成圆柱体几何体：

```cpp
void AUEMotionAxisActor::SetupMesh()
{
    if (!LineMeshComponent) return;
    
    // 清除旧几何体
    LineMeshComponent->ClearAllMeshSections();
    
    // 生成沿指定方向的圆柱体
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    
    const float HalfLength = AxisLength * 0.5f;
    const float Radius = LineThickness * 0.5f;
    
    // 根据轴向确定方向向量
    FVector Direction = GetAxisDirection();
    FVector Perp1, Perp2;
    Direction.FindBestAxisVectors(Perp1, Perp2);
    
    // 生成圆柱体顶点
    for (int32 i = 0; i <= LineSegments; ++i)
    {
        float Angle = (float)i / LineSegments * 2.0f * PI;
        float CosA = FMath::Cos(Angle);
        float SinA = FMath::Sin(Angle);
        
        FVector Offset = (Perp1 * CosA + Perp2 * SinA) * Radius;
        
        // 底部顶点
        Vertices.Add(-Direction * HalfLength + Offset);
        // 顶部顶点
        Vertices.Add(Direction * HalfLength + Offset);
        
        // 法线和颜色...
    }
    
    // 生成三角形索引...
    
    // 创建网格
    LineMeshComponent->CreateMeshSection_LinearColor(
        0, Vertices, Triangles, Normals, UVs, Colors, 
        TArray<FProcMeshTangent>(), true
    );
    
    // 应用材质
    CreateOrLoadAxisMaterial();
}
```

### Step 3: 更新 ApplyRotationForAxis()

**保持不变** - 旋转逻辑仍然适用，因为我们是沿轴生成长条

### Step 4: 材质适配

**保持现有材质系统**：

* M\_Axis\_X (红色) → 应用于 X 轴线条

* M\_Axis\_Y (绿色) → 应用于 Y 轴线条

* M\_Axis\_Z (蓝色) → 应用于 Z 轴线条

材质无需修改，只需应用到新的 ProceduralMeshComponent

### Step 5: 更新 SetupCoordinateAxes()

**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionSceneSetup.cpp`

**修改内容**:

* 移除对 `GizmoArrowHandle` 的引用

* 更新蓝图 SCS 配置（如需要）

* 确保 ProceduralMeshComponent 正确初始化

### Step 6: 编译测试

```bash
C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat PluginTestEditor Win64 Development ...
```

运行测试脚本验证效果：

```bash
python Scripts/examples/cube_basic_animations.py
```

***

## 🎯 配置参数说明

| 参数              | 默认值         | 说明                            |
| --------------- | ----------- | ----------------------------- |
| `LineThickness` | 2.0 UE cm   | 线条直径（视觉粗细）                    |
| `AxisLength`    | 400.0 UE cm | 轴总长度（约 8 UEMotion units）      |
| `LineSegments`  | 8           | 圆柱体圆周分段数（4=方柱, 8=近似圆, 16=光滑圆） |

***

## 📊 预期效果

### 视觉改进

```
【Before - Arrow】              【After - Line】
    ╭─╮                           ━━━━━━
    │ │ X                         ││││││ X (红)
    ╰─╯                           ┃┃┃┃┃┃
                                      
    ╭─╮                           ━━━━━━
    │ │ Y                         ││││││ Y (绿)
    ╰─╯                           
                                  ┃┃┃┃┃┃
    ╭─╮                           ━━━━━━
    │ │ Z                         ││││││ Z (蓝)
    ╰─╯                           
    
    3D箭头模型                    细圆柱体线条
    有体积感                       干净利落
```

### 技术优势

* ✅ **真正的线条**：无限细的视觉效果（可配置粗细）

* ✅ **性能友好**：ProceduralMesh 比 StaticMesh 更轻量

* ✅ **灵活配置**：粗细、长度、圆滑度都可调整

* ✅ **Manim 风格**：更接近数学软件的坐标轴表现

***

## ⚠️ 注意事项

1. **UAsset 兼容性**：现有的 BP\_Axis\_X/Y/Z 蓝图可能需要更新或重新生成
2. **MoviePipeline 渲染**：确保新线条在离线渲染中正确显示
3. **碰撞检测**：坐标轴应设置 `CollisionEnabled::NoCollision`
4. **阴影**：设置 `CastShadow = false`

***

## 📁 涉及文件清单

| 文件路径                               | 修改类型                            |
| ---------------------------------- | ------------------------------- |
| `.../Actors/UEMotionAxisActor.h`   | **修改** - 组件类型和参数声明              |
| `.../Actors/UEMotionAxisActor.cpp` | **重写** - SetupMesh() 函数         |
| `.../Core/UEMotionSceneSetup.cpp`  | **修改** - 移除 GizmoArrowHandle 引用 |

***

## ✅ 验收标准

* [ ] 坐标轴显示为**细线条**而非箭头模型

* [ ] X/Y/Z 轴颜色分别为**红/绿/蓝**

* [ ] 线条粗细可配置（默认 2cm 直径）

* [ ] 编译通过无错误

* [ ] 运行测试脚本正常显示

* [ ] MoviePipeline 渲染输出正确

