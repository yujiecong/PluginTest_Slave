# UEMotion Plugin - Agents & Automation

## Overview

UEMotion Plugin 自动化测试系统基于 Unreal Engine Python 脚本和批处理文件构建，用于验证插件的核心功能、渲染管线和动画系统的正确性。

## Test Architecture

```
PluginTest/
├── run_full_pipeline_test.bat          # 基本流程测试入口（Smoke Test）
├── run_tests.bat                       # 完整测试套件入口
└── Scripts/
    ├── run_full_pipeline_test.py       # 端到端流程测试脚本
    ├── run_tests_entry.py              # 测试套件主入口
    ├── tests/                          # 单元测试 & 集成测试
    │   ├── test_framework.py           # 测试框架基础类
    │   ├── test_01_scene.py            # 场景创建测试
    │   ├── test_02_mobject.py          # Mobject 基础测试
    │   ├── test_03_camera.py           # 相机控制测试
    │   ├── test_04_animation_move.py   # 移动动画测试
    │   └── ... (test_05 ~ test_16)
    └── uemotion/                       # Python API 封装
        ├── constants.py                # ⭐ 坐标系常量定义
        ├── scene.py                    # Scene 类（含坐标系支持）
        ├── camera.py                   # Camera 类
        ├── mobject.py                  # Mobject 类（含便捷定位方法）
        └── animation.py                # Animation 类
```

---

## Coordinate System Specification (1:1 Square)

### Overview

UEMotion 使用 **1:1 正方形坐标系**，灵感来自 Manim，专为数学可视化设计。

```
┌─────────────────────┐
│                     │
│    UL      ↑ UR     │  Q2 | Q1
│         ↗ | ↘       │  (-,+)|(+,+)
│    ←──────┼──────→  │
│         ↙ | ↖       │  (-,-)|(+,-)
│    DL      ↓ DR     │  Q3 | Q4
│                     │
└─────────────────────┘

原点 (ORIGIN): 屏幕中心 (0, 0, 0)
X 轴: 向右为正 (+)
Y 轴: 向上为正 (+)
帧尺寸: 8×8 UEMotion Units (正方形)
分辨率: 1080×1080 像素 (默认)
```

### Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `FRAME_WIDTH` | 8.0 | Frame width (UEMotion units) |
| `FRAME_HEIGHT` | 8.0 | Frame height (UEMotion units) |
| `ASPECT_RATIO` | 1.0 | 1:1 square |
| `ORIGIN` | (0, 0, 0) | Screen center |
| `UP` | (0, 1, 0) | Y+ direction |
| `DOWN` | (0, -1, 0) | Y- direction |
| `LEFT` | (-1, 0, 0) | X- direction |
| `RIGHT` | (1, 0, 0) | X+ direction |
| `UL` | (-4, 4, 0) | Upper-left corner (Q2) |
| `UR` | (4, 4, 0) | Upper-right corner (Q1) |
| `DL` | (-4, -4, 0) | Lower-left corner (Q3) |
| `DR` | (4, -4, 0) | Lower-right corner (Q4) |

### Four Quadrants

| Quadrant | Coordinates | Corner Constants | Sign |
|----------|-------------|-------------------|------|
| Q1 | x > 0, y > 0 | UR | (+, +) |
| Q2 | x < 0, y > 0 | UL | (-, +) |
| Q3 | x < 0, y < 0 | DL | (-, -) |
| Q4 | x > 0, y < 0 | DR | (+, -) |

### Unit Conversion

```python
SCALE_FACTOR = 50.0  # 1 UEMotion Unit = 50 UE cm

# Examples:
# ORIGIN (0,0,0) → UE (0, 0, 0) cm
# UR (4,4,0) → UE (200, 200, 0) cm
# DL (-4,-4,0) → UE (-200, -200, 0) cm
```

### API Quick Reference

```python
from uemotion import Scene
from uemotion.constants import *

s = Scene("demo")  # Default 1080x1080, auto-configured 2D camera

# Position using constants
box = s.cube(size=25, color="cyan", location=DL)     # Start at Q3
box.move_to(ORIGIN, duration=2)                        # Animate to center
box.move_to(UR, duration=2)                            # Animate to Q1

# Convenience methods
box.to_edge(RIGHT)           # Move to right edge
box.next_to(other, UP)       # Place above another object
box.shift(LEFT * 2)          # Move left by 2 units

# Scene properties
s.frame_width   # 8.0
s.frame_height  # 8.0
s.center        # (0, 0, 0)
s.get_corner('UL')  # (-4, 4, 0)
```

