# BP\_Axis 材质与旋转修复计划

## 问题分析

### 问题1：BP\_Axis\_X0 和 BP\_Axis\_Y0 使用相同的材质

**根因定位：**

1. **Blueprint SCS模板设置正确**：在 [UEMotionSceneSetup.cpp:271-303](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionSceneSetup.cpp#L271-L303) 中，创建蓝图时确实根据 BPName 设置了不同的材质（M\_Axis\_X/M\_Axis\_Y）
2. **Spawn后重复设置导致覆盖**：在 [UEMotionSceneSetup.cpp:333-384](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionSceneSetup.cpp#L333-L384) 中，Spawn Actor后又重新加载并设置材质
3. **`InitializeAxis()`** **内部冲突**：[UEMotionAxisActor::InitializeAxis()](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionAxisActor.cpp#L40-L68) 调用 `SetupMesh()` -> `CreateOrLoadAxisMaterial()` 可能会覆盖已设置的材质
4. **材质参数名不匹配**：[CreateStaticAxisMaterial():244](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionAxisActor.cpp#L244) 使用参数名 `"Color"`，但 `BasicShapeMaterial` 的参数名可能是 `"BaseColor"`

### 问题2：旋转不对

**根因定位：**

* [InitializeAxis():57](Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Actors/UEMotionAxisActor.cpp#L57)：Y轴旋转设置为 `(0, 90, 0)`

* GizmoArrowHandle 默认沿 +X 方向

* 在 Unreal Engine 中，Yaw=90 应该将 X轴旋转到 Y轴，但**实际效果可能因坐标系约定而不同**

* **正确的Y轴旋转应该是** **`(0, -90, 0)`** **或** **`(0, 270, 0)`**

***

## 修复方案

### 修复1：统一材质管理流程（解决材质相同问题）

#### 方案：移除重复的材质设置逻辑，保留蓝图中的原始设置

**修改文件：** `Private/Core/UEMotionSceneSetup.cpp`

**当前问题代码（第333-384行）：**

```cpp
if (AxisClass)
{
    AActor* SpawnedActor = SceneWorld.Get()->SpawnActor<AActor>(...);
    if (AUEMotionAxisActor* AxisActor = Cast<AUEMotionAxisActor>(SpawnedActor))
    {
        // 1. InitializeAxis 会调用 SetupMesh -> CreateOrLoadAxisMaterial
        AxisActor->InitializeAxis(AxisType, Len, Color);
        
        // 2. 这里又重新设置材质（可能覆盖了蓝图的正确设置！）
        UStaticMeshComponent* AxisMesh = AxisActor->GetMeshComponent();
        if (AxisMesh)
        {
            // ... 重新加载材质的逻辑 ...
            AxisMesh->SetMaterial(0, AxisMaterial); // ← 问题根源
        }
    }
}
```

**修复后的代码：**

```cpp
if (AxisClass)
{
    AActor* SpawnedActor = SceneWorld.Get()->SpawnActor<AActor>(...);
    if (AUEMotionAxisActor* AxisActor = Cast<AUEMotionAxisActor>(SpawnedActor))
    {
        EAxis::Type AxisType = static_cast<EAxis::Type>(i);
        
        // 只初始化轴类型、长度和颜色，不重新设置材质（蓝图已经设置了正确的材质）
        AxisActor->AxisType = AxisType;
        AxisActor->AxisLength = Len;
        AxisActor->AxisColor = Color;
        
        // 手动设置旋转（绕过 SetupMesh 的材质覆盖）
        if (UStaticMeshComponent* AxisMesh = AxisActor->GetMeshComponent())
        {
            static UStaticMesh* GizmoMesh = LoadObject<UStaticMesh>(
                nullptr, TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle"));
            if (GizmoMesh)
            {
                AxisMesh->SetStaticMesh(GizmoMesh);
            }
            
            // 应用正确的旋转
            FRotator TargetRotation = FRotator::ZeroRotator;
            switch (AxisType)
            {
            case EAxis::X:
                TargetRotation = FRotator(0, 0, 0);      // X轴：不旋转
                break;
            case EAxis::Y:
                TargetRotation = FRotator(0, -90, 0);     // Y轴：绕Z轴旋转-90度
                break;
            case EAxis::Z:
                TargetRotation = FRotator(90, 0, 0);      // Z轴：绕Y轴旋转90度
                break;
            default:
                break;
            }
            AxisMesh->SetRelativeRotation(TargetRotation);
            AxisMesh->SetRelativeScale3D(FVector(Len / 40.0f, 0.2f, 0.2f));
            
            UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Setup axis %d with rotation (%.0f, %.0f, %.0f)"),
                i, TargetRotation.Pitch, TargetRotation.Yaw, TargetRotation.Roll);
        }
    }
}
```

### 修复2：修正旋转角度（解决旋转不对问题）

**修改文件：** `Private/Actors/UEMotionAxisActor.cpp`

**当前错误代码（第50-67行和第130-148行）：**

```cpp
// InitializeAxis() 中的旋转设置
case EAxis::Y:
    TargetRotation = FRotator(0, 90, 0);  // ❌ 错误！
    break;

// ApplyRotationForAxis() 中的旋转设置  
case EAxis::Y:
    TargetRotation = FRotator(0, 90, 0);  // ❌ 错误！
    break;
```

**修正为：**

```cpp
case EAxis::X:
    TargetRotation = FRotator(0, 0, 0);       // X轴：沿+X方向
    break;
case EAxis::Y:
    TargetRotation = FRotator(0, -90, 0);     // Y轴：绕Z轴顺时针旋转90度 → 指向+Y
    break;
case EAxis::Z:
    TargetRotation = FRotator(90, 0, 0);      // Z轴：绕Y轴旋转90度 → 指向+Z
    break;
```

### 修复3：确保材质参数名正确（可选增强）

**修改文件：** `Private/Actors/UEMotionAxisActor.cpp`

**当前代码（第244行）：**

```cpp
NewMaterialInstance->SetVectorParameterValueEditorOnly(FName("Color"), Color);
```

**验证并修正为：**

```cpp
// BasicShapeMaterial 使用 "BaseColor" 参数名（不是 "Color"）
NewMaterialInstance->SetVectorParameterValueEditorOnly(FName("BaseColor"), Color);
```

> ⚠️ **注意**：如果 `BasicShapeMaterial` 确实使用 `"Color"` 参数名，则此修改不需要。建议先查看材质定义确认。

***

## 实施步骤

### 步骤1：修复 UEMotionSceneSetup.cpp 中的轴创建逻辑

* 移除 `InitializeAxis()` 调用（避免触发 `SetupMesh()` 的材质覆盖）

* 改为直接设置属性（AxisType, AxisLength, AxisColor）

* 手动配置 mesh、旋转和缩放

* **保留蓝图中的材质设置不变**

### 步骤2：修复 UEMotionAxisActor.cpp 中的旋转角度

* 修正 Y轴旋转从 `90` 改为 `-90`

* 同步更新 `InitializeAxis()` 和 `ApplyRotationForAxis()` 两处

### 步骤3：（可选）验证并修正材质参数名

* 确认 `BasicShapeMaterial` 的向量参数名是 `"Color"` 还是 `"BaseColor"`

* 如需要则修正

### 步骤4：编译验证

* 运行 Build.bat 编译项目

* 确保无编译错误

### 步骤5：功能测试

* 运行 Python 测试脚本验证：

  * BP\_Axis\_X 显示红色

  * BP\_Axis\_Y 显示绿色

  * X轴水平向右

  * Y轴垂直向上（或在正确的方向）

***

## 预期结果

✅ **材质修复**：X轴红色、Y轴绿色、Z轴蓝色（各不相同）\
✅ **旋转修复**：X轴→+X方向、Y轴→+Y方向、Z轴→+Z方向\
✅ **代码简化**：移除冗余的材质设置逻辑，职责更清晰

***

## 风险评估

* **低风险**：修改仅限于轴创建和初始化逻辑

* **向后兼容**：不影响现有 API 接口

* **可回滚**：如出现问题可快速恢复到原逻辑

