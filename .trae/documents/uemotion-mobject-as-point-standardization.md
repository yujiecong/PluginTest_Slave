# UEMotion 标准化改进方案：让 Mobject 像 Manim 一样"可视为点"

## 问题诊断

当前 UEMotion 存在三个核心问题，导致 Cube 等 3D 对象无法像 Manim 那样方便地当作"点"来操作：

### 问题 1：坐标系断层

* `constants.py` 定义了 UEMotion 坐标系（DL=(-4,-4,0)，1单位=50cm）

* 但 `move_to`、`shift`、`location` 直接将值传给 UE Actor，**没有做 motion\_to\_ue 转换**

* Smoke test 中 `DL=(-4,-4,0)` 被当作 UE 厘米坐标设置，实际只偏移了 4cm 而非 200cm

* `motion_to_ue()` 和 `ue_to_motion()` 已定义但从未在关键路径上使用

### 问题 2：无包围盒感知

* `to_edge(RIGHT)` 把 pivot 移到右边缘，但大 Cube 会有一半超出画面

* `next_to(other, RIGHT)` 只在 other 的 pivot 右边加 buff，不考虑 other 的右边界

* Manim 的核心设计：`get_bounding_box_point(direction)` 让对象的任意关键点都能作为"锚点"

### 问题 3：方向向量不支持运算

* `UP * 2` 会报错（tuple 不支持乘法）

* Manim 中 `shift(UP * 2 + RIGHT * 3)` 是常用写法

* `next_to` 只支持 4 个方向字符串，不支持任意方向向量

***

## 改进方案（4 层架构）

### 第 1 层：向量类 — 替代 tuple，支持算术运算

**文件**: `Scripts/uemotion/vector.py`（新建）

创建 `MVector` 类替代 tuple 作为方向/位置常量：

```python
class MVector:
    """支持算术运算的向量类，兼容 tuple 接口"""

    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x, self.y, self.z = float(x), float(y), float(z)

    def __mul__(self, scalar):      # UP * 2 → MVector(0, 2, 0)
    def __rmul__(self, scalar):     # 2 * UP → MVector(0, 2, 0)
    def __add__(self, other):       # UP * 2 + RIGHT * 3
    def __sub__(self, other):       # UR - ORIGIN
    def __neg__(self):              # -RIGHT → LEFT
    def __iter__(self):             # 兼容 tuple 解包: x, y, z = vec
    def __getitem__(self, idx):     # 兼容索引: vec[0], vec[1], vec[2]
    def __len__(self):              # len() == 3
    def __eq__(self, other):        # 与 tuple/MVector 比较
    def to_tuple(self):             # 转为 tuple
    def to_ue_vector(self):         # 转为 unreal.Vector（UE 厘米）
```

**修改**: `Scripts/uemotion/constants.py`

* 将所有方向常量从 tuple 改为 MVector 实例

* `ORIGIN = MVector(0, 0, 0)`, `UP = MVector(0, 1, 0)`, `DL = MVector(-4, -4, 0)` 等

* 保持 `__all__` 导出不变，API 向后兼容

**影响范围**: 所有使用 `UP * 2`、`LEFT + DOWN` 等表达式的代码将正常工作；现有 tuple 解包代码通过 `__iter__` 兼容。

***

### 第 2 层：坐标转换层 — 统一 UEMotion 单位

**修改**: `Scripts/uemotion/mobject.py`

核心原则：**Python API 层全部使用 UEMotion 单位，与 UE 通信时自动转换**。