---

## Basic Flow Test (Smoke Test)

**Entry Point**: `run_full_pipeline_test.bat`

### Purpose

这是 **最基本的回归测试**，用于快速验证整个 UEMotion Pipeline 是否正常工作。每次代码变更后都应运行此测试，确保核心功能未被破坏。

### What It Tests

1. **Scene Creation**: 创建 3D 场景、灯光设置（使用标准坐标系）
2. **Coordinate System**: 验证 1:1 正方形坐标系配置
3. **Camera Setup**: 标准 2D 俯视相机（自动配置）
4. **Object Animation**: Cube 从 Q3 (DL) 沿 y=x 路径移动到 Q1 (UR)，穿过原点
5. **Keyframe Recording**: Sequencer 关键帧记录与通道映射（Roll/Pitch/Yaw for UE5.7+）
6. **MoviePipeline Rendering**: 渲染 151 帧 PNG 序列（1080x1080 @ 30fps）
7. **Output Validation**: 文件数量、大小、格式验证
8. **UE Shutdown**: 测试完成后自动关闭 UE Editor

### Test Parameters (v2 - 1:1 Coordinate System)

```python
Scene Name     : "y_equals_x"
Resolution     : 1080x1080 (1:1 square)
Frame Size     : 8×8 UEMotion Units
Animation      : 5 steps × 1.0s = 5.0s total
FPS            : 30
Expected Frames: 151 (5.0s × 30 + 1)
Path           : y=x from DL (-4,-4,Q3) to UR (4,4,Q1)
Camera         : standard 2D top-down at Z=500
Lighting       : Directional + Point Light at ORIGIN
Cube           : Size=25 (0.5×SCALE), Color=Cyan, Z=0
```

### Visual Output

```
Cube trajectory: DL (-4,-4) → ORIGIN (0,0) → UR (4,4)

┌─────────────────────┐
│                     │
│               ● END │  Q1 (+,+)  终点
│            ●        │
│         ●           │
│      ●              │
│   ● ORIGIN          │  中心穿越点
│      ●              │
│         ●           │
│            ●        │
│  START ●            │  Q3 (-,-)  起点
│                     │
└─────────────────────┘

✓ Complete four-quadrant visualization
✓ Mathematical coordinate system mapping
✓ 1:1 aspect ratio preserves X/Y equality
```

### Execution

```bash
# Windows
cd c:\Users\42458\Documents\Unreal Projects\PluginTest
run_full_pipeline_test.bat

# Expected Output:
# [1/4] Creating scene...
# [2/4] Creating cube at DL (Q3) and animating along y=x to UR (Q1)...
# [3/4] Rendering frames via MoviePipeline...
# [4/4] Validating output images...
# >>> RESULT: ALL CHECKS PASSED <<<
```

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | All checks passed |
| 1 | One or more checks failed |

### Validation Criteria

- ✅ Scene object created with 1:1 frame dimensions
- ✅ Frame width = 8.0, Frame height = 8.0
- ✅ Camera at standard Z position (500 UE units)
- ✅ Cube created at DL (-4, -4, 0) [Q3]
- ✅ Cube final position = UR (4, 4, 0) [Q1]
- ✅ MoviePipeline render callback success=True
- ✅ Frame count = 151
- ✅ File sizes in range [1MB, 3MB]
- ✅ All files are .png format

---

## Full Test Suite

**Entry Point**: `run_tests.bat`

运行所有单元测试和集成测试（test_01 ~ test_16），覆盖：

- Scene/Camera/Mobject 基础功能
- Move/Rotate/Scale/Fade/Color/Group 动画
- Easing functions
- Light system
- Renderer pipeline
- Python wrapper API
- Boundary conditions

---

## Integration with CI/CD

### GitHub Actions Example

```yaml
name: UEMotion Smoke Test
on: [push, pull_request]
jobs:
  test:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Run Basic Flow Test
        run: |
          cd PluginTest
          run_full_pipeline_test.bat
          
      - name: Check Result
        if: failure()
        run: echo "Smoke test failed!"
        
      - name: Upload Artifacts
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: rendered-frames
          path: PluginTest/Saved/UEMotionTest/full_pipeline/y_equals_x_frames/
```

