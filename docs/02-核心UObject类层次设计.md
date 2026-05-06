# 02 - 核心 UObject 类层次设计

## 1. 类层次总览

```
UObject
├── UPyManimScene              # 场景管理器
├── UPyManimCamera             # 相机控制
├── UPyManimMobject            # 可视对象基类
│   ├── (派生: 由工厂方法创建具体形状)
├── UPyManimAnimation          # 动画基类
│   ├── UPyManimMoveAnimation
│   ├── UPyManimRotateAnimation
│   ├── UPyManimScaleAnimation
│   ├── UPyManimFadeAnimation
│   ├── UPyManimColorAnimation
│   ├── UPyManimWaitAnimation
│   └── UPyManimGroupAnimation
└── UPyManimSequence           # 动画序列（复合动画容器）

AActor
└── APyManimSceneActor         # 场景在世界的物理载体

UBlueprintFunctionLibrary
└── UPyManimBlueprintLibrary   # 静态辅助：形状创建、工具函数
```

所有 `UObject` 子类均标记 `UCLASS(BlueprintType)`，确保暴露到 Python。

## 2. UPyManimScene — 场景管理器

### 职责
- 管理整个渲染世界的生命周期
- 持有所有 Mobject 和 Camera 的引用
- 驱动动画 Tick
- 触发渲染输出

### 设计

```cpp
UCLASS(BlueprintType)
class UPyManimScene : public UObject
{
    GENERATED_BODY()

public:
    // ========== 生命周期 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void Initialize(int32 Width = 1920, int32 Height = 1080);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void Destroy();

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    bool IsInitialized() const;

    // ========== 相机 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimCamera* GetCamera();

    // ========== Mobject 创建 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateSphere(float Radius = 50.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateCube(float Size = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateCylinder(float Radius = 50.0f, float Height = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateCone(float Radius = 50.0f, float Height = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreatePlane(float Width = 200.0f, float Height = 200.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateTorus(float OuterRadius = 100.0f, float InnerRadius = 30.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateArrow(float Length = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateLine(FVector Start, FVector End);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateCircle(float Radius = 100.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateGrid(float Size = 1000.0f, int32 Divisions = 10);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateCoordinateAxes(float Length = 500.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* CreateText(const FString& Text, float Size = 32.0f);

    // ========== 灯光 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void AddDirectionalLight(FVector Direction = FVector(0, -1, -1), FLinearColor Color = FLinearColor::White, float Intensity = 10.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void AddPointLight(FVector Location = FVector(0, 0, 200), FLinearColor Color = FLinearColor::White, float Intensity = 5000.0f);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void SetAmbientLight(FLinearColor Color = FLinearColor(0.1f, 0.1f, 0.1f));

    // ========== 动画驱动 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void Play(UPyManimAnimation* Animation);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void PlaySequential(TArray<UPyManimAnimation*> Animations);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void StopAll();

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    bool IsPlaying() const;

    // ========== Tick (每帧调用) ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void Tick(float DeltaTime);

    // ========== 渲染输出 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void RenderFrame(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    void RenderFrames(const FString& OutputDir, float Duration, float FPS = 30.0f);

    // ========== 查询 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    TArray<UPyManimMobject*> GetAllMobjects() const;

    UFUNCTION(BlueprintCallable, Category = "PyManim|Scene")
    UPyManimMobject* GetMobjectByName(const FString& Name) const;

private:
    UPROPERTY()
    APyManimSceneActor* SceneActor;

    UPROPERTY()
    UPyManimCamera* Camera;

    UPROPERTY()
    TArray<UPyManimMobject*> Mobjects;

    UPROPERTY()
    TArray<UPyManimAnimation*> ActiveAnimations;

    bool bInitialized = false;
};
```

### Python 调用示例

```python
import unreal

scene = unreal.PyManimScene()
scene.initialize(width=1920, height=1080)

sphere = scene.create_sphere(50.0)
sphere.set_color(unreal.LinearColor(1, 0, 0, 1))

camera = scene.get_camera()
camera.set_position(0, -300, 0)
camera.look_at(sphere)

scene.render_frame("D:/output/frame.png")
```

## 3. UPyManimCamera — 相机控制

```cpp
UCLASS(BlueprintType)
class UPyManimCamera : public UObject
{
    GENERATED_BODY()

public:
    // ========== 定位 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    void SetPosition(float X, float Y, float Z);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    void SetPositionVector(const FVector& Position);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    FVector GetPosition() const;

    // ========== 旋转 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    void SetRotation(float Pitch, float Yaw, float Roll);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    FRotator GetRotation() const;

    // ========== FOV ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    void SetFOV(float FieldOfView);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    float GetFOV() const;

    // ========== 注视 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    void LookAt(UPyManimMobject* Target);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    void LookAtPosition(const FVector& Target);

    // ========== 环绕 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    void OrbitAround(UPyManimMobject* Target, float Radius, float AngleDegrees);

    // ========== 动画 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    UPyManimMoveAnimation* CreateMoveAnimation();

    UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
    UPyManimRotateAnimation* CreateRotateAnimation();

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

## 4. UPyManimMobject — 可视对象基类

### 职责
- 封装一个 UE Actor + ProceduralMeshComponent
- 提供位置/旋转/缩放/颜色/材质控制
- 持有 `.animate` 动画构建器入口

```cpp
UCLASS(BlueprintType)
class UPyManimMobject : public UObject
{
    GENERATED_BODY()

public:
    // ========== 元数据 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetName(const FString& Name);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    FString GetName() const;

    // ========== 可见性 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetVisibility(bool bVisible);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    bool IsVisible() const;

