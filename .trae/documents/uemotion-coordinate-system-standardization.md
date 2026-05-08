# UEMotion 坐标系标准化计划

## 背景

当前基本流程测试 (`run_full_pipeline_test.py`) 已跑通，但存在以下问题：

### 现有问题

1. **Cube 不是从"左下角"出发**

   * 当前 cube 从 `(0, 0, 0)` 开始，沿 y=x 移动到 `(50, 50, 0)`

   * 相机在 `(25, 25, 800)` 俯视 `(25, 25, 0)`

   * 结果：cube 从画面**中心**开始向右上方移动

   * 用户期望：cube 应该从**左下角**出发，体现数学坐标系的感觉

2. **缺少标准化的坐标系定义**

   * 没有明确的帧边界常量（FRAME\_WIDTH, FRAME\_HEIGHT）

   * 没有标准的方向向量（UP, DOWN, LEFT, RIGHT）

   * 没有 ORIGIN 定义

   * 坐标单位不明确（是 UE units？还是抽象单位？）

3. **相机设置不规范**

   * 相机高度、FOV 等参数硬编码

   * 缺少标准化的相机配置方案

***

## Manim 坐标系参考

### 核心设计理念

```
┌─────────────────────────────────┐
│                                 │
│         TOP (0, +4)             │
│            ↑                   │
│     Q2     |      Q1           │
│            |                   │
│   LEFT ←---O---→ RIGHT        │  O = ORIGIN (0,0) 在屏幕中心
│     Q3     |      Q4           │
│            |                   │
│            ↓                   │
│        BOTTOM (0, -4)          │
│                                 │
└─────────────────────────────────┘
```

### Manim 关键常量

| 常量               | 值            | 说明                           |
| ---------------- | ------------ | ---------------------------- |
| `FRAME_HEIGHT`   | 8.0          | 帧高度（manim units）             |
| `FRAME_WIDTH`    | \~14.222     | 帧宽度 = HEIGHT × ASPECT\_RATIO |
| `ASPECT_RATIO`   | 16/9 ≈ 1.778 | 宽高比                          |
| `FRAME_Y_RADIUS` | 4.0          | Y 方向半范围                      |
| `FRAME_X_RADIUS` | \~7.111      | X 方向半范围                      |
| `ORIGIN`         | (0, 0, 0)    | 屏幕中心                         |
| `UP`             | (0, 1, 0)    | Y 正方向                        |
| `RIGHT`          | (1, 0, 0)    | X 正方向                        |

***

## UEMotion 标准化方案（v2 - 1:1 正方形）

### 1. 坐标系定义

#### 基本约定 ⭐ 核心

```
UEMotion 坐标系（2D 平面模式）：
- 默认比例: 1:1 (正方形) → 完美展示四个象限
- 原点 (ORIGIN): 屏幕中心 = (0, 0, 0)
- X 轴: 向右为正 → 符合数学惯例
- Y 轴: 向上为正 → 符合数学惯例
- Z 轴: 垂直屏幕向外（3D 模式时使用）
- 工作平面: XY 平面 (Z=0)，用于 2D 动画

四个象限:
        ↑ Y+
        |
   Q2 (-,+) | Q1 (+,)
←---------O----------→ X+
   Q3 (-,-) | Q4 (+,-)
        ↓ Y-
```

#### 视觉示意（1:1 正方形）

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

正方形画面: 完美对称，X/Y 刻度一致
```

#### 常量定义

```python
# uemotion/constants.py

# === 帧尺寸（UEMotion Units）⭐ 1:1 正方形 ===
FRAME_HEIGHT = 8.0              # 帧高度（抽象单位）
ASPECT_RATIO = 1.0              # ⭐ 1:1 正方形比例
FRAME_WIDTH = FRAME_HEIGHT * ASPECT_RATIO  # = 8.0 (与高度相等)

# 半范围（用于边界计算）
FRAME_Y_RADIUS = FRAME_HEIGHT / 2   # = 4.0
FRAME_X_RADIUS = FRAME_WIDTH / 2    # = 4.0 (⭐ 与 Y 相等)

# === 标准向量 ===
ORIGIN = (0.0, 0.0, 0.0)
UP = (0.0, 1.0, 0.0)
DOWN = (0.0, -1.0, 0.0)
LEFT = (-1.0, 0.0, 0.0)
RIGHT = (1.0, 0.0, 0.0)
IN = (0.0, 0.0, -1.0)    # 进入屏幕
OUT = (0.0, 0.0, 1.0)    # 出屏幕

