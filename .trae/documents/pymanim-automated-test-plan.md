# PyManim 自动化测试方案

## 目标

设计一批 Python 测试脚本，通过命令行启动 UE5 编辑器后自动运行，覆盖插件所有 103 个 BlueprintCallable/BlueprintPure API，产出结构化测试报告。

***

## 一、命令行运行方式

UE5 内置 PythonScriptPlugin 支持命令行执行脚本，两种方式：

### 方式 1：编辑器模式（推荐，支持完整 Editor 功能）

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" `
    "c:\Users\42458\Documents\Unreal Projects\PluginTest\PluginTest.uproject" `
    -ExecutePythonScript="c:\Users\42458\Documents\Unreal Projects\PluginTest\Scripts\tests\run_all_tests.py"
```

### 方式 2：Commandlet 模式（无窗口，纯命令行）

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "c:\Users\42458\Documents\Unreal Projects\PluginTest\PluginTest.uproject" `
    -run=python -script="exec(open(r'c:\Users\42458\Documents\Unreal Projects\PluginTest\Scripts\tests\run_all_tests.py').read())"
```

### 方式 3：一键 bat 脚本

创建 `run_tests.bat`，自动清理 + 启动 UE + 运行测试 + 输出报告。

***

## 二、测试架构设计

```
Scripts/tests/
├── run_all_tests.py          # 总入口：运行所有测试，汇总报告
├── test_framework.py         # 测试框架：断言、计数、报告输出
├── test_01_scene.py          # 场景生命周期测试
├── test_02_mobject.py        # Mobject 属性 CRUD 测试
├── test_03_camera.py         # 相机控制测试
├── test_04_animation_move.py # 移动动画测试
├── test_05_animation_rotate.py # 旋转动画测试
├── test_06_animation_scale.py  # 缩放动画测试
├── test_07_animation_fade.py   # 淡入淡出动画测试
├── test_08_animation_color.py  # 颜色动画测试
├── test_09_animation_wait.py   # 等待动画测试
├── test_10_animation_group.py  # 组合动画测试
├── test_11_easing.py          # 缓动函数库测试
├── test_12_light.py           # 灯光系统测试
├── test_13_renderer.py        # 渲染器测试
├── test_14_python_wrapper.py  # Python 封装层测试
├── test_15_boundary.py        # 边界/防御性测试
└── test_report/               # 测试报告输出目录
```

***

## 三、测试框架 (test\_framework.py)

核心设计：

* `TestCase` 基类，提供 `assert_eq`/`assert_ne`/`assert_true`/`assert_false`/`assert_near`/`assert_none`/`assert_not_none`/`assert_no_crash` 等断言

* 自动统计 PASS/FAIL/SKIP 计数

* 每个测试函数自动 try/except，异常不会中断整体流程

* 最终输出结构化报告（JSON + 控制台），包含每个测试的名称、状态、耗时、错误信息

* 支持 `@skip` 标记跳过测试

* 支持 `@timeout(seconds)` 标记超时

***

## 四、各测试模块详细设计

### test\_01\_scene.py — 场景生命周期（覆盖 20 个 Scene API）

| #  | 测试名                                  | 测试内容                                        | 覆盖 API                                                      |
| -- | ------------------------------------ | ------------------------------------------- | ----------------------------------------------------------- |
| 1  | test\_scene\_create                  | 创建 PyManimScene 对象                          | 构造                                                          |
| 2  | test\_scene\_initialize              | Initialize(1920,1080)，验证 IsInitialized=True | Initialize, IsInitialized                                   |
| 3  | test\_scene\_initialize\_custom\_res | Initialize(3840,2160) 自定义分辨率                | Initialize                                                  |
| 4  | test\_scene\_double\_initialize      | 重复 Initialize 不崩溃                           | Initialize                                                  |
| 5  | test\_scene\_get\_camera             | GetCamera 返回非 None                          | GetCamera                                                   |
| 6  | test\_scene\_create\_all\_shapes     | 创建全部6种形状，验证 GetAllMobjects 数量=6             | CreateSphere/Cube/Cylinder/Cone/Plane/Torus, GetAllMobjects |
| 7  | test\_scene\_play\_tick              | Play + Tick 推进动画                            | Play, Tick, HasActiveAnimations                             |
| 8  | test\_scene\_stop\_all               | Play 后 StopAll，验证 HasActiveAnimations=False | Play, StopAll, HasActiveAnimations                          |
| 9  | test\_scene\_clear                   | ClearScene 后 GetAllMobjects 为空              | ClearScene, GetAllMobjects                                  |
| 10 | test\_scene\_destroy                 | Destroy 后 IsInitialized=False               | Destroy, IsInitialized                                      |
| 11 | test\_scene\_add\_directional\_light | AddDirectionalLight 不崩溃                     | AddDirectionalLight                                         |
| 12 | test\_scene\_add\_point\_light       | AddPointLight 不崩溃                           | AddPointLight                                               |
| 13 | test\_scene\_render\_frames          | RenderFrames 到临时目录，验证目录创建                   | RenderFrames                                                |
| 14 | test\_scene\_render\_single\_frame   | RenderSingleFrame 不崩溃                       | RenderSingleFrame                                           |