### Pre-commit Hook

```bash
#!/bin/bash
# .git/hooks/pre-commit

echo "Running UEMotion smoke test..."
cd "$(git rev-parse --show-toplevel)/PluginTest"

./run_full_pipeline_test.bat
if [ $? -ne 0 ]; then
    echo "ERROR: Smoke test failed! Commit aborted."
    exit 1
fi

echo "Smoke test passed. Proceeding with commit..."
```

---

## Troubleshooting

### Common Issues

1. **UE not shutting down after test**
   - Ensure `unreal.SystemLibrary.quit_editor()` is called in callback
   - Check that `set_keep_python_script_alive(False)` is set

2. **Frame count mismatch**
   - Verify TOTAL_DURATION and FPS calculation
   - Check Sequencer playback range matches expected duration

3. **Camera orientation incorrect**
   - Rotation channels must be Roll/Pitch/Yaw order (UE5.7+)
   - See `UEMotionSequencerCompat.h` for implementation

4. **Vector subscript error**
   - Use `.x/.y/.z` properties instead of `[0]/[1]/[2]`
   - Unreal Vector objects are not subscriptable in Python

5. **Coordinate system issues**
   - Ensure using constants from `uemotion.constants`
   - Verify SCALE_FACTOR is correctly applied
   - Check that Scene auto-configures camera in `_setup_standard_2d_camera()`

### Log Files

```
Saved/Logs/PluginTest.log          # Main UE log
Scripts/tests/test_report/report.json  # Test suite results
```

---

## Development Guidelines

### Adding New Tests

1. Create new test file in `Scripts/tests/test_XX_name.py`
2. Inherit from `TestFramework` base class
3. Implement `setup()`, `run()`, `teardown()` methods
4. Register in `run_tests_entry.py`
5. Run smoke test to ensure no regression

### Modifying Core Code

After any changes to:
- `UEMotionCamera.cpp/h` (camera control)
- `UEMotionSequencerCompat.h` (keyframe recording)
- `UEMotionRenderer.cpp/h` (rendering pipeline)

**Always run** `run_full_pipeline_test.bat` to verify no regression.

### Asset Management Rules

⚠️ **严禁直接 Spawn Actor 到 Level**

所有场景中的对象（Actor、Mesh、Light、Camera 等）**必须**基于 UAsset 资源创建，禁止直接在 Level 中通过 `SpawnActor` 或 Python API 动态生成。

#### 核心原则

1. **UAsset 优先**: 所有可复用对象必须先创建为 `.uasset` 文件（Blueprint、StaticMesh、Material 等）
2. **引用而非创建**: 场景中只放置对 UAsset 的引用/实例，不包含原始数据
3. **资源路径管理**: 使用 Content Browser 管理的标准化路径，避免硬编码

#### 正确做法 ✅

```python
# 从 UAsset 创建对象
cube_asset = unreal.EditorAssetLibrary.load_asset("/Game/Meshes/Cube_Mesh")
cube_actor = unreal.EditorLevelLibrary.spawn_actor_from_object(
    cube_asset,
    location=ORIGIN,
    rotation=(0, 0, 0)
)

# 或使用 Scene API（内部封装 UAsset 引用）
s = Scene("demo")
box = s.cube(size=25, color="cyan")  # 内部引用预设 UAsset
```

#### 错误做法 ❌

```python
# 禁止直接动态创建 Actor
actor = unreal.GameplayStatics.spawn_actor(
    world_context,
    actor_class,
    location,
    rotation
)

# 禁止绕过 Asset 系统
mesh_component = actor.add_component("StaticMeshComponent")
mesh_component.set_static_mesh(runtime_mesh)  # 运行时创建的 Mesh
```

#### 例外情况

仅在以下场景允许动态创建：
- 测试脚本中的临时验证对象（测试结束后立即销毁）
- Runtime 逻辑层的数据驱动实例化（需在代码审查时特别说明）

---

## Quick Reference

| Command | Purpose |
|---------|---------|
| `run_full_pipeline_test.bat` | Basic flow validation (smoke test) |
| `run_tests.bat` | Complete test suite (16 tests) |
| `Scripts/run_full_pipeline_test.py` | Direct Python execution (for debugging) |

