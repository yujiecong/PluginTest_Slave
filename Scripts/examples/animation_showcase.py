import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, LEFT, RIGHT, UP, DOWN, Animation

s = Scene("animation_showcase", 1920, 1080, mode="2d")
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)

sphere = s.sphere(0.8, color="red", location=LEFT * 4)
sphere.move_to(RIGHT * 4, duration=2, easing="ease_in_out")

cube = s.cube(1.0, color="blue", location=ORIGIN)
cube.rotate(720, axis=(0, 0, 1), duration=3, easing="ease_out_cubic")

cylinder = s.cylinder(0.6, 1.6, color="green", location=DOWN * 4)
cylinder.scale_to(2.0, duration=2, easing="ease_in_out")

s.play()

sphere2 = s.sphere(0.7, color="yellow", location=UP * 4)
sphere2.fade_in(duration=1.5, easing="ease_in_out")

s.play()

cube2 = s.cube(0.9, color="cyan", location=ORIGIN)
cube2.change_color("magenta", duration=2, easing="ease_in_out")

s.play()

sphere3 = s.sphere(0.6, color="orange", location=ORIGIN)
sphere3.fade_out(duration=1.5, easing="ease_in")

s.play()

print("[PASS] animation_showcase complete!")
