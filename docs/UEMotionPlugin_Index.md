# UEMotionPlugin 模块索引文档

## 项目概览

UEMotionPlugin 是一个 Unreal Engine 5 插件，提供**程序化场景构建 + 动画编排 + 离线渲染**的能力。用户可以通过蓝图或 C++ 在编辑器中动态创建几何体、设置关键帧动画、控制摄像机，并利用 Movie Render Queue (MRQ) 输出渲染序列帧。

插件上层还提供了一套 **Python API**（`Scripts/uemotion/` 包），通过 `unreal` Python 桥接层暴露给 UE Python 脚本使用。

---

## C++ 插件模块结构

```
UEMotionPlugin.Build.cs             # 构建配置
Private/
├── UEMotionPlugin.h/.cpp           # 插件入口（模块启动/关闭）
├── Actors/
│   └── UEMotionSceneActor.h/.cpp   # 场景驱动 Actor
├── Core/
│   ├── UEMotionScene.h/.cpp        # 场景核心（总控）
│   ├── UEMotionMobject.h/.cpp      # 场景物体（Mobject）
│   └── UEMotionCamera.h/.cpp       # 摄像机控制
├── Anim/
│   ├── UEMotionAnimation.h/.cpp        # 动画基类 + Easing
│   ├── UEMotionMoveAnimation.h/.cpp    # 位移动画
│   ├── UEMotionRotateAnimation.h/.cpp  # 旋转动画
│   ├── UEMotionScaleAnimation.h/.cpp   # 缩放动画
│   ├── UEMotionFadeAnimation.h/.cpp    # 淡入淡出动画
│   ├── UEMotionColorAnimation.h/.cpp   # 颜色过渡动画
│   ├── UEMotionWaitAnimation.h/.cpp    # 等待（空占位）
│   └── UEMotionGroupAnimation.h/.cpp   # 动画组合（并行/串行）
├── Rendering/
│   └── UEMotionRenderer.h/.cpp    # MRQ 渲染器
└── Utils/
    ├── UEMotionBlueprintLibrary.h/.cpp  # Easing 函数库
    └── UEMotionSequencerCompat.h        # Sequencer 跨版本兼容层
```

## Python API 封装层结构

```
Scripts/
├── run_full_pipeline_test.py        # 入口脚本（调用链追踪目标）
└── uemotion/
    ├── __init__.py                  # 导出 Scene, Mobject, Camera, Animation
    ├── scene.py                     # Scene 类（Python 侧总控）
    ├── mobject.py                   # Mobject 类（物体操作 + 动画创建）
    ├── camera.py                    # Camera 类（摄像机控制）
    ├── animation.py                 # Animation 类（组合动画构建）
    └── colors.py                    # 颜色解析 + 向量/旋转辅助函数
```

---

## 模块详解

### 1. 插件入口 — `UEMotionPlugin`

| 文件 | 说明 |
|------|------|
| `UEMotionPlugin.h` | 声明 `FUEMotionPluginModule`，继承 `IModuleInterface` |
| `UEMotionPlugin.cpp` | 实现 `StartupModule` / `ShutdownModule`（目前为空） |
| `UEMotionPlugin.Build.cs` | 依赖模块配置 |

**依赖项概览：**
- **公有依赖**：`Core`、`CoreUObject`、`Engine`
- **私有依赖**：`Slate/SlateCore`（UI）、`UnrealEd`（编辑器）、`PythonScriptPlugin`（Python 脚本）、`ProceduralMeshComponent`（程序化网格）、`ImageWriteQueue`（图像写入）、`MovieRenderPipeline*`（MRQ 渲染管线）、`CinematicCamera`（电影摄像机）、`LevelSequence/MovieScene*`（Sequencer）、`AssetTools`（资产管理）

---

### 2. 场景驱动 Actor — `UEMotionSceneActor`

| 文件 | 说明 |
|------|------|
| `UEMotionSceneActor.h` | 声明 `AUEMotionSceneActor`，继承 `ACineCameraActor` |
| `UEMotionSceneActor.cpp` | 实现 Tick → 转发给 OwnerScene |

