# 实现类似 Manim `Create` 动画的示例 - 圆形绘制效果

## 📋 目标

研究并实现类似 Manim `Create(circle)` 的**圆形绘制动画效果**，展示如何用 UEMotion 现有能力组合出"手绘/生长"动画。

## 🔍 技术分析

### Manim `Create` 动画的原理

```python
# Manim 内部实现（简化）
class Create(ShowPartial):
    def interpolate_mobject(self, alpha):
        # alpha: 0→1 表示动画进度
        # 逐帧截取路径的前 alpha% 部分
        for mob in self.mobject.family_members_with_points():
            mob.pointwise_become_partial(mob, 0, alpha)
```

**核心机制**：圆形由数百个 Bezier 控制点组成路径 → 每帧显示前 N% 个点 → 视觉上像"被画出来"

### UEMotion 现有能力 vs 差距

| 能力 | UEMotion 支持 | 说明 |
|------|--------------|------|
| 圆形/圆环形状 | ✅ `torus()` | 可用作 2D 圆环 |
| Scale 动画 | ✅ `scale_to()` | 控制大小 |
| FadeIn 动画 | ✅ `fade_in()` | 控制透明度 |
| 路径截取 | ❌ 不支持 | Manim 的核心技术 |
| 材质参数动画 | ⚠️ 有限 | 仅支持 Opacity |

### 实现方案选择

#### 方案 A：**Scale + FadeIn 组合**（推荐用于当前示例）

```
效果：圆环从中心"生长"出来 + 同步淡入
视觉：类似 GrowFromCenter + FadeIn 的组合
优点：无需修改 C++ 插件，纯 Python 实现
缺点：不是真正的"描边"效果，而是整体缩放
```

**实现步骤**：
1. 使用 `s.torus()` 创建圆环（扁平化视角下看起来像圆形）
2. 设置初始 `scale = 0` 和 `opacity = 0`
3. 并行执行 `scale_to(1.0)` + `fade_in()`
4. 可选：添加轻微旋转增加动感

#### 方案 B：**多段弧形拼接**（高级，需要 C++ 扩展）

```
效果：真正的"描边"动画，从起点沿圆周绘制到终点
优点：100% 还原 Manim Create 效果
缺点：需要 C++ 层支持 Arc 几何体或 Material Progress 参数
复杂度：高（需自定义 Shader/Material）
```

**未来扩展方向**：
- 在 C++ 层添加 `UEMotionDrawAnimation`
- 使用 Material 的 `Discard` + `Angle` 参数实现像素级进度控制
- 或使用 Procedural Mesh 动态生成弧形片段

## 🎯 本示例采用方案 A

### 示例设计：`circle_create_demo.py`

#### 场景描述

**标题**: "Circle Creation Animation (Manim-style)"
**分辨率**: 1920x1080 (16:9) 或 1080x1080 (1:1)
**模式**: 2D（俯视相机）

#### 动画序列

```
时间轴:
[0.0s] ── 初始化场景，创建粉色圆环（隐藏状态）
         circle = s.torus(radius=2.0, color=PINK, location=ORIGIN)
         circle.scale = 0      # 初始不可见（太小）
         circle.opacity = 0    # 完全透明

[0.0s - 3.0s] ── Phase 1: Create 动画（核心！）
         ┌──────────────────────────────────────┐
         │  circle.scale_to(1.0, duration=3)   │  ← 从中心生长
         │  circle.fade_in(duration=3)          │  ← 同步淡入
         │  circle.rotate(180, duration=3)      │  ← 轻微旋转（可选）
         └──────────────────────────────────────┘
         视觉效果：圆环从中心点逐渐放大并显现，类似"被画出"

[3.0s - 4.5s] ── Phase 2: 颜色变化（展示其他动画能力）
         circle.change_color(CYAN, duration=1.5)
         粉色 → 青色渐变

[4.5s - 6.0s] ── Phase 3: 移动到边缘
         circle.move_to(RIGHT * 3, duration=1.5)

[6.0s - 7.5s] ── Phase 4: 淡出消失
         circle.fade_out(duration=1.5)

[7.5s] ── 渲染完成
```

