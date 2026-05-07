# 02 - 核心 UObject 类层次设计

> **文档版本**: v2.0 (2026-05-07 更新)
> **基于代码**: 实际 C++ 头文件同步
> **注意**: C++ 类名使用 `UUEMotion*` 前缀，Python 端通过 `unreal` 模块调用时自动转换为 `UEMotion*`

## 1. 类层次总览（已实现）

```
UObject
├── UUEMotionScene              # ✅ 场景管理器
├── UUEMotionCamera             # ✅ 相机控制
├── UUEMotionMobject            # ✅ 可视对象基类
├── UUEMotionAnimation          # ✅ 动画基类 (Abstract)
│   ├── UUEMotionMoveAnimation     # ✅ 位移动画
│   ├── UUEMotionRotateAnimation   # ✅ 旋转动画
│   ├── UUEMotionScaleAnimation    # ✅ 缩放动画
│   ├── UUEMotionFadeAnimation     # ✅ 淡入淡出动画
│   ├── UUEMotionColorAnimation    # ✅ 颜色渐变动画
│   ├── UUEMotionWaitAnimation     # ✅ 等待动画
│   └── UUEMotionGroupAnimation    # ✅ 组合动画
└── UUEMotionRenderer           # ✅ 渲染器 (MRP)

AActor
└── AUEMotionSceneActor         # ✅ 场景在世界的物理载体

UBlueprintFunctionLibrary
└── UUEMotionBlueprintLibrary   # ✅ 静态辅助函数
```

所有 `UObject` 子类均标记 `UCLASS(BlueprintType)`，确保暴露到 Python。

## 2. UUEMotionScene — 场景管理器 ✅

### 职责
- 管理整个渲染世界的生命周期
- 持有所有 Mobject 和 Camera 的引用
- 驱动动画 Tick
- 触发渲染输出

### 实际 API（基于 UEMotionScene.h）

```cpp
UCLASS(BlueprintType)
class UUEMotionScene : public UObject
{
    GENERATED_BODY()

public:
    // ========== 生命周期 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void Initialize(int32 Width = 1920, int32 Height = 1080);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void Destroy();

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    bool IsInitialized() const;

    // ========== 相机 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    UUEMotionCamera* GetCamera();

    // ========== Mobject 创建（6种基础形状）==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    UUEMotionMobject* CreateSphere(float Radius = 50.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    UUEMotionMobject* CreateCube(float Size = 50.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    UUEMotionMobject* CreateCylinder(float Radius = 50.0f, float Height = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    UUEMotionMobject* CreateCone(float Radius = 50.0f, float Height = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    UUEMotionMobject* CreatePlane(float Width = 500.0f, float Height = 500.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    UUEMotionMobject* CreateTorus(float OuterRadius = 80.0f, float InnerRadius = 25.0f);

    // ========== 灯光 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void AddDirectionalLight(const FVector& Direction, const FLinearColor& Color, float Intensity = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity = 5000.0f);

    // ========== 动画驱动 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void Play(UUEMotionAnimation* Animation);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void Tick(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void StopAll();

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    bool HasActiveAnimations() const;

    // ========== 渲染输出 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void RenderFrames(const FString& OutputDirectory, float Duration, float FPS = 30.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void RenderSingleFrame(const FString& FilePath);

    // ========== 查询与管理 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    TArray<UUEMotionMobject*> GetAllMobjects() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion")
    void ClearScene();

private:
    bool bInitialized = false;
    int32 ResolutionWidth = 1920;
    int32 ResolutionHeight = 1080;

    UPROPERTY()
    AUEMotionSceneActor* SceneActor;

    UPROPERTY()
    UUEMotionCamera* Camera;

    UPROPERTY()
    TArray<UUEMotionMobject*> Mobjects;

    UPROPERTY()
    TArray<UUEMotionAnimation*> ActiveAnimations;

    UPROPERTY()
    UUEMotionRenderer* Renderer;
};
```

### 与原始设计的差异