# === 边界位置（1:1 对称）===
TOP = (0.0, FRAME_Y_RADIUS, 0.0)           # (0, 4, 0)
BOTTOM = (0.0, -FRAME_Y_RADIUS, 0.0)       # (0, -4, 0)
LEFT_SIDE = (-FRAME_X_RADIUS, 0.0, 0.0)    # (-4, 0, 0)  ⭐
RIGHT_SIDE = (FRAME_X_RADIUS, 0.0, 0.0)    # (4, 0, 0)   ⭐

# 角落（1:1 时完全对称）
UL = (-FRAME_X_RADIUS, FRAME_Y_RADIUS, 0.0)   # (-4, 4, 0)   左上 (Q2)
UR = (FRAME_X_RADIUS, FRAME_Y_RADIUS, 0.0)    # (4, 4, 0)    右上 (Q1)
DL = (-FRAME_X_RADIUS, -FRAME_Y_RADIUS, 0.0)  # (-4, -4, 0)  左下 (Q3)
DR = (FRAME_X_RADIUS, -FRAME_Y_RADIUS, 0.0)   # (4, -4, 0)   右下 (Q4)

# === 间距常量（Buff）===
SMALL_BUFF = 0.1
MED_SMALL_BUFF = 0.25
DEFAULT_MOBJECT_TO_EDGE_BUFF = 0.5
DEFAULT_MOBJECT_TO_MOBJECT_BUFF = 0.25

# === 默认渲染配置 ===
DEFAULT_PIXEL_WIDTH = 1080          # ⭐ 正方形分辨率
DEFAULT_PIXEL_HEIGHT = 1080         # ⭐ 正方形分辨率
DEFAULT_FPS = 30
DEFAULT_CAMERA_Z = 10.0             # 相机 Z 高度（正交/近似正交）
SCALE_FACTOR = 50.0                 # UEMotion Unit → UE cm 缩放因子
```

### 2. UE Units 与 UEMotion Units 映射

#### 映射策略

由于 UE 使用厘米为单位，而我们需要一个更直观的坐标系：

```python
# 缩放因子：1 UEMotion Unit = SCALE_FACTOR UE Units (cm)
SCALE_FACTOR = 50.0  # 可调整

def ue_to_motion(x):
    """UE Units → UEMotion Units"""
    return x / SCALE_FACTOR

def motion_to_ue(x):
    """UEMotion Units → UE Units"""
    return x * SCALE_FACTOR
```

#### 1:1 映射表示例

| UEMotion 坐标       | UE 坐标 (cm)      | 象限 | 说明      |
| ----------------- | --------------- | -- | ------- |
| (0, 0, 0)         | (0, 0, 0)       | 中心 | 原点/屏幕中心 |
| (4, 4, 0)         | (200, 200, 0)   | Q1 | 右上角     |
| (-4, 4, 0)        | (-200, 200, 0)  | Q2 | 左上角     |
| (-4, -4, 0)       | (-200, -200, 0) | Q3 | 左下角     |
| (4, -4, 0)        | (200, -200, 0)  | Q4 | 右下角     |
| (0, CAMERA\_Z, 0) | (0, 500, 0)     | -  | 相机位置    |

### 3. Scene 类增强

#### 新增属性和方法

```python
class Scene:
    def __init__(self, name="default", width=1080, height=1080):  # ⭐ 默认正方形
        # ...existing code...
        
        # 新增：坐标系配置
        self._frame_width = FRAME_WIDTH       # 8.0
        self._frame_height = FRAME_HEIGHT     # 8.0
        self._scale_factor = SCALE_FACTOR     # 50.0
        
        # 自动配置相机为标准 2D 俯视
        self._setup_standard_2d_camera()
    
    @property
    def frame_width(self):
        """帧宽度（UEMotion units）"""
        return self._frame_width
    
    @property
    def frame_height(self):
        """帧高度（UEMotion units）"""
        return self._frame_height
    
    @property
    def center(self):
        """场景中心点"""
        return ORIGIN
    
    def get_corner(self, corner_name):
        """获取角落位置: 'UL', 'UR', 'DL', 'DR'"""
        corners = {'UL': UL, 'UR': UR, 'DL': DL, 'DR': DR}
        return corners.get(corner_name.upper(), ORIGIN)
    
    def _setup_standard_2d_camera(self):
        """配置标准 2D 俯视相机（1:1 正交视角）"""
        cam_z = DEFAULT_CAMERA_Z * self._scale_factor  # 500 cm
        self.camera.position = (0, 0, cam_z)
        self.camera.look_at((0, 0, 0))
        # 可选：设置正交或近似正交的 FOV
