# 05 - Python API 与使用指南

## 1. Python API 映射规则

UE5 的 Python 桥接会自动将 C++ 的 `BlueprintCallable` 函数名转换为 Python 风格：

| C++ | Python (ue5 自动转换) |
|-----|----------------------|
| `GetCamera()` | `.get_camera()` |
| `SetPosition(float, float, float)` | `.set_position(x, y, z)` |
| `CreateSphere(float)` | `.create_sphere(radius)` |
| `RenderFrame(const FString&)` | `.render_frame(file_path)` |
| `FVector(0, 0, 100)` | `unreal.Vector(0, 0, 100)` |
| `FLinearColor(1, 0, 0, 1)` | `unreal.LinearColor(1, 0, 0, 1)` |
| `TArray<T>` | Python `list` / `unreal.Array` |

## 2. 完整 API 参考

### 2.1 UEMotionScene

```python
scene = unreal.UEMotionScene()

# === 生命周期 ===
scene.initialize(width=1920, height=1080)
scene.destroy()
scene.is_initialized()                            # → bool

# === 相机 ===
camera = scene.get_camera()                       # → UEMotionCamera

# === Mobject 创建 ===
sphere  = scene.create_sphere(radius=50.0)
cube    = scene.create_cube(size=100.0)
cylinder = scene.create_cylinder(radius=50.0, height=100.0)
cone    = scene.create_cone(radius=50.0, height=100.0)
plane   = scene.create_plane(width=200.0, height=200.0)
torus   = scene.create_torus(outer_radius=100.0, inner_radius=30.0)
arrow   = scene.create_arrow(length=100.0)
line    = scene.create_line(start=[0,0,0], end=[100,100,0])
circle  = scene.create_circle(radius=100.0)
grid    = scene.create_grid(size=1000.0, divisions=10)
axes    = scene.create_coordinate_axes(length=500.0)
text    = scene.create_text("Hello", size=32.0)

# === 光照 ===
scene.add_directional_light(
    direction=[0, -1, -1],
    color=unreal.LinearColor(1, 1, 1, 1),
    intensity=10.0
)
scene.add_point_light(
    location=[0, 0, 200],
    color=unreal.LinearColor(1, 1, 1, 1),
    intensity=5000.0
)
scene.set_ambient_light(unreal.LinearColor(0.15, 0.15, 0.15, 1))

# === 动画 ===
scene.play(animation)
scene.play_sequential([anim1, anim2, anim3])
scene.stop_all()
scene.tick(delta_time)

# === 渲染 ===
scene.render_frame("D:/output/frame.png")
scene.render_frames("D:/output/frames/", duration=5.0, fps=30.0)

# === 查询 ===
mobjects = scene.get_all_mobjects()
obj = scene.get_mobject_by_name("MySphere")
```

### 2.2 UEMotionCamera

```python
camera = scene.get_camera()

# === 位置 ===
camera.set_position(x=-300, y=-400, z=200)
camera.set_position_vector(unreal.Vector(-300, -400, 200))
pos = camera.get_position()                     # → unreal.Vector

# === 旋转 ===
camera.set_rotation(pitch=-15, yaw=0, roll=0)
rot = camera.get_rotation()                     # → unreal.Rotator

# === FOV ===
camera.set_fov(60.0)
fov = camera.get_fov()

# === 注视 ===
camera.look_at(mobject)
camera.look_at_position([0, 0, 0])

# === 环绕 ===
camera.orbit_around(target=mobject, radius=500.0, angle_degrees=90.0)
```

### 2.3 UEMotionMobject

```python
obj = scene.create_sphere(50.0)

# === 元数据 ===
obj.set_name("MySphere")
name = obj.get_name()

# === 可见性 ===
obj.set_visibility(True)
obj.is_visible()

# === 变换 ===
obj.set_location([100, 0, 0])
obj.set_rotation(unreal.Rotator(0, 90, 0))
obj.set_scale([2, 2, 2])
obj.set_transform(some_transform)

loc = obj.get_location()
trans = obj.get_transform()

# === 颜色 ===
obj.set_color(unreal.LinearColor(1, 0.2, 0.2, 1))
color = obj.get_color()

# === 透明度 ===
obj.set_opacity(0.5)

# === 材质 ===
obj.set_material("/Game/Materials/MyGlowMaterial")

# === 几何 ===
bounds = obj.get_bounds()
center = obj.get_center()

# === 创建动画 ===
move    = obj.create_move_animation()
rotate  = obj.create_rotate_animation()
scale   = obj.create_scale_animation()
fade    = obj.create_fade_animation()
color   = obj.create_color_animation()
```