#### 视觉效果示意

```
初始 (t=0)        生长中 (t=1.5s)     完成 (t=3s)       变色 (t=4.5s)
    ○                  ◐                 ●               ●(青色)
   (几乎不可见)       (半圆)           (完整粉圆)        (移动中)
                                              ↓
                                         消失 (t=7.5s)
                                            ○
```

## 📝 实现细节

### 文件位置
`c:\Users\42458\Documents\Unreal Projects\PluginTest\Scripts\examples\circle_create_demo.py`

### 代码结构

```python
import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, RIGHT, UP, MVector

def main():
    # === 配置 ===
    SCENE_NAME = "circle_create_demo"
    RESOLUTION = (1920, 1080)  # 或 (1080, 1080) 用于 1:1 正方形
    MODE = "2d"
    
    # 圆环参数（UEMotion Units）
    CIRCLE_RADIUS = 2.0        # 半径（motion units）
    CIRCLE_COLOR = "PINK"      # 初始颜色（Manim 风格）
    CREATE_DURATION = 3.0      # Create 动画时长
    
    # === 创建场景 ===
    s = Scene(SCENE_NAME, *RESOLUTION, mode=MODE)
    s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
    s.point_light(location=ORIGIN, color="white", intensity=3000)
    
    # === 创建圆环（Torus 作为 2D 圆形）===
    # Torus 参数说明：
    # - outer_radius: 圆环的外半径（决定圆的大小）
    # - inner_radius: 圆环管的粗细（设小一些看起来更像线条）
    circle = s.torus(
        outer_radius=CIRCLE_RADIUS,
        inner_radius=0.08,  # 细管，看起来像圆线而非粗环
        color=CIRCLE_COLOR,
        location=ORIGIN
    )
    
    if not circle:
        print("[ERROR] Failed to create circle!")
        return
    
    # === 设置初始状态（隐藏）===
    circle.scale = 0.001  # 接近零但不为零（避免数值问题）
    circle.opacity = 0.0  # 完全透明
    
    # === Phase 1: Create 动画（核心！）===
    # 模拟 Manim 的 Create(circle) 效果
    # 组合：Scale（生长）+ FadeIn（显现）+ Rotate（可选动感）
    s.play()
    
    # 并行动画组
    circle.scale_to(1.0, duration=CREATE_DURATION, easing="ease_out_cubic")
    circle.fade_in(duration=CREATE_DURATION, easing="ease_out_cubic")
    # 可选：轻微旋转增加"绘制"感
    # circle.rotate(90, axis=(0, 0, 1), duration=CREATE_DURATION, easing="ease_in_out")
    
    s.play()
    
    # === Phase 2: 颜色变化 ===
    circle.change_color("CYAN", duration=1.5, easing="ease_in_out")
    s.play()
    
    # === Phase 3: 移动 ===
    circle.move_to(RIGHT * 3, duration=1.5, easing="ease_in_out")
    s.play()
    
    # === Phase 4: 淡出 ===
    circle.fade_out(duration=1.5, easing="ease_in")
    s.play()
    
    print(f"[PASS] {SCENE_NAME} complete!")
    print(f"  - Circle created with radius={CIRCLE_RADIUS}")
    print(f"  - Create animation duration={CREATE_DURATION}s")
    print(f"  - Total scene time ≈ 7.5s")

if __name__ == "__main__":
    main()
```

### 关键技术点说明

#### 1. 为什么用 Torus 而不是 Sphere/Cube？
- **Torus**（圆环）在 2D 俯视角度下看起来像一个**圆圈/圆环**
- 设置较小的 `inner_radius` 可以让管子变细，看起来更像**线条**
- 这是最接近 Manim `Circle()` 的 UE 原生几何体

#### 2. Scale + FadeIn 为什么能模拟 "Create"？
- **Scale 0→1**: 对象从中心点"生长"到完整大小（空间域）
- **Opacity 0→1**: 对象从透明变为不透明（颜色域）
- **两者同步**: 视觉上产生"逐渐显现并被画出"的感觉
- **Easing 选择**: `ease_out_cubic` 让开始快、结束慢（符合手绘减速特性）