### test\_02\_mobject.py — Mobject 属性 CRUD（覆盖 12 个 Mobject API）

| # | 测试名                                 | 测试内容                                      | 覆盖 API                       |
| - | ----------------------------------- | ----------------------------------------- | ---------------------------- |
| 1 | test\_mobject\_set\_get\_location   | SetLocation + GetLocation 往返一致            | SetLocation, GetLocation     |
| 2 | test\_mobject\_set\_get\_color      | SetColor + GetColor 往返一致                  | SetColor, GetColor           |
| 3 | test\_mobject\_set\_get\_visibility | SetVisibility(True/False) + GetVisibility | SetVisibility, GetVisibility |
| 4 | test\_mobject\_set\_get\_scale      | SetScale + GetScale 往返一致                  | SetScale, GetScale           |
| 5 | test\_mobject\_set\_get\_rotation   | SetRotation + GetRotation 往返一致            | SetRotation, GetRotation     |
| 6 | test\_mobject\_get\_name            | GetName 返回正确名称                            | GetName                      |
| 7 | test\_mobject\_destroy              | Destroy 后操作不崩溃                            | Destroy                      |
| 8 | test\_mobject\_all\_shapes          | 6种形状都能创建且属性可操作                            | 全部 Create 方法                 |

### test\_03\_camera.py — 相机控制（覆盖 8 个 Camera API）

| # | 测试名                              | 测试内容                           | 覆盖 API                   |
| - | -------------------------------- | ------------------------------ | ------------------------ |
| 1 | test\_camera\_set\_get\_position | SetPosition + GetPosition 往返一致 | SetPosition, GetPosition |
| 2 | test\_camera\_set\_get\_rotation | SetRotation + GetRotation 往返一致 | SetRotation, GetRotation |
| 3 | test\_camera\_set\_get\_fov      | SetFOV + GetFOV 往返一致           | SetFOV, GetFOV           |
| 4 | test\_camera\_look\_at           | LookAt 后朝向目标                   | LookAt                   |
| 5 | test\_camera\_orbit              | OrbitAround 后位置正确              | OrbitAround              |
| 6 | test\_camera\_default\_fov       | 默认 FOV=90                      | GetFOV                   |

### test\_04\~10 — 动画系统（覆盖 7 种动画 + 基类 7 API + Group 3 API = 59 API）

每个动画类型的测试模式：

1. **创建与配置**：创建动画对象，设置参数
2. **基类 API**：SetDuration/GetDuration, SetEasing/GetEasing, IsFinished, GetProgress, Reset
3. **Tick 推进**：手动 Tick 推进动画，验证中间状态
4. **完成状态**：Tick 超过 Duration 后 IsFinished=True
5. **数值验证**：验证动画结束值与目标值一致

| 模块               | 测试重点                                | 额外验证                                                |
| ---------------- | ----------------------------------- | --------------------------------------------------- |
| test\_04\_move   | MoveAnimation: Start→End 插值         | 位置线性插值数值验证                                          |
| test\_05\_rotate | RotateAnimation: 角度+轴向              | 旋转角度正确                                              |
| test\_06\_scale  | ScaleAnimation: StartScale→EndScale | 缩放数值验证                                              |
| test\_07\_fade   | FadeAnimation: FadeIn/FadeOut       | 可见性切换                                               |
| test\_08\_color  | ColorAnimation: HSV 插值              | 起止颜色正确                                              |
| test\_09\_wait   | WaitAnimation: 消耗时间                 | Tick 不改变 Mobject 状态                                 |
| test\_10\_group  | GroupAnimation: 并行/顺序               | AddAnimation, SetPlayMode, GetAnimationCount, 时序正确性 |

### test\_11\_easing.py — 缓动函数库（覆盖 30 个 BlueprintPure API）