| 原始设计 API | 实际实现 | 状态 |
|-------------|---------|------|
| `CreateArrow()` | ❌ 不存在 | 未实现 |
| `CreateLine()` | ❌ 不存在 | 未实现 |
| `CreateCircle()` | ❌ 不存在 | 未实现 |
| `CreateGrid()` | ❌ 不存在 | 未实现 |
| `CreateCoordinateAxes()` | ❌ 不存在 | 未实现 |
| `CreateText()` | ❌ 不存在 | 未实现 |
| `SetAmbientLight()` | ❌ 不存在 | 未实现 |
| `PlaySequential()` | ❌ 不存在 | 用 GroupAnimation 替代 |
| `GetMobjectByName()` | ❌ 不存在 | 未实现 |
| `ClearScene()` | ✅ 新增 | 已实现 |
| `HasActiveAnimations()` | ✅ 新增 | 已实现 |
| `Renderer` 成员 | ✅ 新增 | 内部持有渲染器 |

**说明**: 当前版本聚焦核心功能（6种形状），高级形状（Arrow/Line/Grid/Axes/Text）可在后续版本添加。

### Python 调用示例

```python
import unreal

scene = unreal.UEMotionScene()
scene.initialize(width=1920, height=1080)

sphere = scene.create_sphere(50.0)
sphere.set_color(unreal.LinearColor(1, 0, 0, 1))

camera = scene.get_camera()
camera.set_position(0, -300, 0)
camera.look_at(sphere)

scene.render_frame("D:/output/frame.png")
```

## 3. UUEMotionCamera — 相机控制 ✅

```cpp
UCLASS(BlueprintType)
class UUEMotionCamera : public UObject
{
    GENERATED_BODY()

public:
    // ========== 定位 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    void SetPosition(float X, float Y, float Z);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    void SetPositionVector(const FVector& Position);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    FVector GetPosition() const;

    // ========== 旋转 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    void SetRotation(float Pitch, float Yaw, float Roll);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    FRotator GetRotation() const;

    // ========== FOV ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    void SetFOV(float FieldOfView);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    float GetFOV() const;

    // ========== 注视 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    void LookAt(UEMotionMobject* Target);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    void LookAtPosition(const FVector& Target);

    // ========== 环绕 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    void OrbitAround(UEMotionMobject* Target, float Radius, float AngleDegrees);

    // ========== 动画 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    UEMotionMoveAnimation* CreateMoveAnimation();

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
    UEMotionRotateAnimation* CreateRotateAnimation();

private:
    UPROPERTY()
    FVector CurrentPosition;

    UPROPERTY()
    FRotator CurrentRotation;

    UPROPERTY()
    float CurrentFOV = 90.0f;

    UPROPERTY()
    TWeakObjectPtr<ACameraActor> InternalCamera;
};
```

## 4. UUEMotionMobject — 可视对象基类 ✅

### 职责
- 封装一个 UE Actor + **StaticMeshComponent**（使用UE内置基础形状）
- 提供位置/旋转/缩放/颜色/透明度/材质控制
- 持有动画构建器入口（通过 `Create*Animation()` 方法）

### 实际 API（基于 UEMotionMobject.h）