```

### 4. Mobject 类增强

#### 便捷定位方法

```python
class Mobject:
    # ...existing methods...
    
    def move_to(self, target, duration=1.0, easing="ease_in_out"):
        # 已有，保持不变
        pass
    
    def shift(self, vector, duration=1.0, easing="linear"):
        """相对移动（偏移）"""
        current = self.location
        target = (
            current.x + vector[0],
            current.y + vector[1],
            current.z + vector[2] if len(vector) > 2 else current.z
        )
        return self.move_to(target, duration, easing)
    
    def to_edge(self, edge, buff=DEFAULT_MOBJECT_TO_EDGE_BUFF):
        """移动到边缘
        
        Args:
            edge: 'UP', 'DOWN', 'LEFT', 'RIGHT', 'UL', 'UR', 'DL', 'DR'
            buff: 与边缘的距离
        """
        edges = {
            'UP': TOP,
            'DOWN': BOTTOM,
            'LEFT': LEFT_SIDE,
            'RIGHT': RIGHT_SIDE,
            'UL': UL,
            'UR': UR,
            'DL': DL,
            'DR': DR,
        }
        target = edges.get(edge.upper(), ORIGIN)
        
        # 保持另一轴不变（轴向移动时）
        if edge.upper() in ['UP', 'DOWN']:
            target = (self.location.x, target[1], 0)
        elif edge.upper() in ['LEFT', 'RIGHT']:
            target = (target[0], self.location.y, 0)
        
        # 应用 buff（向内缩进）
        # ...buff adjustment logic...
        
        return self.move_to(target)
    
    def next_to(self, other, direction=RIGHT, buff=DEFAULT_MOBJECT_TO_MOBJECT_BUFF):
        """放置在另一个对象旁边"""
        other_pos = other.location
        dir_vec = {
            'UP': UP, 'DOWN': DOWN,
            'LEFT': LEFT, 'RIGHT': RIGHT
        }.get(direction.upper(), RIGHT)
        target = (
            other_pos.x + dir_vec[0] * buff,
            other_pos.y + dir_vec[1] * buff,
            0
        )
        return self.move_to(target)
    
    def align_to(self, other_or_edge, direction=UP):
        """对齐到另一个对象或边缘"""
        pass
```

### 5. 更新 Smoke Test（1:1 四象限版）

#### 修改后的测试参数

```python
# run_full_pipeline_test.py (v2 - 1:1 四象限版)

from uemotion.constants import *

SCENE_NAME = "y_equals_x"
RESOLUTION_W = DEFAULT_PIXEL_WIDTH   # 1080 ⭐
RESOLUTION_H = DEFAULT_PIXEL_HEIGHT  # 1080 ⭐

# 动画参数
NUM_STEPS = 5
STEP_SIZE = 1.0  # 使用 UEMotion Units
MOVE_DURATION = 1.0
FPS = DEFAULT_FPS  # 30

# Cube 参数
CUBE_SIZE = 0.5   # UEMotion Units (会转换为 UE Units = 25cm)
CUBE_COLOR = "cyan"

# 路径：从第三象限 (Q3) 到第一象限 (Q1)，穿过原点
START_POS = DL           # 左下角 Q3: (-4, -4, 0)
END_POS = UR             # 右上角 Q1: (4, 4, 0)

TOTAL_DURATION = NUM_STEPS * MOVE_DURATION
EXPECTED_FRAMES = int(TOTAL_DURATION * FPS) + 1  # 151
```

#### 测试场景描述（四象限可视化）

```
更新后的场景（1:1 正方形画面）：

Cube 运动轨迹: DL (-4,-4) → ORIGIN (0,0) → UR (4,4)

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

视觉效果：
✓ 完整展示四个象限
✓ Cube 从左下角出发（符合数学直觉）
✓ 沿 y=x 直线穿过原点到右上角
✓ 1:1 比例确保 X/Y 轴刻度一致
✓ 数学坐标系的完美映射
```

#### 验证检查项更新

```python
# 验证项
check(s is not None, "Scene object created")
check(s.frame_width == FRAME_WIDTH, f"Frame width = {FRAME_WIDTH} (1:1)")
check(s.frame_height == FRAME_HEIGHT, f"Frame height = {FRAME_HEIGHT} (1:1)")
check(abs(s.camera.position.z - DEFAULT_CAMERA_Z * SCALE_FACTOR) < 0.01,
      f"Camera at standard Z = {DEFAULT_CAMERA_Z * SCALE_FACTOR}")

