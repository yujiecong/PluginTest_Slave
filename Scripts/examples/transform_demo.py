import unreal
import sys
import os
import math

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, UP, DOWN, LEFT, RIGHT, PINK, BLUE, RED, YELLOW, GREEN, PI, Animation

s = Scene("transform_demo", 1920, 1080, mode="2d")
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)

print("=" * 60)
print("  UEMotion Transform Animation Demo")
print("  演示形状变形效果 (Square → Circle)")
print("=" * 60)

circle = s.circle(radius=1.5, color=PINK, opacity=0.8)
circle.opacity = 0.0

square = s.square(side_length=2.6, color=PINK, opacity=0.8)
square.rotate(PI / 4)

print("\n[Step 1] 显示旋转的方形（菱形）...")
square.scale_to(1.0, duration=0.01, easing="linear")
square.fade_in(duration=0.01, easing="linear")
s.play()
print("  ✓ 菱形已显示")

print("\n[Step 2] Transform: 菱形 → 圆形 (2秒)...")
s.transform(square, circle, duration=2.0, easing="ease_in_out")
s.play()
print("  ✓ 变形完成")

print("\n[Step 3] 保持圆形状态 (1秒)...")
s.wait(1.0)
print("  ✓ 等待完成")

print("\n[Step 4] FadeOut 圆形...")
square.fade_out(duration=1.0, easing="ease_in")
s.play()
print("  ✓ 淡出完成")

print("\n" + "=" * 60)
print("  [PASS] Transform Demo Complete!")
print("=" * 60)
print("\n动画序列:")
print("  1. Create: 显示菱形")
print("  2. Transform: 菱形 → 圆形 (2秒, ease_in_out)")
print("  3. Wait: 保持1秒")
print("  4. FadeOut: 淡出 (1秒)")
print("\n总时长: ~5秒")
