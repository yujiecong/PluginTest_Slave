# UEMotion 路线图 v2.0

> **目标**: 基于 UE5 的好玩的可视化渲染平台，用 Sequencer 快速出动画
> **核心理念**: 不复刻 Manim，而是发挥 UE + Sequencer 的独特优势

---

## 当前状态快照

| 维度 | 现状 | 说明 |
|------|------|------|
| **基础架构** | ✅ 可用 | Scene/Mobject/Animation/Camera 四件套 |
| **3D几何体** | ⚠️ 基础 | 6种（Sphere/Cube/Cylinder/Cone/Torus/Plane）|
| **动画系统** | ⚠️ 基础 | Move/Rotate/Scale/Fade/Color + 简单组合 |
| **2D图形** | ❌ 缺失 | Circle/Square/Line/Arrow/Polygon... |
| **文本系统** | ❌ 缺失 | Text/LaTeX/Code... |
| **Sequencer集成** | ⚠️ 浅层 | 能录关键帧，但缺少高级抽象 |
| **实时预览** | ✅ 核心优势 | UE Editor 即时反馈 |
| **渲染质量** | ✅ 核心优势 | Nanite/Lumen/PBR |

---

## Phase 0: 基础设施加固 (Week 1-2)

### 目标：让现有系统稳定、好用、易扩展

#### 0.1 错误处理体系
```python
# 当前问题：UE返回nullptr → Python崩溃
# 目标：统一异常捕获 + 友好错误信息

class UEMotionError(Exception):
    pass

def safe_call(ue_function, *args, **kwargs):
    try:
        result = ue_function(*args, **kwargs)
        if result is None:
            raise UEMotionError(f"{ue_function.__name__} returned None")
        return result
    except Exception as e:
        raise UEMotionError(f"UE call failed: {e}")
```

#### 0.2 配置系统集中化
```python
# 新增 config.py
class Config:
    RESOLUTION = (1080, 1080)
    FPS = 30
    FRAME_SIZE = (8.0, 8.0)  # UEMotion units
    SCALE_FACTOR = 50.0       # 1 unit = 50 UE cm
    CAMERA_Z = 500.0          # 2D camera height
    BACKGROUND_COLOR = "#1a1a2e"
```

#### 0.3 缓动函数模块化
```python
# easing.py - 从animation.py中提取
EASING_FUNCTIONS = {
    'linear': lambda t: t,
    'ease_in_out': lambda t: t * t * (3 - 2 * t),
    'ease_in': lambda t: t * t,
    'ease_out': lambda t: t * (2 - t),
    # Phase 0 先做这4个，够用了
}
```

#### 0.4 测试基建增强
- [ ] 每个 public 方法至少 1 个 happy path + 1 个 edge case
- [ ] Smoke test 自动化（每次 push 都跑）
- [ ] 性能基准测试（记录渲染时间）

**交付物**:
- ✅ `config.py` 集中配置
- ✅ `easing.py` 独立缓动模块
- ✅ 统一错误装饰器
- ✅ 测试覆盖率 > 80%（现有代码）

---

## Phase 1: Sequencer 深度集成 (Week 3-5) ⭐ 核心

### 目标：让 Sequencer 成为第一公民，而非黑盒

#### 1.1 动画时间轴 API
```python
s = Scene("demo")

# 当前方式（底层）
cube = s.cube(size=50)
anim = Animation(s)
anim._add(cube.move_to(UR, duration=2))
built = anim.build()

# 新方式（高层，Sequencer友好）
timeline = s.timeline

with timeline.section("intro", duration=2):
    cube.move_to(ORIGIN)

with timeline.section("main", duration=3):
    cube.move_to(UR)
    cube.rotate(angle=360, axis=UP)

with timeline.section("outro", duration=1):
    cube.fade_out()

s.render()  # 自动生成 Sequencer Track
```

#### 1.2 关键帧直接操作
```python
# 直接操作关键帧（给高级用户）
track = timeline.add_track(cube, property="location")

track.add_keyframe(time=0.0, value=DL)
track.add_keyframe(time=2.5, value=ORIGIN, easing="ease_in_out")
track.add_keyframe(time=5.0, value=UR)

# 可视化调试
track.show_curve_editor()  # 在UE Editor中打开曲线编辑器
```