```cpp
UCLASS(BlueprintType)
class UUEMotionMobject : public UObject
{
    GENERATED_BODY()

public:
    // ========== 初始化 ==========
    void InitializeAsSphere(AUEMotionSceneActor* Owner, float Radius);
    void InitializeAsMesh(AUEMotionSceneActor* Owner, const FString& MeshPath, float Scale);

    // ========== 元数据 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FString GetName() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetMobjectName(const FString& Name);

    // ========== 可见性 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetVisibility(bool bVisible);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    bool GetVisibility() const;

    // ========== 变换 (直接设置) ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetLocation(const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FVector GetLocation() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetScale(const FVector& Scale);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FVector GetScale() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetRotation(const FRotator& Rotation);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FRotator GetRotation() const;

    // ========== 颜色 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetColor(const FLinearColor& Color);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FLinearColor GetColor() const;

    // ========== 透明度 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetOpacity(float Opacity);  // 0.0 ~ 1.0

    // ========== 材质 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetMaterial(const FString& MaterialPath);  // 资源路径

    // ========== 几何信息 (只读) ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FBox GetBounds() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FVector GetCenter() const;

    // ========== 关键方法: 返回动画对象 ==========
    // 每个返回一个预设参数的 UUEMotion*Animation 子类
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject|Animation")
    UEMotionMoveAnimation* CreateMoveAnimation();

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject|Animation")
    UEMotionRotateAnimation* CreateRotateAnimation();

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject|Animation")
    UEMotionScaleAnimation* CreateScaleAnimation();

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject|Animation")
    UEMotionFadeAnimation* CreateFadeAnimation();

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject|Animation")
    UEMotionColorAnimation* CreateColorAnimation();

    // ========== 内部 ==========
    FString MobjectName;
    bool bVisible = true;
    float CurrentOpacity = 1.0f;
    FLinearColor CurrentColor = FLinearColor::White;

    UPROPERTY()
    TWeakObjectPtr<AActor> InternalActor;

    UPROPERTY()
    TWeakObjectPtr<UStaticMeshComponent> MeshComponent;  // 注意：使用 StaticMeshComponent

    UPROPERTY()
    TWeakObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

    UPROPERTY()
    UMaterialInterface* CachedBaseMaterial = nullptr;
};
```

## 5. UUEMotionAnimation — 动画基类 + 子类 ✅

### 实际 API（基于 UEMotionAnimation.h）

```cpp
// ========== 动画基类 (Abstract) ==========
UCLASS(BlueprintType, Abstract)
class UUEMotionAnimation : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetDuration(float InDuration) { Duration = InDuration; }

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    float GetDuration() const { return Duration; }

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetEasing(const FString& Type) { EasingType = Type; }

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    FString GetEasing() const { return EasingType; }

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    virtual bool IsFinished() const { return Elapsed >= Duration; }

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    float GetProgress() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    virtual void Reset() { Elapsed = 0.0f; }

    // 内部推进方法
    void Advance(float DeltaTime);

    static float ApplyEasing(const FString& Type, float t);

protected:
    virtual void TickAnimation(float DeltaTime, float EasedProgress) {}

    UPROPERTY()
    float Duration = 1.0f;

    UPROPERTY()
    float Elapsed = 0.0f;

    UPROPERTY()
    FString EasingType = TEXT("linear");
};


// ========== 位移动画 ==========
UCLASS(BlueprintType)
class UUEMotionMoveAnimation : public UUEMotionAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetTarget(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetStart(const FVector& StartLocation);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetTargetMobject(UEMotionMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UEMotionMobject* TargetMobject;

    FVector Start;
    FVector End;
};


// ========== 旋转动画 ==========
UCLASS(BlueprintType)
class UEMotionRotateAnimation : public UEMotionAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetRotationAngle(float AngleDegrees);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetAxis(const FVector& InAxis);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetTargetMobject(UEMotionMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UEMotionMobject* TargetMobject;

    float Angle = 360.0f;
    FVector Axis = FVector(0, 0, 1);
};


// ========== 缩放动画 ==========
UCLASS(BlueprintType)
class UEMotionScaleAnimation : public UEMotionAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetStartScale(float InStartScale);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetEndScale(float InEndScale);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetTargetMobject(UEMotionMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UEMotionMobject* TargetMobject;

    float StartScale = 1.0f;
    float EndScale = 2.0f;
};


// ========== 淡入淡出动画 ==========
UCLASS(BlueprintType)
class UEMotionFadeAnimation : public UEMotionAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetFadeIn(bool bIn);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetFadeOut(bool bOut);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetTargetMobject(UEMotionMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UEMotionMobject* TargetMobject;

    bool bFadeIn = false;
    bool bFadeOut = false;
};


// ========== 颜色渐变动画 ==========
UCLASS(BlueprintType)
class UEMotionColorAnimation : public UEMotionAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetStartColor(const FLinearColor& InColor);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetEndColor(const FLinearColor& InColor);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetTargetMobject(UEMotionMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UEMotionMobject* TargetMobject;

    FLinearColor StartColor;
    FLinearColor EndColor;
};


// ========== 等待动画 ==========
UCLASS(BlueprintType)
class UEMotionWaitAnimation : public UEMotionAnimation
{
    GENERATED_BODY()

    // 什么都不做，纯粹等待 Duration 秒
};


// ========== 组合动画 ==========
UCLASS(BlueprintType)
class UEMotionGroupAnimation : public UEMotionAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void AddAnimation(UEMotionAnimation* Animation);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetPlayMode(bool bSequential);  // true=顺序, false=同时

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    TArray<UEMotionAnimation*> ChildAnimations;

    bool bPlaySequential = false;
};
```

