# 02 - 核心 UObject 类层次设计

## 1. 类层次总览

```
UObject
├── UEMotionScene              # 场景管理器
├── UEMotionCamera             # 相机控制
├── UEMotionMobject            # 可视对象基类
│   ├── (派生: 由工厂方法创建具体形状)
├── UEMotionAnimation          # 动画基类
│   ├── UEMotionMoveAnimation
│   ├── UEMotionRotateAnimation
│   ├── UEMotionScaleAnimation
│   ├── UEMotionFadeAnimation
│   ├── UEMotionColorAnimation
│   ├── UEMotionWaitAnimation
│   └── UEMotionGroupAnimation
└── UEMotionSequence           # 动画序列（复合动画容器）

AActor
└── AUEMotionSceneActor         # 场景在世界的物理载体

UBlueprintFunctionLibrary
└── UEMotionBlueprintLibrary   # 静态辅助：形状创建、工具函数
```

所有 `UObject` 子类均标记 `UCLASS(BlueprintType)`，确保暴露到 Python。

## 2. UEMotionScene — 场景管理器

### 职责
- 管理整个渲染世界的生命周期
- 持有所有 Mobject 和 Camera 的引用
- 驱动动画 Tick
- 触发渲染输出

### 设计

```cpp
UCLASS(BlueprintType)
class UEMotionScene : public UObject
{
    GENERATED_BODY()

public:
    // ========== 生命周期 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void Initialize(int32 Width = 1920, int32 Height = 1080);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void Destroy();

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    bool IsInitialized() const;

    // ========== 相机 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionCamera* GetCamera();

    // ========== Mobject 创建 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateSphere(float Radius = 50.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateCube(float Size = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateCylinder(float Radius = 50.0f, float Height = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateCone(float Radius = 50.0f, float Height = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreatePlane(float Width = 200.0f, float Height = 200.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateTorus(float OuterRadius = 100.0f, float InnerRadius = 30.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateArrow(float Length = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateLine(FVector Start, FVector End);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateCircle(float Radius = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateGrid(float Size = 1000.0f, int32 Divisions = 10);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateCoordinateAxes(float Length = 500.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* CreateText(const FString& Text, float Size = 32.0f);

    // ========== 灯光 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void AddDirectionalLight(FVector Direction = FVector(0, -1, -1), FLinearColor Color = FLinearColor::White, float Intensity = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void AddPointLight(FVector Location = FVector(0, 0, 200), FLinearColor Color = FLinearColor::White, float Intensity = 5000.0f);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void SetAmbientLight(FLinearColor Color = FLinearColor(0.1f, 0.1f, 0.1f));

    // ========== 动画驱动 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void Play(UEMotionAnimation* Animation);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void PlaySequential(TArray<UEMotionAnimation*> Animations);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void StopAll();

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    bool IsPlaying() const;

    // ========== Tick (每帧调用) ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void Tick(float DeltaTime);

    // ========== 渲染输出 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void RenderFrame(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    void RenderFrames(const FString& OutputDir, float Duration, float FPS = 30.0f);

    // ========== 查询 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    TArray<UEMotionMobject*> GetAllMobjects() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Scene")
    UEMotionMobject* GetMobjectByName(const FString& Name) const;

private:
    UPROPERTY()
    AUEMotionSceneActor* SceneActor;

    UPROPERTY()
    UEMotionCamera* Camera;

    UPROPERTY()
    TArray<UEMotionMobject*> Mobjects;

    UPROPERTY()
    TArray<UEMotionAnimation*> ActiveAnimations;

    bool bInitialized = false;
};
```

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

## 3. UEMotionCamera — 相机控制

```cpp
UCLASS(BlueprintType)
class UEMotionCamera : public UObject
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

## 4. UEMotionMobject — 可视对象基类

### 职责
- 封装一个 UE Actor + ProceduralMeshComponent
- 提供位置/旋转/缩放/颜色/材质控制
- 持有 `.animate` 动画构建器入口

```cpp
UCLASS(BlueprintType)
class UEMotionMobject : public UObject
{
    GENERATED_BODY()

public:
    // ========== 元数据 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetName(const FString& Name);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FString GetName() const;

    // ========== 可见性 ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetVisibility(bool bVisible);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    bool IsVisible() const;

    // ========== 变换 (直接设置) ==========
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetTransform(const FTransform& Transform);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FTransform GetTransform() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetLocation(const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetRotation(const FRotator& Rotation);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    void SetScale(const FVector& Scale);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
    FVector GetLocation() const;

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
    FLinearColor CurrentColor = FLinearColor::White;

    UPROPERTY()
    TWeakObjectPtr<AActor> InternalActor;

    UPROPERTY()
    TWeakObjectPtr<UProceduralMeshComponent> MeshComponent;
};
```

## 5. UEMotionAnimation — 动画基类 + 子类

```cpp
// ========== 动画基类 ==========
UCLASS(BlueprintType)
class UEMotionAnimation : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetDuration(float InDuration);

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    float GetDuration() const;

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    void SetEasing(const FString& EasingType);  // "linear", "ease_in", "ease_out", "ease_in_out"

    // 由 Scene::Tick 调用，子类覆写
    virtual void Tick(float DeltaTime, float Progress) {}

    UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
    bool IsFinished() const;

protected:
    float Duration = 1.0f;
    float Elapsed = 0.0f;
    FString EasingType = "linear";
};


// ========== 位移动画 ==========
UCLASS(BlueprintType)
class UEMotionMoveAnimation : public UEMotionAnimation
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