# Cube 位置验证
box = s.cube(CUBE_SIZE, color=CUBE_COLOR, location=START_POS)
check(box is not None, f"Cube created at {START_POS} (DL/Q3)")

# 动画：沿 y=x 从 Q3 到 Q1
for i in range(1, NUM_STEPS + 1):
    t = i / NUM_STEPS  # 0.2, 0.4, 0.6, 0.8, 1.0
    target_x = START_POS[0] + t * (END_POS[0] - START_POS[0])  # -4 → 4
    target_y = START_POS[1] + t * (END_POS[1] - START_POS[1])  # -4 → 4
    box.move_to((target_x, target_y, 0), duration=MOVE_DURATION, easing="linear")
    s.play()

final_pos = box.location
check(abs(final_pos.x - END_POS[0]) < 0.01 and abs(final_pos.y - END_POS[1]) < 0.01,
      f"Cube reached {END_POS} (UR/Q1)")
```

***

## 实施步骤

### Step 1: 创建 constants.py 文件

**文件**: `Scripts/uemotion/constants.py` （新建）

**内容**:

* 所有坐标系常量（FRAME\_WIDTH=8.0, FRAME\_HEIGHT=8.0, 1:1 比例）

* 标准向量（ORIGIN, UP, DOWN, LEFT, RIGHT, UL/UR/DL/DR）

* 缩放因子 (SCALE\_FACTOR=50.0)

* 间距常量 (SMALL\_BUFF, etc.)

* 默认配置值（1080x1080, FPS=30）

### Step 2: 更新 __init__.py 导出

**文件**: `Scripts/uemotion/__init__.py`

**修改**:

* 导出 constants 模块中的关键符号

* 方便用户 `from uemotion import *` 使用

### Step 3: 更新 Scene 类

**文件**: `Scripts/uemotion/scene.py`

**修改**:

* 导入 constants

* 默认分辨率改为 1080x1080

* 添加 frame\_width/frame\_height 属性

* 添加 center 属性

* 添加 get\_corner() 方法

* 实现 `_setup_standard_2d_camera()` 方法

* 在 `__init__` 中自动调用相机设置

### Step 4: 增强 Mobject 类

**文件**: `Scripts/uemotion/mobject.py`

**修改**:

* 导入 constants

* 添加 `shift()` 方法 - 相对移动

* 添加 `to_edge()` 方法 - 移动到边缘

* 添加 `next_to()` 方法 - 放在对象旁边

* 添加 `align_to()` 方法 - 对齐

### Step 5: 更新 Camera 类（可选增强）

**文件**: `Scripts/uemotion/camera.py`

**修改**:

* 导入 constants

* 添加标准 2D 配置辅助方法

* 文档说明 FOV 设置建议

### Step 6: 重写 Smoke Test

**文件**: `Scripts/run_full_pipeline_test.py`

**修改**:

* 导入新的 constants

* 分辨率改为 1080x1080（1:1）

* Cube 起点: DL (-4, -4, 0) \[Q3 左下角]

* Cube 终点: UR (4, 4, 0) \[Q1 右上角]

* 路径: y=x 直线穿过原点

* 更新所有验证逻辑

* 添加坐标系验证检查项

### Step 7: 更新 AGENTS.md 文档

**文件**: `AGENTS.md`

**修改**:

* 添加坐标系规范章节

* 更新测试参数表（1:1 版本）

* 添加 API 快速参考

* 添加四象限示意图

### Step 8: 验证测试

**操作**:

* 运行 `run_full_pipeline_test.bat`

* 验证输出为 1080x1080 正方形帧

* 验证 151 帧输出正确

* 验证 cube 位置轨迹: Q3 → Origin → Q1

* 验证四个象限都可见且正确

***

## 文件变更清单

| 文件                                  | 操作     | 优先级 | 说明            |
| ----------------------------------- | ------ | --- | ------------- |
| `Scripts/uemotion/constants.py`     | **新建** | P0  | 坐标系常量定义（核心）   |
| `Scripts/uemotion/__init__.py`      | 修改     | P0  | 导出新符号         |
| `Scripts/uemotion/scene.py`         | 修改     | P0  | Scene 类增强     |
| `Scripts/uemotion/mobject.py`       | 修改     | P1  | Mobject 定位方法  |
| `Scripts/uemotion/camera.py`        | 修改     | P2  | Camera 辅助方法   |
| `Scripts/run_full_pipeline_test.py` | 修改     | P0  | Smoke Test 重写 |
| `AGENTS.md`                         | 修改     | P1  | 文档更新          |

***

## 预期效果

### 标准化后的用户体验

```python
from uemotion import Scene
from uemotion.constants import *

