import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN


def main():
    """
    Manim-style Circle Creation Animation (简化版)

    演示 UEMotion 的 Create 动画效果：圆形从中心生长并显现。
    使用 Cylinder（扁平圆盘）模拟 2D 圆形。

    运行方式：
    在 UE Editor 的 Python Console 中执行此脚本
    """

    SCENE_NAME = "circle_create_demo"
    CIRCLE_COLOR = "PINK"

    s = Scene(SCENE_NAME, 1920, 1080, mode="2d")
    s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
    s.point_light(location=ORIGIN, color="white", intensity=3000)

    circle = s.cylinder(radius=2.0, height=0.06, color=CIRCLE_COLOR, location=ORIGIN)
    if not circle:
        print("[ERROR] Failed to create circle!")
        return

    circle.scale = 0.001
    circle.opacity = 0.0

    s.play()

    circle.scale_to(1.0, duration=1.0, easing="ease_out_cubic")
    circle.fade_in(duration=1.0, easing="ease_out_cubic")

    s.play()

    print(f"[PASS] {SCENE_NAME} complete!")


if __name__ == "__main__":
    main()