**职责：** 作为场景中的"宿主 Actor"，在 Tick 中将时间推进委托给 `UUEMotionScene`，驱动整个场景的动画更新。继承自 `ACineCameraActor` 但实际摄像机由另一个独立的 `ACineCameraActor` 管理。

---

### 3. 场景核心 — `UEMotionScene`

| 文件 | 说明 |
|------|------|
| `UEMotionScene.h` | 声明 `UUEMotionScene`，**插件的总控类** |
| `UEMotionScene.cpp` | ~814 行完整实现 |

**核心能力：**
- **场景初始化** `Initialize()`：创建 Map 资产 + LevelSequence 资产，生成场景 Actor 和摄像机，添加默认方向光，打开 Sequencer 编辑器
- **几何体创建**：`CreateSphere`、`CreateCube`、`CreateCylinder`、`CreateCone`、`CreatePlane`、`CreateTorus` — 共 6 种基本几何体
- **光照管理**：`AddDirectionalLight`、`AddPointLight`
- **动画编排** `Play()`：接收 `UUEMotionAnimation` 子类，根据类型（Move/Rotate/Scale/Fade/Group）在 LevelSequence 上写入对应帧的 Transform/Float 关键帧。支持 `Wait()` 推进时间轴
- **渲染** `RenderFrames()` / `RenderSingleFrame()`：委托给 `UUEMotionRenderer`
- **资源管理**：`SaveAssets()` / `CleanupAssets()` / `Destroy()` — 支持自动清理资产
- **渲染回调**：`OnRenderFinished` 动态多播委托

**Sequencer 集成方法：**
- `AddActorToSequencer` → 创建 Possessable 绑定
- `GetOrCreateTransformTrack` → 获取/创建 3DTransform 轨道
- `GetOrCreateFloatTrack` → 获取/创建 Float 轨道（用于 Opacity 等属性）
- `RecordTransformKey` / `RecordFloatKey` → 在两个时间点写入关键帧实现补间

---

### 4. 场景物体 — `UEMotionMobject`

| 文件 | 说明 |
|------|------|
| `UEMotionMobject.h` | 声明 `UUEMotionMobject`，场景中物体的抽象 |
| `UEMotionMobject.cpp` | 实现 |

**核心能力：**
- 封装一个 `AActor` + `UStaticMeshComponent` + `UMaterialInstanceDynamic`
- 两种初始化方式：`InitializeAsSphere`（球体使用程序化网格思路加载 Sphere 模型）、`InitializeAsMesh`（通用 StaticMesh 加载）
- 支持属性：**位置/旋转/缩放/颜色/透明度/可见性**
- **动态材质系统**：在 `GetOrCreateBaseMaterial()` 中程序化创建 Translucent 材质，暴露 `BaseColor`（Vector 参数）和 `Opacity`（Scalar 参数）用于动画驱动
- `Destroy()` 清理内部 Actor 和组件引用

---

### 5. 摄像机控制 — `UEMotionCamera`

| 文件 | 说明 |
|------|------|
| `UEMotionCamera.h` | 声明 `UUEMotionCamera` |
| `UEMotionCamera.cpp` | 实现 |

**核心能力：**
- 包装 `ACineCameraActor`，提供更友好的 API
- `SetPosition` / `SetRotation` / `SetFOV` / `GetPosition` / `GetRotation` / `GetFOV`
- `LookAt(Target)` — 使用 `UKismetMathLibrary::FindLookAtRotation` 实现
- `OrbitAround(Center, Radius, Angle, Height)` — 实现绕点旋转，自动 `LookAt` 中心

---

### 6. 动画系统 — `Anim/`

#### 6a. 动画基类 — `UEMotionAnimation`

| 文件 | 说明 |
|------|------|
| `UEMotionAnimation.h` | 抽象基类，定义动画框架 |
| `UEMotionAnimation.cpp` | 将 Easing 委托给 BlueprintLibrary |

