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
        ├── scene.py                    # Scene 类
        ├── camera.py                   # Camera 类
        ├── mobject.py                  # Mobject 类
        └── animation.py                # Animation 类
```

---

## Basic Flow Test (Smoke Test)

**Entry Point**: `run_full_pipeline_test.bat`

### Purpose

这是 **最基本的回归测试**，用于快速验证整个 UEMotion Pipeline 是否正常工作。每次代码变更后都应运行此测试，确保核心功能未被破坏。

### What It Tests

1. **Scene Creation**: 创建 3D 场景、灯光设置
2. **Camera Setup**: 2D 俯视相机（top-down view at Z=800）
3. **Object Animation**: Cube 沿 y=x 路径均匀移动（5 步，每步 10 单位）
4. **Keyframe Recording**: Sequencer 关键帧记录与通道映射（Roll/Pitch/Yaw for UE5.7+）
5. **MoviePipeline Rendering**: 渲染 151 帧 PNG 序列（1920x1080 @ 30fps）
6. **Output Validation**: 文件数量、大小、格式验证
7. **UE Shutdown**: 测试完成后自动关闭 UE Editor

### Test Parameters

```python
Scene Name     : "y_equals_x"
Resolution     : 1920x1080
Animation      : 5 steps × 1.0s = 5.0s total
FPS            : 30
Expected Frames: 151 (5.0s × 30 + 1)
Path           : y=x from (0,0) to (50,50)
Camera         : top-down at Z=800
Lighting       : Directional + Point Light
Cube           : Size=30, Color=Cyan, Z=0
```

### Execution

```bash
# Windows
cd c:\Users\42458\Documents\Unreal Projects\PluginTest
run_full_pipeline_test.bat

# Expected Output:
# [1/4] Creating scene...
# [2/4] Creating cube and animating along y=x...
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

- ✅ Scene object created successfully
- ✅ Camera position at Z=800 (top-down)
- ✅ Cube final position = (50, 50, 0)
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

---

## Quick Reference

| Command | Purpose |
|---------|---------|
| `run_full_pipeline_test.bat` | Basic flow validation (smoke test) |
| `run_tests.bat` | Complete test suite (16 tests) |
| `Scripts/run_full_pipeline_test.py` | Direct Python execution (for debugging) |

**Test Duration**: ~30-60 seconds (depending on hardware)
**Required**: UE5.7+ Editor installed at `C:\Program Files\Epic Games\UE_5.7\`