```python
class Mobject:
    @property
    def location(self):
        ue_loc = self._ue.get_location()
        return MVector(
            ue_to_motion(ue_loc.x),
            ue_to_motion(ue_loc.y),
            ue_to_motion(ue_loc.z)
        )

    @location.setter
    def location(self, value):
        v = _to_mvector(value)
        self._ue.set_location(vec(
            motion_to_ue(v.x),
            motion_to_ue(v.y),
            motion_to_ue(v.z)
        ))

    def move_to(self, target, aligned_edge=ORIGIN, duration=1.0, easing="ease_in_out"):
        target_v = self._resolve_target_point(target, aligned_edge)
        current_v = self.get_bounding_box_point(aligned_edge)
        shift_v = target_v - current_v
        return self.shift(shift_v, duration=duration, easing=easing)

    def shift(self, vector, duration=1.0, easing="linear"):
        v = _to_mvector(vector)
        current = self.location
        target = current + v
        ue_target = vec(motion_to_ue(target.x), motion_to_ue(target.y), motion_to_ue(target.z))
        anim = unreal.UEMotionMoveAnimation()
        anim.set_target_mobject(self._ue)
        anim.set_target(ue_target)
        anim.set_duration(duration)
        anim.set_easing(easing)
        self._scene._pending_animations.append(anim)
        return self
```

**关键变化**:

* `location` getter 返回 UEMotion 单位的 MVector

* `location` setter 自动 `motion_to_ue` 转换

* `move_to` 接受 UEMotion 单位坐标

* `shift` 接受 UEMotion 单位偏移量

* 新增 `aligned_edge` 参数

**修改**: `Scripts/uemotion/scene.py`

* `from_config` / `from_asset` 中设置 location 时自动转换

* `point_light` 的 location 参数自动转换

* `_setup_standard_2d_camera` 已正确使用 `SCALE_FACTOR`，无需修改

***

### 第 3 层：包围盒系统 — 让对象"可视为点"

#### 3A. C++ 层：新增 GetBounds 方法

**修改**: `UEMotionMobject.h` / `UEMotionMobject.cpp`

```cpp
// UEMotionMobject.h 新增：
UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
void GetBounds(FVector& OutOrigin, FVector& OutBoxExtent) const;
```

```cpp
// UEMotionMobject.cpp 新增：
void UUEMotionMobject::GetBounds(FVector& OutOrigin, FVector& OutBoxExtent) const
{
    if (InternalActor.IsValid())
    {
        InternalActor->GetActorBounds(false, OutOrigin, OutBoxExtent);
    }
    else
    {
        OutOrigin = FVector::ZeroVector;
        OutBoxExtent = FVector::ZeroVector;
    }
}
```

#### 3B. Python 层：包围盒与锚点 API

**修改**: `Scripts/uemotion/mobject.py`

```python
class Mobject:
    def get_bounding_box(self):
        """返回 UEMotion 单位的包围盒 {min, mid, max}"""
        origin, extent = self._ue.get_bounds()
        center = MVector(ue_to_motion(origin.x), ue_to_motion(origin.y), ue_to_motion(origin.z))
        half = MVector(ue_to_motion(extent.x), ue_to_motion(extent.y), ue_to_motion(extent.z))
        return {
            'min': center - half,
            'mid': center,
            'max': center + half,
        }

    def get_bounding_box_point(self, direction):
        """Manim 风格锚点：direction 为方向向量，返回包围盒上的对应点"""
        bb = self.get_bounding_box()
        d = _to_mvector(direction)
        return MVector(
            bb['min'].x * (1 - d.x) / 2 + bb['max'].x * (1 + d.x) / 2,
            bb['min'].y * (1 - d.y) / 2 + bb['max'].y * (1 + d.y) / 2,
            bb['min'].z * (1 - d.z) / 2 + bb['max'].z * (1 + d.z) / 2,
        )

    def get_center(self):
        return self.get_bounding_box_point(ORIGIN)

    def get_top(self):
        return self.get_bounding_box_point(UP)

    def get_bottom(self):
        return self.get_bounding_box_point(DOWN)

    def get_left(self):
        return self.get_bounding_box_point(LEFT)

    def get_right(self):
        return self.get_bounding_box_point(RIGHT)

    def get_corner(self, direction):
        return self.get_bounding_box_point(direction)

    @property
    def width(self):
        bb = self.get_bounding_box()
        return bb['max'].x - bb['min'].x

    @property
    def height(self):
        bb = self.get_bounding_box()
        return bb['max'].y - bb['min'].y
```