#### 1.3 动画预设库
```python
# 常用动画模式一键应用
cube.appear(duration=1)           # FadeIn + Scale from 0
cube.disappear(duration=1)        # FadeOut + Scale to 0
cube.pulse(count=3, duration=0.5) # Scale 1.0→1.2→1.0 循环
cube.shake(intensity=0.1)         # 随机位移抖动
cube.highlight(color=YELLOW)      # 颜色闪烁 + Glow
```

#### 1.4 多对象编排
```python
# 编排多个对象的动画序列
s.play(
    circle.appear(),
    square.shift(RIGHT),
    run_time=1.5,
    lag_ratio=0.1,  # 错开开始
)

s.play_sequence([
    ("intro", [circle.appear(), text.write()]),
    ("transform", [circle.morph_into(square)]),
    ("outro", [all_objects.fade_out()]),
])
```

**技术要点**:
- 每个动画调用生成对应的 `UMovieSceneSection`
- 支持嵌套时间轴（子轨道）
- Easing 曲线映射到 UE 的 `RichCurve`
- 导出为 `.uelevel` 或直接在 Editor 中预览

**交付物**:
- ✅ `timeline.py` 时间轴管理器
- ✅ `presets.py` 动画预设库
- ✅ Sequencer Track 自动生成
- ✅ 曲线编辑器集成
- ✅ 10+ 动画预设（appear/disappear/pulse/shake/highlight...）

---

## Phase 2: 2D 图形系统 (Week 6-8)

### 目标：支持数学可视化的基础图形，但不追求 Manim 式完备性

#### 2.1 核心 2D 对象（TOP 8）
```python
# 必须有的（覆盖80%使用场景）
s.circle(radius=1, color=BLUE, fill=True)
s.square(side_length=1.5, color=RED)
s.rectangle(width=2, height=1, color=GREEN)
s.line(start=(-2, 0), end=(2, 0), thickness=0.05)
s.arrow(start=ORIGIN, end=(1, 1), color=YELLOW)
s.dot(radius=0.08, color=WHITE)
s.arc(radius=1, angle=PI/2, start_angle=0)
s.regular_polygon(n_sides=6, radius=1, color=CYAN)
```

**技术方案**: ProceduralMeshComponent + 顶点生成算法

#### 2.2 图形属性
```python
circle = s.circle(radius=1, color=BLUE)

# 描边 vs 填充
circle.stroke(width=0.05, color=DARK_BLUE)
circle.fill(opacity=0.7)

# 渐变（UE优势！）
circle.fill_gradient(colors=[BLUE, CYAN], direction="radial")

# 发光效果（后处理）
circle.glow(intensity=0.5, color=BLUE, radius=0.3)
```

#### 2.3 图形组合
```python
group = VGroup(circle, square, triangle)

# 批量操作
group.scale(1.5)
group.move_to(UR)
group.rotate(angle=45)

# 排列布局
group.arrange(direction=RIGHT, buff=0.3)
group.arrange_to_center()

# 子对象访问
group[0].set_color(RED)
```

**交付物**:
- ✅ 8 种 2D 图形对象
- ✅ Stroke/Fill/Gradient 属性
- ✅ VGroup 组合与布局
- ✅ Glow 后处理特效

---

## Phase 3: 文本与标注系统 (Week 9-10)

### 目标：能显示文字和简单公式，够用即可

#### 3.1 基础文本
```python
title = s.text("Hello UEMotion!", font_size=48, color=WHITE)
subtitle = s.text("Powered by Unreal Engine 5", font_size=24, color=GRAY)

# 动画
title.write(duration=2)      # 打字机效果
title.fade_in(shift=UP)      # 从上方淡入
title.typing_error_fix()     # 打错字→删除→重打（趣味性！）
```

**技术方案**: UMG Widget + Widget Component（3D空间渲染）

#### 3.2 数字显示
```python
number = s.decimal_number(value=0, num_decimal_places=2)

# 数字滚动动画
number.count_to(target=3.14159, duration=2)

# 与 ValueTracker 联动
tracker = ValueTracker(0)
number.bind_to(tracker)
tracker.animate_to(100, duration=3)  # number自动更新
```

#### 3.3 简单标注
```python
# 高亮框
rect = s.surrounding_rectangle(target=circle, color=YELLOW, padding=0.2)

# 大括号
brace = s.brace(target=line, direction=DOWN, label="Length: 5 units")

# 角度标记
angle_mark = s.angle_mark(vertex=O, point_a=A, point_b=B, label="45°")
```