**关键设计：**
- `Duration` / `Elapsed` 时间追踪
- `Advance(DeltaTime)` → 计算进度 → ApplyEasing → `TickAnimation(DeltaTime, EasedProgress)`（虚函数，子类实现）
- `IsFinished()` / `GetProgress()` / `Reset()` 生命周期管理
- `EasingType` 支持通过名称（`linear`、`ease_in`、`ease_out_bounce` 等）选择缓动函数

#### 6b. 位移动画 — `UEMotionMoveAnimation`

在两个位置之间线性插值（`FMath::Lerp`）。需要设置目标 Mobject 和终点位置，起点自动从 Mobject 当前位置读取。

#### 6c. 旋转动画 — `UEMotionRotateAnimation`

围绕指定轴旋转指定角度（默认绕 Z 轴 360 度）。使用四元数 `FQuat` 计算增量旋转。

#### 6d. 缩放动画 — `UEMotionScaleAnimation`

在两个缩放值之间线性插值。

#### 6e. 淡入淡出动画 — `UEMotionFadeAnimation`

通过修改 Mobject 的 `Opacity` 属性实现。FadeIn 从 0→1，FadeOut 从 1→0。`Reset()` 时重置 Mobject 到初始透明度。

#### 6f. 颜色过渡动画 — `UEMotionColorAnimation`

在两个颜色之间使用 HSV 插值（`FLinearColor::LerpUsingHSV`）。

#### 6g. 等待动画 — `UEMotionWaitAnimation`

空实现（`TickAnimation` 不做任何事），仅用于时间轴占位。

#### 6h. 组合动画 — `UEMotionGroupAnimation`

支持两种模式：
- **并行模式**（默认）：所有子动画同时播放，总时长 = 最长子动画的时长
- **串行模式**：子动画依次播放，总时长 = 各子动画时长之和

`TickAnimation` 在串行模式下会跟踪每个子动画的已播放时间，按顺序推进。

---

### 7. 渲染器 — `UEMotionRenderer`

| 文件 | 说明 |
|------|------|
| `UEMotionRenderer.h` | 声明 `UUEMotionRenderer` |
| `UEMotionRenderer.cpp` | 实现 MRQ 渲染集成 |

**核心能力：**
- 通过 `UMoviePipelineQueueSubsystem` 创建渲染任务
- 配置：PNG 序列输出、Deferred 渲染管线、自定义分辨率/帧率/输出目录
- 使用 `UMoviePipelinePIEExecutor` 在 PIE 模式下执行渲染
- 渲染完成通过 `OnRenderFinishedDelegate` 广播
- `GetProgress()` 当前返回固定值 0.5（占位）

---

### 8. 工具库 — `Utils/`

#### 8a. Easing 函数库 — `UEMotionBlueprintLibrary`

| 文件 | 说明 |
|------|------|
| `UEMotionBlueprintLibrary.h` | 声明 27 个 Easing 函数 |
| `UEMotionBlueprintLibrary.cpp` | 完整实现 |

**支持的缓动类型（共 27 种）：**
| 类别 | 函数 |
|------|------|
| 线性 | `Linear` |
| 二次 | `InQuad` / `OutQuad` / `InOutQuad` |
| 三次 | `InCubic` / `OutCubic` / `InOutCubic` |
| 四次 | `InQuart` / `OutQuart` / `InOutQuart` |
| 五次 | `InQuint` / `OutQuint` / `InOutQuint` |
| 指数 | `InExpo` / `OutExpo` / `InOutExpo` |
| 圆形 | `InCirc` / `OutCirc` / `InOutCirc` |
| 回退 | `InBack` / `OutBack` / `InOutBack` |
| 弹跳 | `InBounce` / `OutBounce` / `InOutBounce` |
| 弹性 | `InElastic` / `OutElastic` / `InOutElastic` |

`ApplyEasingByName` 提供字符串名称路由，`GetAllEasingNames` 返回所有可用名称列表。

#### 8b. Sequencer 兼容层 — `UEMotionSequencerCompat`

| 文件 | 说明 |
|------|------|
| `UEMotionSequencerCompat.h` | 仅头文件，跨版本兼容命名空间 `UEMotionCompat` |

