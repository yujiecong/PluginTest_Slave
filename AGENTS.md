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