#### 3. 与真正 Manim Create 的区别

| 特性 | Manim Create | UEMotion 模拟 |
|------|-------------|---------------|
| 绘制方式 | 沿路径逐点显示 | 整体缩放+淡入 |
| 起点/终点 | 可指定起始角度 | 固定从中心 |
| 支持形状 | 所有 VMobject | 仅限有界的几何体 |
| 精确度 | 像素级路径截取 | 近似效果 |
| 性能开销 | 中等（路径计算） | 低（标准变换） |

## 🚀 扩展建议（未来优化方向）

### 短期优化（Python 层面）
1. **封装 `create()` 方法** 到 Mobject 类：
   ```python
   def create(self, duration=2.0, rotation_deg=0):
       self.scale = 0.001
       self.opacity = 0.0
       self.scale_to(1.0, duration=duration)
       self.fade_in(duration=duration)
       if rotation_deg != 0:
           self.rotate(rotation_deg, duration=duration)
       return self
   ```

2. **添加更多预设动画组合**：
   - `grow_from_center()` = scale(0→1) + fade_in
   - `shrink_to_center()` = scale(1→0) + fade_out
   - `spin_in()` = rotate(360) + scale(0→1) + fade_in

### 中期优化（Material 层面）
3. **自定义 Material Instance** 支持 Progress 参数：
   - 创建 `M_CircleDraw` 材质，包含 `DrawProgress` (0→1) ScalarParameter
   - 在 Pixel Shader 中根据极坐标角度 discard 像素
   - 通过 Sequencer 的 Material Track 动画化该参数
   - **效果**：真正的"描边"动画！

### 长期优化（C++ 插件层面）
4. **新增 `UEMotionDrawAnimation` 类**：
   ```cpp
   UCLASS()
   class UEMOTIONPLUGIN_API UUEMotionDrawAnimation : public UUEMotionAnimation
   {
       GENERATED_BODY()
   public:
       // 控制绘制进度 (0.0 -> 1.0)
       UPROPERTY(EditAnywhere)
       float StartProgress = 0.0f;
       
       UPROPERTY(EditAnywhere)
       float EndProgress = 1.0f;
       
       // 绘制方向（顺时针/逆时针）
       UPROPERTY(EditAnywhere)
       bool bClockwise = true;
       
       // 起始角度（度数）
       UPROPERTY(EditAnywhere)
       float StartAngle = 0.0f;
   };
   ```

5. **Procedural Mesh 支持**：
   - 动态生成 Arc（圆弧）几何体
   - 根据进度参数动态调整弧长
   - 完美复刻 Manim 的 `pointwise_become_partial`

## ✅ 验证清单

- [ ] 示例脚本可正常运行（无 Python 错误）
- [ ] UE 编译无报错
- [ ] 渲染输出帧序列正常（151 帧 @ 30fps × 5s）
- [ ] 视觉效果符合预期（圆环从中心生长并显现）
- [ ] 动画流畅无明显卡顿
- [ ] 文件保存到正确路径

## 📦 交付物

1. **示例文件**: `Scripts/examples/circle_create_demo.py`
2. **文档注释**: 代码内含详细中文注释
3. **运行说明**: README 或代码头部说明如何执行
4. **对比截图**: （可选）与 Manim 原版效果的对比

## 🔄 后续改进路线图

```
Phase 1 (当前): ✅ Python 示例 - Scale+FadeIn 模拟
     ↓
Phase 2: 封装 create() 到 Mobject API
     ↓
Phase 3: Material Progress 参数动画（半真实描边）
     ↓
Phase 4: C++ DrawAnimation + Procedural Mesh（完全还原）
```

---

**预计工作量**: 30 分钟（仅示例代码）  
**技术难度**: ⭐⭐☆☆☆（初级，纯 Python）  
**依赖项**: UEMotionPlugin 已安装、UE 5.7 Editor