***

### 第 4 层：升级定位方法 — 包围盒感知

**修改**: `Scripts/uemotion/mobject.py`

#### 4A. move\_to 升级

```python
def move_to(self, target, aligned_edge=ORIGIN, duration=1.0, easing="ease_in_out"):
    if isinstance(target, Mobject):
        target_point = target.get_bounding_box_point(aligned_edge)
    else:
        target_point = _to_mvector(target)

    current_point = self.get_bounding_box_point(aligned_edge)
    shift_vec = target_point - current_point
    return self.shift(shift_vec, duration=duration, easing=easing)
```

#### 4B. next\_to 升级

```python
def next_to(self, other, direction=RIGHT, buff=DEFAULT_MOBJECT_TO_MOBJECT_BUFF, aligned_edge=ORIGIN):
    d = _to_mvector(direction)
    if isinstance(other, Mobject):
        target_anchor = other.get_bounding_box_point(d)
    else:
        target_anchor = _to_mvector(other)

    neg_d = -d
    self_anchor = self.get_bounding_box_point(neg_d)
    shift_vec = target_anchor - self_anchor + d * buff
    return self.shift(shift_vec)
```

**效果对比**:

* 旧：`next_to(big_cube, RIGHT)` → 小方块移到 big\_cube 中心右边 0.25 单位

* 新：`next_to(big_cube, RIGHT)` → 小方块移到 big\_cube 右边界右边 0.25 单位

#### 4C. to\_edge 升级

```python
def to_edge(self, edge, buff=DEFAULT_MOBJECT_TO_EDGE_BUFF):
    d = _to_mvector(edge)
    frame_edge_point = self._scene.get_bounding_box_point(d)
    self_anchor = self.get_bounding_box_point(d)
    neg_d = -d
    shift_vec = frame_edge_point - self_anchor + neg_d * buff
    return self.shift(shift_vec)
```

**效果对比**:

* 旧：`to_edge(RIGHT)` → pivot 在右边缘，大 Cube 一半超出画面

* 新：`to_edge(RIGHT)` → Cube 右边界贴着画面右边缘（减去 buff）

#### 4D. align\_to 实现

```python
def align_to(self, other_or_edge, direction=UP):
    d = _to_mvector(direction)
    if isinstance(other_or_edge, Mobject):
        target = other_or_edge.get_bounding_box_point(d)
    elif isinstance(other_or_edge, str):
        target = self._scene.get_corner(other_or_edge)
        target = MVector(*target)
    else:
        target = _to_mvector(other_or_edge)

    current = self.get_bounding_box_point(d)
    shift_vec = target - current
    shift_vec = MVector(shift_vec.x * abs(d.x), shift_vec.y * abs(d.y), shift_vec.z * abs(d.z))
    return self.shift(shift_vec)
```

***

## Scene 层升级

**修改**: `Scripts/uemotion/scene.py`

Scene 也需要包围盒支持（代表画面帧）：

```python
class Scene:
    def get_bounding_box(self):
        return {
            'min': MVector(-FRAME_X_RADIUS, -FRAME_Y_RADIUS, 0),
            'mid': MVector(0, 0, 0),
            'max': MVector(FRAME_X_RADIUS, FRAME_Y_RADIUS, 0),
        }

    def get_bounding_box_point(self, direction):
        bb = self.get_bounding_box()
        d = _to_mvector(direction)
        return MVector(
            bb['min'].x * (1 - d.x) / 2 + bb['max'].x * (1 + d.x) / 2,
            bb['min'].y * (1 - d.y) / 2 + bb['max'].y * (1 + d.y) / 2,
            bb['min'].z * (1 - d.z) / 2 + bb['max'].z * (1 + d.z) / 2,
        )
```