# 创建场景（自动 1:1 正方形 + 标准 2D 相机）
s = Scene("four_quadrants")  # 默认 1080x1080

# ✨ 清晰的四象限操作
box = s.cube(size=0.5, color="cyan")

# 方式1: 使用预定义角落常量
box.move_to(DL)                      # 放到左下角 (Q3)
box.move_to(ORIGIN, duration=2)      # 动画到中心
box.move_to(UR, duration=2)          # 动画到右上角 (Q1)

# 方式2: 使用便捷方法
circle = s.sphere(radius=0.3)
circle.to_edge(UR)                   # 放到右上角

text_box = s.cube(size=0.3)
text_box.next_to(circle, DOWN)       # 在 circle 下方

# 方式3: 直接坐标（数学直觉）
point = s.sphere(radius=0.2, location=(2, 3, 0))  # Q1 的某位置
point.shift(LEFT * 2)                # 向左移动 2 单位

# 语义化动画
s.play(
    box.move_to(ORIGIN),             # 向中心移动
    circle.to_edge(RIGHT_SIDE),      # 到右边
)

# 渲染（自动 1:1 正方形输出）
s.render("output/four_quad", duration=5, fps=30)
```

### 坐标系对比

| 特性          | 当前实现             | 标准化后 (v2)                             |
| ----------- | ---------------- | ------------------------------------- |
| **分辨率**     | 1920×1080 (16:9) | **1080×1080 (1:1)** ⭐                 |
| **原点**      | UE 世界原点 (0,0,0)  | 屏幕中心 (0,0,0)                          |
| **Cube 起点** | (0,0,0) = 画面中心   | **DL (-4,-4,0) \[Q3]** ⭐              |
| **Y 轴方向**   | UE 默认            | 向上（数学惯例）                              |
| **帧形状**     | 无定义              | **正方形 8×8** ⭐                         |
| **可见象限**    | 不明确              | **全部四个象限** ⭐                          |
| **X/Y 一致性** | 不保证              | **完全一致** ⭐                            |
| 定位 API      | 仅 move\_to()     | move\_to/to\_edge/next\_to/shift      |
| 方向向量        | 无                | UP/DOWN/LEFT/RIGHT/ORIGIN/UL/UR/DL/DR |

***

## 1:1 正方形的优势

### 为什么选择 1:1？

```
16:9 (宽屏)          vs          1:1 (正方形)
┌───────────────┐               ┌───────────┐
│               │               │           │
│               │               │           │
│       ·       │               │    ·      │
│               │               │           │
└───────────────┘               └───────────┘
  Y 被压缩                        X/Y 完全等价
  
问题:                          优势:
• Y 轴看起来被压缩              ✓ 数学直觉完整保留
• 圆形变成椭圆                  ✓ 斜率直观正确
• 角度失真                      ✓ 四个象限同等展示
• 不适合数学绘图                ✓ 坐标网格是正方形
```

### 适用场景

✅ **适合 1:1 的场景**:

* 数学函数图像 (y=x, y=x², sin(x))

* 几何图形 (圆、三角形、多边形)

* 向量运算演示

* 坐标变换教学

* 极坐标/参数方程

* 分形图案

❌ **可能需要其他比例的场景**:

* 电影级视频输出 (16:9)

* 展示宽幅内容 (时间线、谱图)

* 特定 UI 布局需求

> 注：未来可扩展支持自定义 ASPECT\_RATIO，但 **1:1 作为默认值**

***

## 风险评估与缓解

| 风险         | 影响        | 缓解措施                  |
| ---------- | --------- | --------------------- |
| 缩放因子选择不当   | 对象大小不符合预期 | 提供默认值 50.0，允许自定义      |
| 1:1 分辨率非传统 | 用户期望 16:9 | 文档清晰说明理由；支持自定义        |
| 破坏现有测试     | 回归问题      | 旧 API 保持兼容；新增为扩展      |
| UE 坐标转换误差  | 位置偏移      | 充分测试转换函数；单元验证         |
| 性能开销       | 每次访问都转换   | 缓存常用值；lazy evaluation |

***

## 后续扩展（可选 Phase 2）

1. **自定义比例支持**: 允许 `Scene(aspect_ratio=16/9)`
2. **坐标网格辅助**: 可选显示坐标轴和网格线
3. **3D 模式扩展**: 完整的 3D 坐标操作
4. **Manim API 兼容**: 提供 `manim_compat` 模块
5. **配置文件支持**: `.uemotion_config.yml` 自定义默认值

