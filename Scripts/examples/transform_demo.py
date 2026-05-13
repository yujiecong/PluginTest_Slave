import unreal
import sys
import os
import math

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, PINK, PI, Animation

s = Scene("transform_demo", 1920, 1080, mode="2d")

print("=" * 60)
print("  UEMotion Transform Animation Demo")
print("  Replace+Crossfade: Cube → Cylinder")
print("=" * 60)

square = s.cube(size=2.6, color=PINK, location=ORIGIN)
square.rotate(PI / 4)

circle = s.cylinder(radius=1.5, height=0.05, color=PINK, location=ORIGIN)
circle.opacity = 0.0

print("\n[Step 1] 显示旋转的立方体（菱形 45°）...")
square.scale_to(1.0, duration=0.01, easing="linear")
square.fade_in(duration=0.01, easing="linear")
s.play()
print("  ✓ 菱形已显示 (旋转45°)")

print("\n[Step 2] Transform: Cube→Cylinder (2秒)")
print("  ├── 源(Cube): 保持可见 (用户控制fadeOut)")
print("  └── 目标(Cylinder): Scale 0→1 + Opacity 0→1")
square.fade_out(duration=2.0, easing="ease_in_out")
s.transform(square, circle, duration=2.0, easing="ease_in_out")
s.play()
print("  ✓ 变换完成")

print("\n[Step 3] 保持圆形状态 (1秒)...")
s.wait(1.0)
print("  ✓ 等待完成")

print("\n[Step 4] FadeOut 圆形...")
circle.fade_out(duration=1.0, easing="ease_in")
s.play()
print("  ✓ 淡出完成")

print("\n" + "=" * 60)
print("  [PASS] Transform Demo Complete!")
print("=" * 60)
print("\n动画序列:")
print("  1. Create: 菱形(Cube旋转45°)")
print("  2. Transform: Cube→Cylinder (2s, 并行)")
print("     ├── square.fade_out():   Opacity 1→0")
print("     └── circle.transform(): Scale 0→1 + Opacity 0→1")
print("  3. Wait: 保持1秒")
print("  4. FadeOut: circle 淡出 (1秒)")
print("\n总时长: ~5秒")