### 2.4 UEMotionAnimation 子类

```python
# ========== 位移动画 ==========
move = obj.create_move_animation()
move.set_start([0, 0, 0])
move.set_target([200, 0, 100])
move.set_duration(2.0)
move.set_easing("ease_in_out")

# ========== 旋转动画 ==========
rotate = obj.create_rotate_animation()
rotate.set_rotation_angle(360)
rotate.set_axis([0, 0, 1])
rotate.set_duration(3.0)
rotate.set_easing("linear")

# ========== 缩放动画 ==========
scale = obj.create_scale_animation()
scale.set_start_scale(0.5)
scale.set_end_scale(2.0)
scale.set_duration(1.5)

# ========== 淡入淡出 ==========
fade = obj.create_fade_animation()
fade.set_fade_in(True)
fade.set_duration(1.0)

# ========== 颜色渐变 ==========
color_anim = obj.create_color_animation()
color_anim.set_start_color(unreal.LinearColor(1, 0, 0, 1))
color_anim.set_end_color(unreal.LinearColor(0, 0, 1, 1))
color_anim.set_duration(2.0)

# ========== 组合动画 ==========
group = unreal.UEMotionGroupAnimation()
group.add_animation(move)
group.add_animation(rotate)
group.set_play_mode(False)  # False=同时播放, True=顺序播放

# ========== 等待 ==========
wait = unreal.UEMotionWaitAnimation()
wait.set_duration(0.5)
```

## 3. 示例：Hello Sphere

```python
import unreal

scene = unreal.UEMotionScene()
scene.initialize(width=1920, height=1080)

# 光照
scene.add_directional_light([0, -1, -1], unreal.LinearColor(1, 1, 1, 1), 8)
scene.set_ambient_light(unreal.LinearColor(0.15, 0.15, 0.15, 1))

# 相机
camera = scene.get_camera()
camera.set_position(-300, -400, 200)
camera.look_at_position([0, 0, 0])

# 球体
sphere = scene.create_sphere(60)
sphere.set_color(unreal.LinearColor(0.2, 0.6, 1, 1))

# 动画：弹跳效果
bounce_up = sphere.create_move_animation()
bounce_up.set_target([0, 0, 150])
bounce_up.set_duration(1)
bounce_up.set_easing("ease_out")

bounce_down = sphere.create_move_animation()
bounce_down.set_target([0, 0, 0])
bounce_down.set_duration(1)
bounce_down.set_easing("ease_in")

scene.play_sequential([bounce_up, bounce_down])

# 输出
scene.render_frames("D:/output/hello_sphere/", duration=2.0, fps=30)
```

## 4. 示例：数学可视化（3D sin 曲线）

```python
import unreal
import math

scene = unreal.UEMotionScene()
scene.initialize(1920, 1080)

scene.add_directional_light([0, -1, -1], unreal.LinearColor(1,1,1,1), 8)
scene.set_ambient_light(unreal.LinearColor(0.15,0.15,0.2,1))

camera = scene.get_camera()
camera.set_position(0, -600, 150)
camera.look_at_position([0, 0, 50])
scene.create_grid(800, 16)

# 沿 X 轴放置一系列小球，Y 位置 = sin(x)
animations = []
for i in range(20):
    x = -400 + i * 40
    y = math.sin(i * 0.5) * 150
    z = 0

    ball = scene.create_sphere(12)
    ball.set_location([x, y, z])
    ball.set_color(unreal.LinearColor(
        (math.sin(i * 0.3) + 1) / 2,
        (math.cos(i * 0.3) + 1) / 2,
        0.8, 1
    ))

    # 每个球延迟出现
    fade_in = ball.create_fade_animation()
    fade_in.set_fade_in(True)
    fade_in.set_duration(0.3)
    animations.append(fade_in)

scene.play_sequential(animations)
scene.render_frames("D:/output/sin_wave/", duration=8.0, fps=30)
```

## 5. 示例：几何变换