**解决 UE 5.5 → UE 5.7+ 的 API 差异：**
- `FindObjectBinding` — UE5.7 使用 Lambda 查找 `FMovieScenePossessable`，旧版用 `FindBinding`
- `FindTransformTrack` — UE5.7 使用 `FindTrack<T>`，旧版遍历 `GetAllTracks`
- `AddPossessable` — UE5.7 使用 `AddPossessable(name, class)`，旧版使用 `AddPossessable(FName, class)`
- `CreateAndAddSection` — UE5.7 使用 `CreateNewSection` + `AddSection`，旧版用 `AddSectionToAll`
- `RecordTransformKeys` — UE5.7 通过 `GetChannelProxy` 访问 `FMovieSceneDoubleChannel`，旧版直接访问 `Section->GetChannel(i)`
- `RecordFloatKey` — UE5.7 使用 `AddKeyToChannel`，旧版使用 `SetKeyInChannel`
- `SavePackageToDisk` — UE5.7 使用 `FSavePackageArgs`，旧版直接传 Flags
- `MarkPackageDirty` — UE5.7 使用 `SetDirtyFlag(true)`，旧版使用 `MarkDirty()`
- `DisplayFrameToTick` — 统一的显示帧 → Tick 帧转换

---

## 类继承关系

```
IModuleInterface
  └─ FUEMotionPluginModule                  # 插件模块入口

UObject
  ├─ UUEMotionScene (BlueprintType)         # 场景总控
  ├─ UUEMotionMobject (BlueprintType)       # 场景物体
  ├─ UUEMotionCamera (BlueprintType)        # 摄像机控制
  ├─ UUEMotionAnimation (BlueprintType, Abstract)  # 动画基类
  │   ├─ UUEMotionMoveAnimation             # 位移
  │   ├─ UUEMotionRotateAnimation           # 旋转
  │   ├─ UUEMotionScaleAnimation            # 缩放
  │   ├─ UUEMotionFadeAnimation             # 淡入淡出
  │   ├─ UUEMotionColorAnimation            # 颜色过渡
  │   ├─ UUEMotionWaitAnimation             # 等待
  │   └─ UUEMotionGroupAnimation            # 组合
  ├─ UUEMotionRenderer (BlueprintType)      # 渲染器
  └─ UUEMotionBlueprintLibrary              # 蓝图函数库

ACineCameraActor
  └─ AUEMotionSceneActor                    # 场景驱动 Actor
```

---

## 调用链追踪

以下以 `run_full_pipeline_test.py` 为例，追踪从 Python 入口脚本到 C++ 插件底层每一层的完整调用路径。

> 调用链中的缩进表示**调用深度**，`→` 表示跨层调用（Python→C++ 桥接）。