**Test Duration**: ~30-60 seconds (depending on hardware)
**Required**: UE5.7+ Editor installed at `C:\Program Files\Epic Games\UE_5.7\`

---

## File Change Summary (Coordinate System Update)

| File | Status | Description |
|------|--------|-------------|
| `Scripts/uemotion/constants.py` | **NEW** | Core coordinate system constants |
| `Scripts/uemotion/__init__.py` | Updated | Export new symbols |
| `Scripts/uemotion/scene.py` | Updated | Added coordinate properties & standard camera setup |
| `Scripts/uemotion/mobject.py` | Updated | Added shift/to_edge/next_to methods |
| `Scripts/run_full_pipeline_test.py` | **REWRITTEN** | Four-quadrant smoke test (DL→UR) |
| `AGENTS.md` | Updated | This documentation |

---

## Asset Creation Best Practices (血泪教训)

### 🎯 **铁律：一切皆UAsset，禁止运行时动态生成！**

**日期**: 2026-05-13
**背景**: 修复BP_Axis材质分配bug + BP_BlackFloor材质问题
**痛苦指数**: ⭐⭐⭐⭐⭐ (浪费3小时debug)

#### 💔 **我踩过的坑（真实案例）**

##### **Bug #1: 动态材质导致蓝图显示异常**

**问题描述**:
- BP_Axis_X 和 BP_Axis_Y 都使用了**相同的红色材质M_Axis_X**
- 在UE编辑器中打开蓝图，Material Slot显示为空或错误
- 运行时材质颜色正确，但无法在编辑器预览

**根本原因**:
```cpp
// ❌ 错误代码：使用Contains()模糊匹配
if (BPName.Contains(TEXT("X")))  // "BP_Axis_Y" 也包含 "X"！
    MaterialName = TEXT("M_Axis_X");  // Y轴也用了X轴材质

// ❌ 错误代码：运行时创建动态材质
UMaterialInstanceDynamic* DynamicMat = UMaterialInstanceDynamic::Create(AxisMaterial, this);
DynamicMat->SetVectorParameterValue(FName("Color"), AxisColor);
MeshComponent->SetMaterial(0, DynamicMat);  // 每次运行都新建，不保存到蓝图！
```

**后果**:
1. 材质是**内存临时对象**，不会序列化到蓝图的SCS
2. Content Browser看不到材质引用
3. 无法在编辑器中调整材质参数
4. 其他开发者打开项目一脸懵逼："材质呢？"

**✅ 正确方案：静态UAsset + 精确匹配**
```cpp
// ✅ 使用EndsWith精确匹配轴名称
if (BPName.EndsWith(TEXT("X")))
    MaterialName = TEXT("M_Axis_X");
else if (BPName.EndsWith(TEXT("Y")))
    MaterialName = TEXT("M_Axis_Y");
else if (BPName.EndsWith(TEXT("Z")))
    MaterialName = TEXT("M_Axis_Z");

// ✅ 先加载/创建UAsset
UMaterialInterface* AxisMaterial = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
if (!AxisMaterial)
{
    // 创建并保存为UAsset文件
    AxisMaterial = CreateStaticAxisMaterial(MaterialName, Color);  // 返回UAsset
}

// ✅ 分配到蓝图的MeshTemplate（不是运行时Component！）
MeshTemplate->SetMaterial(0, AxisMaterial);  // 写入SCS模板，保存到.uasset
```

---

##### **Bug #2: 黑色地板材质不反光需求**

**用户需求**:
> "现在的BP_BlackFloor用的是内存生成的material，我要求自己创建父材质球，用纯黑的不反光的材质"

**我的第一次尝试（❌ 失败）**:
```cpp
// 尝试自定义UMaterial作为父材质
UMaterial* ParentMaterial = NewObject<UMaterial>(...);
ParentMaterial->SetShadingModel(MSM_Unlit);