**可选（Phase 5）**: LaTeX 公式（依赖 MathJax 或类似库）

**交付物**:
- ✅ Text 文本渲染（支持中文）
- ✅ DecimalNumber 数字动画
- ✅ SurroundingRectangle / Brace / AngleMark
- ✅ 文本动画预设（write/fade_in/typing）

---

## Phase 4: 视觉效果增强 (Week 11-12) ⭐ 差异化优势

### 目标：发挥 UE 的渲染优势，做出 Manim 做不到的效果

#### 4.1 材质系统
```python
cube = s.cube(size=50)

# PBR 材质
cube.set_metallic(0.8)
cube.set_roughness(0.2)
cube.set_normal_map("textures/metal_normal.png")

# 程序化材质
cube.set_material("holographic")  # 全息投影效果
cube.set_material("neon_glow")    # 霓虹灯效果
cube.set_material("glass")        # 玻璃折射

# 材质动画
cube.morph_material(from="solid", to="holographic", duration=2)
```

#### 4.2 光照系统
```python
# 动态光源
light = s.point_light(location=ORIGIN, color=WHITE, intensity=10000)
.light.animate_move_to(UR, duration=3)

# 光照动画
s.fade_lights(in_or_out="out", duration=1)  # 熄灯效果
s.strobe_light(frequency=10, duration=2)   # 闪光灯
```

#### 4.3 后处理特效
```python
# 场景级后处理
s.post_process.bloom(enabled=True, intensity=0.8)
s.post_process.dof(focus_distance=500, blur_amount=0.5)
s.post_process.motion_blur(amount=0.3)
s.post_process.color_grading(lut="cinematic")

# 对象级特效
circle.glow(color=CYAN, intensity=0.6)
square.trail(length=20, fade_time=0.5)  # 运动拖尾
cube.reflection(probe="dynamic")         # 实时反射
```

#### 4.4 粒子系统（Niagara）
```python
# 预设粒子效果
emitter = s.particles("explosion", position=cube.location)
emitter.emit(duration=1.5, count=500)

emitter = s.particles("sparkle", follow=circle)
emitter.emit(continuous=True)  # 持续发射

# 自定义粒子（简化API）
emitter = s.particles_custom(
    template="ring",
    color=GRADIENT([RED, YELLOW]),
    count=100,
    lifetime=2.0,
)
```

**交付物**:
- ✅ 5+ 预设材质（holographic/neon/glass/metal/plasma）
- ✅ 动态光照控制 API
- ✅ 后处理特效包（Bloom/DOF/MotionBlur/Vignette）
- ✅ Niagara 粒子封装（10+ 预设）
- ✅ 运动拖尾 / 发光 / 反射特效

---

## Phase 5: 高级功能 (Week 13-16)

### 5.1 相机动画
```python
# MovingCamera 支持
camera = s.camera

camera.save_state()
camera.zoom(to=0.5, duration=2)       # 拉远
camera.pan_to(target=UR, duration=2)  # 平移
camera.orbit_around(center=ORIGIN, angle=360, duration=5)  # 环绕
camera.restore(duration=1.5)          # 恢复

# 多机位切换
cam_a = s.create_camera("wide_angle", position=(0, 0, 1000))
cam_b = s.create_camera("close_up", position=(0, 0, 200))

s.switch_camera(cam_a, duration=1)  # 切镜效果
```

### 5.2 物理模拟
```python
cube = s.cube(size=50)
cube.enable_physics()
cube.set_mass(10.0)

# 施加力
cube.apply_force(force=(0, 0, -980))  # 重力
cube.apply_impulse(impulse=(1000, 0, 0))  # 冲击

# 物理录制
s.record_physics(duration=5)  # 录制物理模拟到 Sequencer
```

### 5.3 数据可视化（轻量版）
```python
# 坐标系
axes = s.axes(x_range=[-5, 5], y_range=[-3, 3])

# 函数曲线
curve = axes.plot(lambda x: x**2, color=BLUE, samples=100)

# 散点图
scatter = s.scatter_plot(data=[(1,2), (2,3), (3,5)], color=RED)

# 柱状图
bars = s.bar_chart(values=[3, 7, 2, 9], labels=["A","B","C","D"])
```