```
run_full_pipeline_test.py
│
├── from uemotion import Scene
│
├─▶ [1] s = Scene("y_equals_x", 1920, 1080)
│   │
│   └── Scene.__init__　　　　　　　　　　　　　　　　　　（uemotion/scene.py:15）
│       │
│       ├── self._ue = unreal.UEMotionScene()
│       │    → 创建 C++ 对象 UUEMotionScene（Python 桥接）
│       │
│       └── self._ue.initialize(name, width, height)
│            → UUEMotionScene::Initialize()　　　　　　（UEMotionScene.cpp:147）
│                │
│                ├── CreateSceneMap()　　　　　　　　　 （UEMotionScene.cpp:50）
│                │   ├── 删除旧 Map（若存在）
│                │   ├── NewMapFromTemplate(Template_Default)
│                │   └── SaveMap → /Game/UEMotion/Scenes/y_equals_x
│                │
│                ├── CreateLevelSequenceAsset()　　　　（UEMotionScene.cpp:77）
│                │   ├── CreatePackage → /Game/UEMotion/Sequences/LS_y_equals_x
│                │   ├── NewObject<ULevelSequence>
│                │   ├── 设置 DisplayRate=30fps, PlaybackRange
│                │   └── SavePackageToDisk(.uasset)
│                │
│                ├── SpawnActor<AUEMotionSceneActor>　（UEMotionSceneActor 宿主）
│                ├── SpawnActor<ACineCameraActor>　　（独立电影摄像机）
│                ├── Camera = NewObject<UUEMotionCamera>
│                ├── Camera->Init(CineCamera)　　　　　（UEMotionCamera.cpp:7）
│                ├── Camera->LookAt(0,0,0)
│                │
│                ├── AddActorToSequencer(SceneActor)　（UEMotionScene.cpp:378）
│                │   └── UEMotionCompat::FindOrAddPossessable()
│                │       ├── FindObjectBinding()　　　　（UEMotionSequencerCompat.h:33）
│                │       └── AddPossessable() + BindPossessableObject()
│                │
│                ├── AddActorToSequencer(CineCamera)
│                ├── 创建 CameraCutTrack + 绑定 CineCamera
│                │
│                ├── SetupDefaultLighting()　　　　　　（UEMotionScene.cpp:116）
│                │   └── SpawnActor<ADirectionalLight>
│                │
│                ├── SaveMap
│                └── OpenLevelSequenceInEditor()
│
├─▶ [2] s.directional_light(direction=(0,-1,-1), color="white", intensity=10)
│   │
│   └── Scene.directional_light()　　　　　　　　　　　（uemotion/scene.py:127）
│       │
│       └── self._ue.add_directional_light(vec, color, intensity)
│            → UUEMotionScene::AddDirectionalLight()　（UEMotionScene.cpp:339）
│                └── SpawnActor<ADirectionalLight> + 设置强度和颜色
│
├─▶ [3] s.camera.position = (-300, -500, 400)
│   │
│   └── Camera.position.setter　　　　　　　　　　　　　（uemotion/camera.py:14）
│       │
│       └── self._ue.set_position(x, y, z)
│            → UUEMotionCamera::SetPosition()　　　　　（UEMotionCamera.cpp:11）
│                └── CameraActor->SetActorLocation(FVector)
│
├─▶ [4] s.camera.look_at((50, 50, 50))
│   │
│   └── Camera.look_at()　　　　　　　　　　　　　　　　（uemotion/camera.py:29）
│       │
│       └── self._ue.look_at(target)
│            → UUEMotionCamera::LookAt()　　　　　　　（UEMotionCamera.cpp:56）
│                └── UKismetMathLibrary::FindLookAtRotation + SetActorRotation
│
├─▶ [5] box = s.cube(30, color="cyan", location=(0, 0, 50))
│   │
│   └── Scene.cube()　　　　　　　　　　　　　　　　　　（uemotion/scene.py:72）
│       │
│       ├── self._ue.create_cube(30)
│       │   → UUEMotionScene::CreateCube()　　　　　　（UEMotionScene.cpp:250）
│       │       │
│       │       ├── NewObject<UUEMotionMobject>
│       │       ├── Obj->InitializeAsMesh(SceneActor, "/Engine/BasicShapes/Cube.Cube", 30/50)
│       │       │   → UUEMotionMobject::CreateStaticMeshActor()　（UEMotionMobject.cpp:79）
│       │       │       │
│       │       │       ├── World->SpawnActor<AActor>　（Mobject_xxx）
│       │       │       ├── NewObject<UStaticMeshComponent> → 注册 + 设为 Root
│       │       │       ├── LoadObject<UStaticMesh>("/Engine/BasicShapes/Cube.Cube")
│       │       │       └── GetOrCreateBaseMaterial()　（UEMotionMobject.cpp:13）
│       │       │           ├── NewObject<UMaterial>　（M_UEMotionBase）
│       │       │           ├── BlendMode = Translucent
│       │       │           ├── UMaterialExpressionVectorParameter("BaseColor")
│       │       │           ├── UMaterialExpressionScalarParameter("Opacity")
│       │       │           ├── UMaterialExpressionConstant(Roughness=0.5)
│       │       │           └── UMaterialExpressionConstant(Metallic=0.0)
│       │       │
│       │       ├── MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial)
│       │       ├── MeshComp->SetMaterial(0, MaterialInstance)
│       │       └── Mobjects.Add(Obj)
│       │
│       ├── Mobject(scene, ue_mobject)　　　　　　　　（uemotion/mobject.py:5）
│       └── m.color = "cyan"
│           └── self._ue.set_color(LinearColor(0.1, 0.9, 0.9, 1))
│                → UUEMotionMobject::SetColor()　　　　（UEMotionMobject.cpp:139）
│
├─▶ [6--10] for i=1..5:
│   │
│   ├── box.move_to((i*10, i*10, 50), duration=1.0, easing="linear")
│   │   → Mobject.move_to()　　　　　　　　　　　　　　（uemotion/mobject.py:61）
│   │       │
│   │       ├── anim = unreal.UEMotionMoveAnimation()
│   │       │    → 创建 C++ 对象 UUEMotionMoveAnimation
│   │       ├── anim.set_target_mobject(self._ue)
│   │       │   → UUEMotionMoveAnimation::SetTargetMobject() （UEMotionMoveAnimation.cpp:4）
│   │       │       └── 记录 Start = Mobject->GetLocation()
│   │       ├── anim.set_target((x, y, z))
│   │       │   → UUEMotionMoveAnimation::SetTarget()
│   │       ├── anim.set_duration(1.0)
│   │       ├── anim.set_easing("linear")
│   │       └── self._scene._pending_animations.append(anim)
│   │
│   └── s.play()
│       → Scene.play()　　　　　　　　　　　　　　　　　（uemotion/scene.py:133）
│           │
│           └── self._ue.play(anim)
│                → UUEMotionScene::Play()　　　　　　　（UEMotionScene.cpp:503）
│                    │
│                    ├── Cast<UUEMotionMoveAnimation>(anim)
│                    ├── GetOrCreateTransformTrack(MoveTarget)
│                    │   → UUEMotionScene::GetOrCreateTransformTrack() (UEMotionScene.cpp:391)
│                    │       ├── UEMotionCompat::FindOrAddPossessable()
│                    │       ├── UEMotionCompat::FindTransformTrack()
│                    │       └── MovieScene->AddTrack<UMovieScene3DTransformTrack>
│                    │
│                    ├── RecordTransformKey(StartFrame, StartLoc, Rot, Scale)
│                    ├── RecordTransformKey(EndFrame, EndLoc, Rot, Scale)
│                    │   → UUEMotionScene::RecordTransformKey() (UEMotionScene.cpp:459)
│                    │       └── UEMotionCompat::RecordTransformKeys()
│                    │           → Section->GetChannelProxy() / Section->GetChannel(i)
│                    │           → AddKeyToChannel() / SetKeyInChannel()
│                    │
│                    ├── ActiveAnimations.Add(Animation)
│                    ├── CurrentTime = EndTime
│                    └── MovieScene->SetPlaybackRange(0, TotalFrames)
│
├─▶ [11] s.on_render_finished(on_render_done)
│   │
│   └── Scene.on_render_finished(callback)　　　　　　（uemotion/scene.py:174）
│       │
│       └── self._render_callbacks.append(callback)
│           （预先注册：__init__ 时 _bind_render_delegate 已绑定
│            → self._ue.on_render_finished_delegate...add_callable_unique(self._on_render_finished)
│            → self._on_render_finished() 遍历 self._render_callbacks 逐一调用）
│
└─▶ [12] s._ue.render_frames(frames_dir, total_duration, FPS)
    │
    → UUEMotionScene::RenderFrames()　　　　　　　　（UEMotionScene.cpp:660）
        │
        ├── SaveAssets()
        │   → UUEMotionScene::SaveAssets()　　　　　（UEMotionScene.cpp:744）
        │       ├── LevelSequence->MarkPackageDirty()
        │       ├── UEMotionCompat::SavePackageToDisk(.uasset)
        │       └── UEditorLoadingAndSavingUtils::SaveMap()
        │
        └── Renderer->RenderSequence(LevelSequence, OutputDir, Duration, FPS)
            → UUEMotionRenderer::RenderSequence()　（UEMotionRenderer.cpp:28）
                │
                ├── GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>()
                ├── PipelineQueue->DeleteAllJobs()
                ├── AllocateNewJob(UMoviePipelineExecutorJob)
                ├── Job->Sequence = LevelSequence
                ├── Job->Map = TargetWorld
                ├── 配置 UMoviePipelinePrimaryConfig
                │   ├── UMoviePipelineOutputSetting
                │   │   ├── OutputDirectory = frames_dir
                │   │   ├── FileNameFormat = "{sequence_name}.{frame_number}"
                │   │   ├── OutputResolution = 1920×1080
                │   │   ├── OutputFrameRate = 30fps
                │   │   └── ZeroPadFrameNumbers = 4
                │   ├── UMoviePipelineDeferredPassBase （渲染管线）
                │   └── UMoviePipelineImageSequenceOutput_PNG （输出格式）
                │
                └── QueueSubsystem->RenderQueueWithExecutorInstance(Executor)
                    → PIEExecutor 启动 Play-In-Editor 渲染
                    │
                    └── 渲染完成后的回调链：
                        │
                        UMoviePipelineExecutorBase::OnExecutorFinished()
                        └── UUEMotionRenderer::OnRenderFinished()　（UEMotionRenderer.cpp:118）
                            │
                            └── OnRenderFinishedDelegate.Broadcast(bSuccess)
                                │
                                → UUEMotionScene::OnRendererFinished()　（UEMotionScene.cpp:806）
                                    │
                                    ├── Renderer->OnRenderFinishedDelegate.RemoveDynamic
                                    └── OnRenderFinished.Broadcast(this, bSuccess)
                                        │
                                        → Python: Scene._on_render_finished(scene, success)
                                            │
                                            └── for cb in self._render_callbacks:
                                                │
                                                └── run_full_pipeline_test.py::on_render_done(success)
                                                    │
                                                    ├── 检查输出目录 PNG 文件
                                                    ├── 打印测试结果 (PASSED/FAILED)
                                                    └── s.destroy()
                                                        → UUEMotionScene::Destroy()　（UEMotionScene.cpp:711）
                                                            ├── ClearScene() → 销毁所有 Mobject
                                                            ├── 销毁 SceneActor / CineCamera
                                                            ├── 清理 Renderer 引用
                                                            └── [auto_cleanup] CleanupAssets()
                                                                → 删除 Map + Sequence 资产
```