    // ========== 变换 (直接设置) ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetTransform(const FTransform& Transform);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    FTransform GetTransform() const;

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetLocation(const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetRotation(const FRotator& Rotation);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetScale(const FVector& Scale);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    FVector GetLocation() const;

    // ========== 颜色 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetColor(const FLinearColor& Color);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    FLinearColor GetColor() const;

    // ========== 透明度 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetOpacity(float Opacity);  // 0.0 ~ 1.0

    // ========== 材质 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    void SetMaterial(const FString& MaterialPath);  // 资源路径

    // ========== 几何信息 (只读) ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    FBox GetBounds() const;

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
    FVector GetCenter() const;

    // ========== 关键方法: 返回动画对象 ==========
    // 每个返回一个预设参数的 UPyManim*Animation 子类
    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject|Animation")
    UPyManimMoveAnimation* CreateMoveAnimation();

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject|Animation")
    UPyManimRotateAnimation* CreateRotateAnimation();

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject|Animation")
    UPyManimScaleAnimation* CreateScaleAnimation();

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject|Animation")
    UPyManimFadeAnimation* CreateFadeAnimation();

    UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject|Animation")
    UPyManimColorAnimation* CreateColorAnimation();

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

## 5. UPyManimAnimation — 动画基类 + 子类

```cpp
// ========== 动画基类 ==========
UCLASS(BlueprintType)
class UPyManimAnimation : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetDuration(float InDuration);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    float GetDuration() const;

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetEasing(const FString& EasingType);  // "linear", "ease_in", "ease_out", "ease_in_out"

    // 由 Scene::Tick 调用，子类覆写
    virtual void Tick(float DeltaTime, float Progress) {}

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    bool IsFinished() const;

protected:
    float Duration = 1.0f;
    float Elapsed = 0.0f;
    FString EasingType = "linear";
};


// ========== 位移动画 ==========
UCLASS(BlueprintType)
class UPyManimMoveAnimation : public UPyManimAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetTarget(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetStart(const FVector& StartLocation);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetTargetMobject(UPyManimMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UPyManimMobject* TargetMobject;

    FVector Start;
    FVector End;
};


// ========== 旋转动画 ==========
UCLASS(BlueprintType)
class UPyManimRotateAnimation : public UPyManimAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetRotationAngle(float AngleDegrees);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetAxis(const FVector& InAxis);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetTargetMobject(UPyManimMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UPyManimMobject* TargetMobject;

    float Angle = 360.0f;
    FVector Axis = FVector(0, 0, 1);
};


// ========== 缩放动画 ==========
UCLASS(BlueprintType)
class UPyManimScaleAnimation : public UPyManimAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetStartScale(float InStartScale);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetEndScale(float InEndScale);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetTargetMobject(UPyManimMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UPyManimMobject* TargetMobject;

    float StartScale = 1.0f;
    float EndScale = 2.0f;
};


// ========== 淡入淡出动画 ==========
UCLASS(BlueprintType)
class UPyManimFadeAnimation : public UPyManimAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetFadeIn(bool bIn);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetFadeOut(bool bOut);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetTargetMobject(UPyManimMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UPyManimMobject* TargetMobject;

    bool bFadeIn = false;
    bool bFadeOut = false;
};


// ========== 颜色渐变动画 ==========
UCLASS(BlueprintType)
class UPyManimColorAnimation : public UPyManimAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetStartColor(const FLinearColor& InColor);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetEndColor(const FLinearColor& InColor);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetTargetMobject(UPyManimMobject* InTarget);

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    UPyManimMobject* TargetMobject;

    FLinearColor StartColor;
    FLinearColor EndColor;
};


// ========== 等待动画 ==========
UCLASS(BlueprintType)
class UPyManimWaitAnimation : public UPyManimAnimation
{
    GENERATED_BODY()

    // 什么都不做，纯粹等待 Duration 秒
};


// ========== 组合动画 ==========
UCLASS(BlueprintType)
class UPyManimGroupAnimation : public UPyManimAnimation
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void AddAnimation(UPyManimAnimation* Animation);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
    void SetPlayMode(bool bSequential);  // true=顺序, false=同时

    virtual void Tick(float DeltaTime, float Progress) override;

    UPROPERTY()
    TArray<UPyManimAnimation*> ChildAnimations;

    bool bPlaySequential = false;
};
```

## 6. UPyManimBlueprintLibrary — 静态辅助

```cpp
UCLASS()
class UPyManimBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ========== 形状快速创建 (直接生成 Actor) ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Factory")
    static UProceduralMeshComponent* CreateBoxMesh(float Size);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Factory")
    static UProceduralMeshComponent* CreateSphereMesh(float Radius, int32 Segments = 32);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Factory")
    static UProceduralMeshComponent* CreateCylinderMesh(float Radius, float Height, int32 Segments = 32);

    // ========== 工具函数 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Util")
    static float EaseLinear(float t);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Util")
    static float EaseInQuad(float t);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Util")
    static float EaseOutQuad(float t);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Util")
    static float EaseInOutQuad(float t);

    UFUNCTION(BlueprintCallable, Category = "PyManim|Util")
    static float ApplyEasing(const FString& EasingType, float t);

    // ========== 颜色工具 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Util")
    static FLinearColor LerpColor(const FLinearColor& A, const FLinearColor& B, float Alpha);

    // ========== 数学工具 ==========
    UFUNCTION(BlueprintCallable, Category = "PyManim|Util")
    static FVector LerpVector(const FVector& A, const FVector& B, float Alpha);
};
```

## 7. 完整的 Python 使用示例

```python
import unreal

# 1. 创建场景
scene = unreal.PyManimScene()
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

group = unreal.PyManimGroupAnimation()
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