## 6. UEMotionBlueprintLibrary — 静态辅助

```cpp
UCLASS()
class UEMotionBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ========== 形状快速创建 (直接生成 Actor) ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Factory")
    static UProceduralMeshComponent* CreateBoxMesh(float Size);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Factory")
    static UProceduralMeshComponent* CreateSphereMesh(float Radius, int32 Segments = 32);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Factory")
    static UProceduralMeshComponent* CreateCylinderMesh(float Radius, float Height, int32 Segments = 32);

    // ========== 工具函数 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Util")
    static float EaseLinear(float t);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Util")
    static float EaseInQuad(float t);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Util")
    static float EaseOutQuad(float t);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Util")
    static float EaseInOutQuad(float t);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Util")
    static float ApplyEasing(const FString& EasingType, float t);

    // ========== 颜色工具 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Util")
    static FLinearColor LerpColor(const FLinearColor& A, const FLinearColor& B, float Alpha);

    // ========== 数学工具 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Util")
    static FVector LerpVector(const FVector& A, const FVector& B, float Alpha);
};
```

## 7. 完整的 Python 使用示例

```python
import unreal

# 1. 创建场景
scene = unreal.UEMotionScene()
scene.initialize(width=1920, height=1080)

# 2. 光照
scene.add_directional_light([0, -1, -1], unreal.LinearColor(1, 1, 1, 1), 10.0)
scene.set_ambient_light(unreal.LinearColor(0.15, 0.15, 0.15, 1))

# 3. 相机
camera = scene.get_camera()
camera.set_position(-250, -300, 150)
camera.look_at_position([0, 0, 0])

# 4. 创建物体
sphere = scene.create_sphere(50.0)
sphere.set_location([0, 0, 100])
sphere.set_color(unreal.LinearColor(1, 0.2, 0.2, 1))

cube = scene.create_cube(80.0)
cube.set_location([150, 0, 0])
cube.set_color(unreal.LinearColor(0.2, 0.4, 1, 1))

ground = scene.create_plane(600.0, 600.0)
ground.set_location([0, 0, -100])
ground.set_color(unreal.LinearColor(0.3, 0.3, 0.3, 1))

axes = scene.create_coordinate_axes(300.0)

# 5. 动画 - 方式A：创建动画对象
move = sphere.create_move_animation()
move.set_target([200, 0, 100])
move.set_duration(2.0)
move.set_easing("ease_in_out")
scene.play(move)

# 6. 动画 - 方式B：组合动画
fade = sphere.create_fade_animation()
fade.set_fade_out(True)
fade.set_duration(1.0)

rotate = cube.create_rotate_animation()
rotate.set_rotation_angle(360)
rotate.set_duration(3.0)

group = unreal.UEMotionGroupAnimation()
group.add_animation(fade)
group.add_animation(rotate)
group.set_play_mode(False)  # 同时播放
scene.play(group)

# 7. 渲染
scene.render_frame("D:/output/frame_hello.png")

# 8. 批量渲染动画序列
scene.render_frames("D:/output/frames/", duration=5.0, fps=30.0)
```

## 8. 设计原则总结

| 原则 | 体现 |
|------|------|
| **单一职责** | Scene 管世界、Camera 管视角、Mobject 管物体、Animation 管插值 |
| **开放封闭** | 动画基类可扩展，新增动画类型只需加子类，不改 Scene |
| **Python 友好** | 所有参数用基础类型（FVector/FLinearColor/String/float），不依赖复杂模板 |
| **与 Manim 对齐** | Mobject → 物体，Animation → 动效，Scene.play() → 播放 |