### 5.4 导出与分享
```python
# 传统导出
s.render(output="animation.mp4", resolution=(1920, 1080))
s.render(output="frames/", format="png_sequence")

# UE 特色导出
s.export_pixel_streaming(port=8888)  # Web实时查看
s.export_vr_template(platform="Quest")  # VR 应用
s.render_realtime_preview()  # 在Editor中循环播放
```

**交付物**:
- ✅ MovingCamera 系统
- ✅ Chaos Physics 封装
- ✅ 基础数据可视化（Axes/Scatter/BarChart）
- ✅ 多格式导出（MP4/GIF/Web/VR）

---

## Phase 6: 生态与工具链 (Week 17-20)

### 6.1 示例画廊
```
examples/
├── basics/
│   ├── hello_world.py          # 第一个动画
│   ├── shapes_and_colors.py    # 图形展示
│   └── animations_101.py       # 动画入门
├── math/
│   ├── function_graph.py       # 函数图像
│   ├── geometry_proof.py       # 几何证明
│   └── calculus_demo.py        # 微积分演示
├── effects/
│   ├── materials_showcase.py   # 材质秀
│   ├── particles_demo.py       # 粒子效果
│   └── post_processing.py      # 后处理
├── interactive/
│   ├── physics_playground.py   # 物理游乐场
│   └── camera_cinematics.py    # 镜头语言
└── real_world/
    ├── presentation_template.py # 演示模板
    └── educational_video.py    # 教学视频片段
```

### 6.2 VS Code 插件
- 语法高亮（UEMotion Python API）
- 代码片段（常用模式快速插入）
- 运行按钮（一键运行脚本并预览）
- 错误提示（集成 UE 日志）

### 6.3 Web Dashboard
- 示例浏览与在线编辑（Monaco Editor）
- 一键复制代码到剪贴板
- 渲染结果 GIF 预览
- 社区作品分享

### 6.4 性能优化
- 大场景 LOD 管理
- Instanced Rendering（批量相同对象）
- GPU Driven Rendering（万级对象）
- 渲染缓存（相同帧不重复计算）

**交付物**:
- ✅ 15+ 示例脚本（覆盖主要使用场景）
- ✅ VS Code 插件 v1.0
- ✅ Web Dashboard MVP
- ✅ 性能优化白皮书

---

## 技术债务清理（穿插进行）

### 必须修复
- [x] ~~align_to 未实现~~ → Phase 0 补齐
- [x] ~~缓动函数硬编码~~ → Phase 0 提取到 easing.py
- [x] ~~缺少错误处理~~ → Phase 0 统一异常体系

### 架构改进
```python
# 当前层次
Mobject (单一基类)

# 目标层次
Mobject (基类，通用接口)
├── Shape2D (Circle, Square, Line...)    # Phase 2
├── Shape3D (Cube, Sphere, Cylinder...)  # 已有
├── TextObject (Text, Number...)          # Phase 3
├── VGroup (组合对象)                      # Phase 2
└── EffectWrapper (Glow/Trail/Reflect...) # Phase 4
```

---

## 成功标准（每个Phase结束时的验收条件）

### Phase 0 通过标准
```bash
# 所有测试通过
pytest tests/ -v

# Smoke test < 30秒
run_full_pipeline_test.bat  # Exit code 0

# 无明显内存泄漏
# 性能回归 < 5%
```

### Phase 1 通过标准
```python
# 能流畅运行这个示例
s = Scene("sequencer_test")
cube = s.cube(size=50, location=DL)

timeline = s.timeline
timeline.add_keyframes(cube, "location", [
    (0.0, DL),
    (2.5, ORIGIN, "ease_in_out"),
    (5.0, UR),
])

s.render(output="test.mp4")
# ✓ Sequencer 中有正确的 Track
# ✓ 关键帧曲线平滑
# ✓ 渲染结果符合预期
```

### Phase 2 通过标准
```python
# 能流畅运行这个示例
s = Scene("shapes_demo")
shapes = [
    s.circle(radius=1, color=BLUE),
    s.square(side_length=1.5, color=RED),
    s.triangle(base=2, height=1.7, color=GREEN),
]

group = VGroup(*shapes).arrange(RIGHT, buff=0.5)
s.play(group.appear())
s.play(group[0].pulse(count=3))
s.render()
# ✓ 2D图形正确显示
# ✓ 动画流畅
# ✓ 组合操作正常
```