```python
import unreal

scene = unreal.UEMotionScene()
scene.initialize(1920, 1080)
scene.add_directional_light([0, -1, -1], unreal.LinearColor(1,1,1,1), 8)
scene.set_ambient_light(unreal.LinearColor(0.15, 0.15, 0.15, 1))
scene.create_grid(800, 10)

camera = scene.get_camera()
camera.set_position(-300, -400, 200)
camera.look_at_position([0, 0, 0])

# 三个基本形状
sphere = scene.create_sphere(40)
sphere.set_location([-150, 0, 0])
sphere.set_color(unreal.LinearColor(1, 0.3, 0.3, 1))

cube = scene.create_cube(60)
cube.set_location([0, 0, 0])
cube.set_color(unreal.LinearColor(0.3, 0.5, 1, 1))

cone = scene.create_cone(40, 100)
cone.set_location([150, 0, 0])
cone.set_color(unreal.LinearColor(0.3, 1, 0.3, 1))

# 同时旋转
anim_group = unreal.UEMotionGroupAnimation()

for obj in [sphere, cube, cone]:
    rot = obj.create_rotate_animation()
    rot.set_rotation_angle(720)
    rot.set_duration(5)
    anim_group.add_animation(rot)

# 球体缩放
scale_anim = sphere.create_scale_animation()
scale_anim.set_end_scale(2.0)
scale_anim.set_duration(5)
anim_group.add_animation(scale_anim)

anim_group.set_play_mode(False)  # 同时播放
scene.play(anim_group)

scene.render_frames("D:/output/geometry/", duration=6.0, fps=30)
```

## 6. 实用工具：Python 包装层（可选）

如果觉得 `unreal.UEMotionScene()` 太冗长，可以在 Python 侧做一层轻量封装：

```python
# uemotion_wrapper.py — 可选的 Python 封装（放在 Scripts/ 目录）
import unreal

class Scene:
    """Pythonic 场景包装"""
    def __init__(self, width=1920, height=1080):
        self._native = unreal.UEMotionScene()
        self._native.initialize(width, height)

    def sphere(self, radius=50, color=None, location=None):
        obj = self._native.create_sphere(radius)
        if color:
            obj.set_color(color)
        if location:
            obj.set_location(location)
        return Mobject(obj)

    def cube(self, size=100, color=None, location=None):
        obj = self._native.create_cube(size)
        if color:
            obj.set_color(color)
        if location:
            obj.set_location(location)
        return Mobject(obj)

    def camera(self):
        return self._native.get_camera()

    def play(self, animation):
        self._native.play(animation._native if hasattr(animation, '_native') else animation)

    def render_frame(self, path):
        self._native.render_frame(path)

    def render(self, output_dir, duration, fps=30):
        self._native.render_frames(output_dir, duration, fps)

    @property
    def native(self):
        return self._native


class Mobject:
    """Pythonic Mobject 包装"""
    def __init__(self, native):
        self._native = native

    def move_animation(self, target, duration=1):
        anim = self._native.create_move_animation()
        anim.set_target(target)
        anim.set_duration(duration)
        return anim

    def rotate_animation(self, angle, duration=1):
        anim = self._native.create_rotate_animation()
        anim.set_rotation_angle(angle)
        anim.set_duration(duration)
        return anim

    @property
    def native(self):
        return self._native


# ===== 使用示例 =====
if __name__ == "__main__":
    RED = unreal.LinearColor(1, 0.2, 0.2, 1)
    BLUE = unreal.LinearColor(0.2, 0.4, 1, 1)

    s = Scene(1920, 1080)
    ball = s.sphere(60, color=RED, location=[-200, 0, 50])
    box = s.cube(80, color=BLUE, location=[200, 0, 50])

    s.play(ball.move_animation([200, 0, 50], duration=3))
    s.play(box.rotate_animation(360, duration=2))

    s.render("D:/output/wrapped/", duration=5.0, fps=30)
```

## 7. 常见问题

### Q: Python 脚本放哪里？
放在项目根目录的 `Scripts/` 文件夹。CLI 模式用绝对路径或相对路径。

### Q: 如何访问项目资产？
```python
# 加载材质、贴图等
material = unreal.EditorAssetLibrary.load_asset("/Game/Materials/MyMat")
obj.set_material(material)  # 如果 SetMaterial 接受 UObject*
```

### Q: import unreal 报错？
确保 `PythonScriptPlugin` 已在 `PluginTest.uproject` 中启用，且在 UE 编辑器环境中运行。

### Q: 脚本执行到一半想看结果？
在交互模式下，`scene.render_frame(...)` 会立即输出当前帧的 PNG，可以随时查看。