| #  | 测试名                              | 测试内容                                        |
| -- | -------------------------------- | ------------------------------------------- |
| 1  | test\_easing\_endpoints          | 全部 28 种缓动函数 t=0→0, t=1→1                    |
| 2  | test\_easing\_monotonic          | 标准缓动函数单调递增                                  |
| 3  | test\_easing\_range              | 所有缓动函数值在合理范围 \[-0.5, 1.5]                   |
| 4  | test\_easing\_known\_values      | 关键缓动函数已知值验证（如 ease\_in\_quad(0.5)=0.25）     |
| 5  | test\_easing\_clamp              | t<0 和 t>1 时 clamp 到 \[0,1]                  |
| 6  | test\_apply\_easing\_by\_name    | 按名称查找返回正确值                                  |
| 7  | test\_get\_all\_easing\_names    | 返回列表长度 >= 28                                |
| 8  | test\_easing\_back\_overshoot    | ease\_in\_back/ease\_out\_back 超出 \[0,1] 范围 |
| 9  | test\_easing\_elastic\_overshoot | ease\_out\_elastic 超出 \[0,1] 范围             |
| 10 | test\_easing\_bounce\_bounces    | ease\_out\_bounce 有反弹段                      |

### test\_12\_light.py — 灯光系统

| # | 测试名                                | 测试内容      |
| - | ---------------------------------- | --------- |
| 1 | test\_directional\_light\_create   | 创建方向光不崩溃  |
| 2 | test\_point\_light\_create         | 创建点光源不崩溃  |
| 3 | test\_multiple\_lights             | 创建多个灯光不崩溃 |
| 4 | test\_light\_with\_zero\_intensity | 零强度不崩溃    |

### test\_13\_renderer.py — 渲染器（覆盖 8 个 Renderer API）

| # | 测试名                                   | 测试内容                  | 覆盖 API            |
| - | ------------------------------------- | --------------------- | ----------------- |
| 1 | test\_renderer\_initialize            | Initialize 不崩溃        | Initialize        |
| 2 | test\_renderer\_bind\_camera          | BindCamera 不崩溃        | BindCamera        |
| 3 | test\_renderer\_set\_aa               | SetAntiAliasing 不崩溃   | SetAntiAliasing   |
| 4 | test\_renderer\_set\_format           | SetOutputFormat 不崩溃   | SetOutputFormat   |
| 5 | test\_renderer\_is\_rendering         | IsRendering 初始=False  | IsRendering       |
| 6 | test\_renderer\_get\_progress         | GetProgress 返回 float  | GetProgress       |
| 7 | test\_renderer\_render\_sequence      | RenderSequence 到临时目录  | RenderSequence    |
| 8 | test\_renderer\_render\_single\_frame | RenderSingleFrame 不崩溃 | RenderSingleFrame |

### test\_14\_python\_wrapper.py — Python 封装层

| #  | 测试名                              | 测试内容                         |
| -- | -------------------------------- | ---------------------------- |
| 1  | test\_wrapper\_scene\_create     | Scene() 创建                   |
| 2  | test\_wrapper\_sphere            | s.sphere() 创建 + 属性设置         |
| 3  | test\_wrapper\_cube              | s.cube() 创建                  |
| 4  | test\_wrapper\_all\_shapes       | 全部6种形状                       |
| 5  | test\_wrapper\_move\_to          | m.move\_to() + s.play()      |
| 6  | test\_wrapper\_rotate            | m.rotate() + s.play()        |
| 7  | test\_wrapper\_scale\_to         | m.scale\_to() + s.play()     |
| 8  | test\_wrapper\_fade\_in          | m.fade\_in() + s.play()      |
| 9  | test\_wrapper\_fade\_out         | m.fade\_out() + s.play()     |
| 10 | test\_wrapper\_change\_color     | m.change\_color() + s.play() |
| 11 | test\_wrapper\_camera            | camera.position/fov/look\_at |
| 12 | test\_wrapper\_wait              | s.wait()                     |
| 13 | test\_wrapper\_color\_resolution | resolve\_color 各种格式          |
| 14 | test\_wrapper\_vec               | vec() 工具函数                   |
| 15 | test\_wrapper\_chain             | 链式调用 move\_to().rotate()     |

### test\_15\_boundary.py — 边界/防御性测试

| #  | 测试名                             | 测试内容                          |
| -- | ------------------------------- | ----------------------------- |
| 1  | test\_negative\_radius          | CreateSphere(-1) 不崩溃，返回非 None |
| 2  | test\_negative\_size            | CreateCube(-5) 不崩溃            |
| 3  | test\_negative\_cylinder        | CreateCylinder(-1,-1) 不崩溃     |
| 4  | test\_uninitialized\_scene      | 未 Initialize 就调用各种方法不崩溃       |
| 5  | test\_zero\_duration\_animation | Duration=0 的动画不崩溃             |
| 6  | test\_empty\_path\_render       | RenderFrames("") 不崩溃          |
| 7  | test\_many\_mobjects\_100       | 创建 100 个 Mobject 不崩溃          |
| 8  | test\_long\_animation\_60s      | 60 秒动画稳定完成                    |
| 9  | test\_chinese\_path             | 中文路径渲染不崩溃                     |
| 10 | test\_destroy\_twice            | 重复 Destroy 不崩溃                |
| 11 | test\_tick\_zero\_delta         | Tick(0) 不崩溃                   |
| 12 | test\_tick\_negative\_delta     | Tick(-1) 不崩溃                  |
| 13 | test\_play\_none                | Play(None) 不崩溃                |