#if WITH_EDITOR
// ❌ UE 5.7编译错误！Expressions和BaseColor不可访问
ParentMaterial->Expressions.Add(ColorParam);      // error C2039: "Expressions": 不是 "UMaterial" 的成员
ParentMaterial->BaseColor.Expression = ColorParam;  // error C2039: "BaseColor": 不是 "UMaterial" 的成员
#endif
```

**错误原因**:
- UE 5.7对`UMaterial`的API做了限制，不允许直接操作表达式图
- 这是Epic的安全措施，防止底层材质被篡改

**✅ 最终解决方案：使用M_Unlit + MaterialInstanceConstant**
```cpp
// 直接使用UE内置的Unlit材质作为parent（天然不反光）
UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
    nullptr, TEXT("/Engine/EngineMaterials/M_Unlit.M_Unlit"));

UMaterialInstanceConstant* BlackMIC = NewObject<UMaterialInstanceConstant>(...);
BlackMIC->SetParentEditorOnly(BaseMat);

#if WITH_EDITOR
BlackMIC->SetVectorParameterValueEditorOnly(FName("BaseColor"), FLinearColor::Black);
BlackMIC->PostEditChange();  // 关键：触发参数序列化
#endif

// 保存为UAsset
UPackage::SavePackage(Package, BlackMIC, *FilePath, SaveArgs);  // 传入对象引用！
```

**为什么这样更好？**
1. ✅ M_Unlit是引擎内置的，100%稳定
2. ✅ Unlit shading model = 零光照计算 = 天然不反光
3. ✅ MaterialInstanceConstant可以覆盖参数（BaseColor等）
4. ✅ 保存为`.uasset`后可在编辑器自由调整

---

### 🏆 **核心原则（刻在DNA里）**

#### **原则 #1: UAsset优先于一切**

```
优先级排序：
1. 🥇 从Content Browser加载已有UAsset（最快最稳）
2. 🥈 创建新UAsset并保存到磁盘（可复用）
3. 🉐 运行时动态创建（仅用于测试/临时对象）
4. ❌ 禁止：动态对象不保存就使用（必出bug）
```

**为什么必须用UAsset？**

| 特性 | UAsset（静态） | 动态创建 |
|------|---------------|---------|
| 编辑器可见性 | ✅ Content Browser显示 | ❌ 内存幽灵对象 |
| 蓝图引用 | ✅ SCS自动记录 | ❌ 每次运行都丢失 |
| 参数调整 | ✅ 双击即可编辑 | ❌ 必须改代码重新编译 |
| 团队协作 | ✅ 提交Git即可共享 | ❌ 别人电脑上没有 |
| 调试难度 | 🔴 简单 | 🔴🔴🔴 极难（找不到对象） |
| 性能影响 | ✅ 加载一次缓存 | ❌ 每帧/每次都重建 |

**实际案例对比**:
```cpp
// ❌ 动态材质（我之前的错误实现）
void AUEMotionAxisActor::CreateOrLoadAxisMaterial()
{
    if (UMaterialInstanceDynamic* DynamicMat = UMaterialInstanceDynamic::Create(AxisMaterial, this))
    {
        DynamicMat->SetVectorParameterValue(FName("Color"), AxisColor);
        MeshComponent->SetMaterial(0, DynamicMat);  // 运行时才存在
    }
}
// 问题：蓝图里看不到材质，每次运行都重新创建

// ✅ 静态UAsset（修复后的正确实现）
void AUEMotionAxisActor::CreateOrLoadAxisMaterial()
{
    if (MeshComponent->GetMaterial(0))
    {
        AxisMaterial = MeshComponent->GetMaterial(0);  // 直接使用蓝图已分配的
        return;
    }
    
    AxisMaterial = CreateStaticAxisMaterial(...);  // 创建UAsset
    MeshComponent->SetMaterial(0, AxisMaterial);   // 应用静态材质
}
// 优势：蓝图里有材质引用，编辑器可直接查看
```

---

#### **原则 #2: 字符串匹配必须精确**

**血的教训**：
```cpp
// ❌ Contains() - 模糊匹配（坑死我）
"BP_Axis_X".Contains("X")  → true  ✓
"BP_Axis_Y".Contains("X")  → true  ✗ （Y轴被当成X轴！）
"BP_Axis_Z".Contains("X")  → false ✓