***

## 辅助函数

**修改**: `Scripts/uemotion/mobject.py` 顶部

```python
def _to_mvector(value):
    """将 tuple/list/MVector/unreal.Vector 统一转为 MVector"""
    if isinstance(value, MVector):
        return value
    if isinstance(value, (list, tuple)):
        return MVector(*value)
    if hasattr(value, 'x') and hasattr(value, 'y'):
        return MVector(value.x, value.y, getattr(value, 'z', 0))
    return MVector(value, 0, 0)
```

***

## 实施步骤

| 步骤 | 内容                           | 文件                          | 风险          |
| -- | ---------------------------- | --------------------------- | ----------- |
| 1  | 创建 MVector 类                 | `vector.py`（新建）             | 低           |
| 2  | 更新 constants.py 常量为 MVector  | `constants.py`              | 中（需验证兼容性）   |
| 3  | 更新 __init__.py 导出 MVector    | `__init__.py`               | 低           |
| 4  | C++ 层新增 GetBounds 方法         | `UEMotionMobject.h/cpp`     | 中（需重新编译插件）  |
| 5  | Python 层新增包围盒 API            | `mobject.py`                | 低           |
| 6  | 升级 move\_to（含 aligned\_edge） | `mobject.py`                | 中（API 变更）   |
| 7  | 升级 shift（坐标转换）               | `mobject.py`                | 中           |
| 8  | 升级 next\_to（包围盒感知）           | `mobject.py`                | 中           |
| 9  | 升级 to\_edge（包围盒感知）           | `mobject.py`                | 中           |
| 10 | 实现 align\_to                 | `mobject.py`                | 低           |
| 11 | Scene 层新增包围盒方法               | `scene.py`                  | 低           |
| 12 | 修复 location 属性坐标转换           | `mobject.py`                | 高（影响所有现有代码） |
| 13 | 修复 scene.py 中 location 设置    | `scene.py`                  | 中           |
| 14 | 更新 smoke test                | `run_full_pipeline_test.py` | 中           |
| 15 | 更新示例脚本                       | `examples/`                 | 低           |

***

## 向后兼容策略

由于坐标转换是**破坏性变更**（现有代码中的 UE 厘米值会变成 UEMotion 单位），采用以下策略：

1. **MVector 完全兼容 tuple**：通过 `__iter__`、`__getitem__`、`__len__` 支持解包和索引
2. **分阶段迁移**：先添加新 API（包围盒、aligned\_edge），后切换坐标转换
3. **提供兼容函数**：`raw_location` 属性直接返回 UE 厘米值，供需要直接操作 UE 的场景使用
4. **更新文档和示例**：所有示例统一使用 UEMotion 单位

***

## 预期效果

改进后的 API 使用体验：

```python
from uemotion import Scene, ORIGIN, UP, DOWN, LEFT, RIGHT, UR, DL

s = Scene("demo")

# 创建对象 — size 和 location 都是 UEMotion 单位
box = s.cube(size=0.5, color="cyan", location=DL)   # 0.5 UEMotion 单位边长，位于 Q3
small = s.cube(size=0.2, color="red", location=ORIGIN)

# 向量运算 — 自然表达
box.move_to(UR, duration=2)                           # 从 DL 动画到 Q1
box.shift(LEFT * 2 + UP, duration=1)                  # 左移2单位+上移1单位

# 包围盒感知布局 — 不会重叠
small.next_to(box, RIGHT, buff=0.3)                   # 自动贴着 box 右边界
box.to_edge(LEFT)                                     # box 左边界贴画面左边缘

# 锚点对齐 — 精确控制
box.move_to(ORIGIN, aligned_edge=DL)                  # box 左下角对齐原点

# 查询包围盒
print(box.width, box.height)                          # 0.5, 0.5
print(box.get_corner(UR))                             # 右上角坐标
print(box.get_center())                               # 中心坐标
```