***

## 五、测试报告格式

### 控制台输出

```
============================================================
  PyManim Automated Test Report
  Date: 2026-05-07 14:30:00
  UE Version: 5.7
============================================================

[01/15] Scene Lifecycle .............. 14/14 PASS (0.8s)
[02/15] Mobject Properties ..........  8/8 PASS (0.5s)
[03/15] Camera Control .............. 6/6 PASS (0.3s)
[04/15] Move Animation .............. 5/5 PASS (0.4s)
[05/15] Rotate Animation ............ 5/5 PASS (0.4s)
[06/15] Scale Animation ............. 5/5 PASS (0.3s)
[07/15] Fade Animation .............. 5/5 PASS (0.3s)
[08/15] Color Animation ............. 5/5 PASS (0.3s)
[09/15] Wait Animation .............. 3/3 PASS (0.2s)
[10/15] Group Animation ............. 7/7 PASS (0.6s)
[11/15] Easing Functions ............ 10/10 PASS (0.1s)
[12/15] Light System ................ 4/4 PASS (0.2s)
[13/15] Renderer .................... 8/8 PASS (2.1s)
[14/15] Python Wrapper .............. 15/15 PASS (1.2s)
[15/15] Boundary Tests .............. 13/13 PASS (3.5s)

============================================================
  TOTAL: 103/103 PASS  0 FAIL  0 SKIP
  Time: 11.1s
  Result: ALL TESTS PASSED ✓
============================================================
```

### JSON 报告 (test\_report/report.json)

```json
{
  "timestamp": "2026-05-07T14:30:00",
  "ue_version": "5.7",
  "total": 103,
  "passed": 103,
  "failed": 0,
  "skipped": 0,
  "duration_s": 11.1,
  "suites": [
    {
      "name": "Scene Lifecycle",
      "passed": 14, "failed": 0, "skipped": 0,
      "duration_s": 0.8,
      "tests": [
        {"name": "test_scene_create", "status": "pass", "duration_s": 0.01},
        ...
      ]
    },
    ...
  ]
}
```

***

## 六、实现步骤

### Step 1：创建测试框架 test\_framework.py

* TestCase 基类 + 断言方法

* TestRunner 管理器 + 模块发现

* JSON 报告输出

* 控制台格式化输出

### Step 2：创建 run\_all\_tests.py 总入口

* 自动发现 Scripts/tests/test\_\*.py

* 按编号顺序执行

* 汇总报告

* 支持命令行参数（--filter, --output, --verbose）

### Step 3：实现 test\_01\_scene.py \~ test\_10\_animation\_group.py

* 核心功能测试，覆盖所有 C++ API

### Step 4：实现 test\_11\_easing.py \~ test\_15\_boundary.py

* 缓动函数 + 封装层 + 边界测试

### Step 5：创建 run\_tests.bat 一键脚本

* 自动编译插件（可选）

* 启动 UE 运行测试

* 解析退出码和报告

### Step 6：验证

* 在 UE 编辑器内手动运行 run\_all\_tests.py 确认全部通过

* 通过命令行运行确认自动化流程可行

***

## 七、API 覆盖率追踪

| 类                        | API 总数  | 测试模块               | 预计覆盖           |
| ------------------------ | ------- | ------------------ | -------------- |
| UPyManimBlueprintLibrary | 30      | test\_11\_easing   | 30             |
| UPyManimScene            | 20      | test\_01\_scene    | 20             |
| UPyManimRenderer         | 8       | test\_13\_renderer | 8              |
| UPyManimAnimation (基类)   | 7       | test\_04\~10       | 7              |
| UPyManimGroupAnimation   | 3       | test\_10\_group    | 3              |
| UPyManimColorAnimation   | 3       | test\_08\_color    | 3              |
| UPyManimFadeAnimation    | 2       | test\_07\_fade     | 2              |
| UPyManimScaleAnimation   | 3       | test\_06\_scale    | 3              |
| UPyManimRotateAnimation  | 3       | test\_05\_rotate   | 3              |
| UPyManimMoveAnimation    | 3       | test\_04\_move     | 3              |
| UPyManimWaitAnimation    | 0 (继承7) | test\_09\_wait     | 7              |
| UPyManimMobject          | 12      | test\_02\_mobject  | 12             |
| UPyManimCamera           | 8       | test\_03\_camera   | 8              |
| APyManimSceneActor       | 1       | test\_01\_scene    | 1              |
| **合计**                   | **103** | <br />             | **103 (100%)** |