// ✅ EndsWith() - 精确匹配（救了我）
"BP_Axis_X".EndsWith("X")  → true  ✓
"BP_Axis_Y".EndsWith("X")  → false ✓
"BP_Axis_Z".EndsWith("X")  → false ✓
```

**通用规则**:
- 用 `EndsWith()` 匹配后缀（如轴名称X/Y/Z）
- 用 `StartsWith()` 匹配前缀（如BP_前缀）
- 用 `==` 或 `Equals()` 做完全匹配
- **永远不要用 `Contains()` 做关键逻辑判断**（除非你真的想模糊搜索）

---

#### **原则 #3: 保存时必须传入对象引用**

**又一个坑**:
```cpp
// ❌ 错误：传nullptr（资产可能损坏或无法加载）
UPackage::SavePackage(Package, nullptr, *FilePath, SaveArgs);

// ✅ 正确：传入具体的对象指针
UPackage::SavePackage(Package, BlackMIC, *FilePath, SaveArgs);  // BlackMIC是要保存的对象
UPackage::SavePackage(Package, NewBP, *BPFilename, SaveArgs);     // NewBP是要保存的蓝图
```

**为什么？**
- UE需要知道你要保存哪个对象来更新资产注册表
- 传nullptr可能导致资产元数据丢失
- 后续`LoadObject()`可能返回nullptr或损坏的对象

---

#### **原则 #4: PostEditChange()是救命稻草**

**什么时候必须调用？**
```cpp
#if WITH_EDITOR
BlackMIC->SetVectorParameterValueEditorOnly(FName("BaseColor"), FLinearColor::Black);
BlackMIC->PostEditChange();  // ✅ 触发完整序列化流程
#endif
```

**不调用的后果**:
- 参数值只在内存中，未写入磁盘
- 下次加载UAsset时参数丢失（变回默认值）
- 编辑器中双击材质看到的值与代码设置的不一致

**适用场景**:
- 设置材质参数后（颜色、纹理、标量等）
- 修改组件属性后（Scale、Collision等）
- 配置SCS节点后

---

### 📋 **标准资产创建流程（5步法）**

以创建坐标轴为例：

```cpp
UMaterialInterface* CreateAxisMaterialUAsset(const FString& Name, const FLinearColor& Color)
{
    // Step 1: 检查是否已存在（避免重复创建）
    FString Path = FString::Printf(TEXT("/Game/UEMotion/Materials/%s"), *Name);
    if (UEditorAssetLibrary::DoesAssetExist(Path))
    {
        UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, *Path);
        if (Existing) return Existing;  // ✅ 复用已有UAsset
        
        // 损坏处理
        UE_LOG(LogTemp, Warning, TEXT("Corrupted asset detected: %s"), *Path);
        UEditorAssetLibrary::DeleteAsset(Path);
    }

    // Step 2: 创建Package和对象
    UPackage* Pkg = CreatePackage(*Path);
    UMaterialInstanceConstant* MIC = NewObject<UMaterialInstanceConstant>(
        Pkg, *Name, RF_Public | RF_Standalone | RF_Transactional
    );

    // Step 3: 配置属性
    MIC->SetParentEditorOnly(BaseMaterial);  // 使用稳定的parent
    
#if WITH_EDITOR
    MIC->SetVectorParameterValueEditorOnly(FName("Color"), Color);
    MIC->PostEditChange();  // ✅ 必须调用！