### Phase 3 通过标准
```python
s = Scene("text_demo")
title = s.text("Hello UEMotion!", font_size=48)
subtitle = s.text("Real-time Preview", font_size=24, color=GRAY)

s.play(title.write(duration=2))
s.play(subtitle.fade_in(shift=UP))
s.render()
# ✓ 文字清晰可读
# ✓ 打字机效果流畅
# ✓ 支持中英文
```

---

## 不做的事（明确边界）

### ❌ 不追求 Manim 100% 兼容
- 不实现 VMobject 贝塞尔曲线系统（过度复杂）
- 不实现 LaTeX 公式渲染（成本太高，收益有限）
- 不实现 Cairo 级别的 2D 精确控制（UE 是 3D 引擎）

### ❌ 不做在线 IDE
- 本地开发为主（UE Editor 必须本地运行）
- Web Dashboard 只做展示和代码分享

### ❌ 不做移动端支持
- 桌面端 + VR 足够
- 移动端性能和交互差异太大

### ❌ 不做协作编辑
- 单人开发工作流
- 版本控制用 Git 就够了

---

## 资源需求

### 开发资源
- **主要开发者**: 1 人（你）
- **每周投入**: 15-20 小时
- **总工期**: 20 周（约 5 个月）

### 外部依赖
- UE 5.7+ Editor（已有）
- Python 3.10+（已有）
- VS Code（已有）
- 可选：MathJax（LaTeX，Phase 5 再决定）

### 学习资源
- UE Sequencer 官方文档
- ProceduralMeshComponent 教程
- Niagara 粒子系统入门
- UMG Widget 开发指南

---

## 快速启动检查清单

### 开始编码前
- [x] 已阅读本文档并理解路线图
- [ ] 已创建 `config.py` 和 `easing.py` 模块
- [ ] 已为现有代码添加错误处理
- [ ] 已运行完整测试套件确认基线

### Week 1 结束时
- [ ] Config 类可用
- [ ] Easing 模块独立
- [ ] 所有 public API 有 try-catch
- [ ] 测试覆盖率 > 80%

### Month 1 结束时（Phase 0 + Phase 1 部分）
- [ ] Timeline API 可用
- [ ] 能通过代码创建 Sequencer Track
- [ ] 至少 5 个动画预设
- [ ] Smoke test 绿色

### Month 2 结束时（Phase 1 + Phase 2）
- [ ] Timeline 完整功能
- [ ] 8 种 2D 图形
- [ ] VGroup 组合系统
- [ ] 3 个完整示例脚本

### Month 3 结束时（Phase 2 + Phase 3）
- [ ] 文本系统可用
- [ ] 标注系统可用
- [ ] 10+ 示例脚本
- [ ] 第一次公开 Demo

---

## 附录：设计哲学

### 1. Sequencer First
所有动画最终都映射到 Sequencer Track。这意味着：
- 可以在 Editor 中手动调整
- 支持非线性编辑
- 方便团队协作（艺术家调动画，程序员写逻辑）

### 2. Progressive Disclosure
简单事情简单做，复杂事情可以做：
```python
# 简单用法（一行代码）
cube.move_to(UR, duration=2)

# 复杂用法（完全控制）
track = timeline.add_track(cube, "location")
track.add_keyframe(0.0, DL, easing="custom_curve")
track.add_keyframe(2.5, ORIGIN)
track.add_keyframe(5.0, UR, easing="bounce")
track.show_curve_editor()
```

### 3. Embrace UE Strengths
不做 Manim 能做的，要做 Manim **做不到**的：
- ✅ 实时光照变化
- ✅ PBR 材质动画
- ✅ 物理模拟录制
- ✅ 粒子特效
- ✅ VR/AR 输出
- ✅ 电影级后处理

### 4. Fun Factor（好玩！）
```python
# 有趣的预设动画
cube.dance(style="disco", duration=3)      # 迪斯科舞
text.typo_effect(word="UEMotion", fix_to="Unreal")  # 打字纠错
s.screen_shake(intensity=0.2)              # 屏幕震动
s.flashbang(color=WHITE, duration=0.5)     # 闪光弹
```

### 5. Performance by Default
- 默认开启 Instance Rendering
- 自动 LOD（远距离降低精度）
- 智能批处理（合并相同材质的对象）
- 渲染缓存（避免重复计算）

---

**版本**: v2.0
**最后更新**: 2026-05-12
**核心理念**: 用 UE + Sequencer 做一个**好玩**的可视化平台，而不是复刻 Manim