---

## 数据流 / 典型使用流程

```
1. 创建场景
   UUEMotionScene::Initialize("MyScene")
     ├─ CreateSceneMap()         → 创建 /Game/UEMotion/Scenes/MyScene 地图
     ├─ CreateLevelSequenceAsset() → 创建 /Game/UEMotion/Sequences/LS_MyScene
     ├─ Spawn SceneActor & CineCamera
     ├─ 注册 Sequencer 绑定 + 摄像机裁剪轨道
     ├─ 默认方向光
     └─ 打开 Sequencer 编辑器

2. 创建物体
   UUEMotionScene::CreateSphere() / CreateCube() / ...
     └─ UUEMotionMobject::InitializeAsSphere/InitializeAsMesh
         ├─ Spawn AActor + UStaticMeshComponent
         ├─ 加载 StaticMesh
         └─ 创建动态材质实例 (BaseColor + Opacity)

3. 编排动画
   UUEMotionScene::Play(MoveAnimation)
     ├─ 根据动画类型获取/创建 Track
     ├─ 在 StartFrame / EndFrame 写入 TransformKey/FloatKey
     └─ 更新 LevelSequence 播放范围

4. 渲染输出
   UUEMotionScene::RenderFrames(OutputDir, Duration, FPS)
     └─ UUEMotionRenderer::RenderSequence()
         ├─ 配置 MRQ Pipeline (PNG输出, DeferredPass)
         └─ PIEExecutor 执行渲染
             └─ OnRenderFinished → 广播 OnRenderFinished 委托
```

---

> 生成日期：2026-05-08  
> 代码行数统计：约 2,000+ 行 C++（含头文件与实现）+ ~300 行 Python 封装  
> UE 版本兼容：UE 5.5 ~ UE 5.7+（通过 `UEMotionSequencerCompat.h` 适配）