#endif

    // Step 4: 注册到资产系统
    MIC->SetFlags(RF_Public | RF_Standalone);
    FAssetRegistryModule::AssetCreated(MIC);
    Pkg->MarkPackageDirty();

    // Step 5: 保存到磁盘（传入对象引用！）
    FString FilePath = GetUAssetFilePath(Path);
    FSavePackageArgs Args;
    Args.SaveFlags = RF_Public | RF_Standalone;
    
    bool bSuccess = UPackage::SavePackage(Pkg, MIC, *FilePath, Args);  // ✅ 传MIC不是nullptr
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save: %s"), *Path);
        return nullptr;
    }

    return MIC;  // 返回有效的UAsset指针
}
```

---

### 🎨 **蓝图+材质集成最佳实践**

**目标**: 在蓝图中直接看到材质分配（不是运行时才应用）

```cpp
void CreateBlueprintWithMaterialAssignment()
{
    // 1. 创建蓝图
    UBlueprint* NewBP = CreateBlueprint("/Game/UEMotion/Blueprints/BP_MyActor");

    // 2. 配置SCS
    USimpleConstructionScript* SCS = NewBP->SimpleConstructionScript;
    USCS_Node* MeshNode = SCS->CreateNode(UStaticMeshComponent::StaticClass(), TEXT("Mesh"));
    
    UStaticMeshComponent* MeshTemplate = Cast<UStaticMeshComponent>(MeshNode->ComponentTemplate);
    
    // 3. ✅ 关键：在这里分配材质UAsset（写入SCS模板）
    UMaterialInterface* MyMaterial = LoadOrCreateMaterialUAsset("/Game/UEMotion/Materials/M_MyMaterial");
    MeshTemplate->SetMaterial(0, MyMaterial);  // 这行让材质出现在蓝图的Details面板！

    // 4. 编译并保存
    FKismetEditorUtilities::CompileBlueprint(NewBP);
    SaveBlueprintToDisk(NewBP);

    // 5. Spawn时无需再设置材质（蓝图自带了）
    AActor* Actor = World->SpawnActor<AActor>(NewBP->GeneratedClass, ...);
    // Actor的Mesh组件已经自动应用了MyMaterial
}
```

**效果**:
- 打开BP_MyActor蓝图 → Details面板 → Mesh组件 → Materials[0] = M_MyMaterial ✅
- 双击M_MyMaterial → 可在材质编辑器中调整参数 ✅
- 其他开发者看到蓝图就知道用了什么材质 ✅

---

### ⚠️ **UE版本兼容性备忘录**

| API | UE 5.4及以下 | UE 5.7 | 建议 |
|-----|-------------|--------|------|
| `UMaterial::Expressions` | ✅ 可访问 | ❌ 编译错误 | 改用MaterialInstanceConstant |
| `UMaterial::BaseColor` | ✅ 可访问 | ❌ 编译错误 | 同上 |
| `PostEditChange()` | 可选 | **必须调用** | 统一加上 |
| `SavePackage(obj)` | 可传nullptr | **建议传对象** | 统一传对象 |

**结论**: 如果你的代码要在多个UE版本运行，**始终使用MaterialInstanceConstant + PostEditChange + 对象引用**

---

### ✅ **质量检查清单（每次提交前必查）**

- [ ] 所有材质都是**静态UAsset**（没有UMaterialInstanceDynamic）
- [ ] 蓝图的SCS模板已配置材质（不是运行时SetMaterial）
- [ ] 字符串匹配使用**EndsWith/StartsWith**（不用Contains）
- [ ] 调用了**PostEditChange()**（#if WITH_EDITOR块内）
- [ ] SavePackage传入了**对象指针**（不是nullptr）
- [ ] 资产路径符合规范 `/Game/UEMotion/{Type}/{Name}`
- [ ] 使用了正确的标志 `RF_Public | RF_Standalone | RF_Transactional`
- [ ] 检查了**保存返回值**（失败时不使用无效对象）
- [ ] Content Browser能看到所有生成的UAsset
- [ ] 在UE编辑器中打开蓝图验证Material Slot

---

### 🚨 **常见错误模式（警惕信号）**

如果你看到这些代码，**立即重构**：

```cpp
// 🚨 警告信号 #1: 动态材质
UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(Mat, this);
// → 改为：预先创建MaterialInstanceConstant UAsset

// 🚨 警告信号 #2: Contains做关键匹配
if (Name.Contains("X")) { ... }
// → 改为：Name.EndsWith("X")

// 🚨 警告信号 #3: SavePackage传nullptr
UPackage::SavePackage(Pkg, nullptr, path, args);
// → 改为：UPackage::SavePackage(Pkg, ActualObject, path, args)

// 🚨 警告信号 #4: Spawn后才设置材质
AActor* Actor = SpawnActor(...);
Actor->FindComponentByClass<UStaticMeshComponent>()->SetMaterial(0, Mat);
// → 改为：在蓝图的SCS模板中设置

// 🚨 警告信号 #5: 没有PostEditChange
MIC->SetVectorParameter(Name, Value);
// → 添加：MIC->PostEditChange();
```

---

**最后的话**:

> **"如果它不在磁盘上作为一个.uasset文件存在，那它就不值得信任。"**
> 
> — 一个被动态材质折磨了3小时的AI助手

记住：**UAsset是你的朋友，内存对象是暂时的过客。** 让所有的材质、蓝图、mesh都成为持久的资产，你的未来自己会感谢现在的你！
