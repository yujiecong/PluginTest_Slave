import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, Animation

s = Scene("geometry_transform", 1920, 1080)
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.camera.position = (-400, -500, 300)
s.camera.look_at((0, 0, 0))

shape_a = s.sphere(40, color="red", location=(-200, 0, 50))
shape_b = s.cube(50, color="blue", location=(200, 0, 50))

shape_a.move_to((0, 0, 50), duration=2, easing="ease_in_out")
shape_b.move_to((0, 0, 50), duration=2, easing="ease_in_out")

s.play()

merged = s.sphere(60, color="purple", location=(0, 0, 50))
merged.fade_in(duration=1, easing="ease_out")

shape_a.fade_out(duration=0.8, easing="ease_in")
shape_b.fade_out(duration=0.8, easing="ease_in")

s.play()

merged.scale_to(1.5, duration=1.5, easing="ease_out_back")
s.play()

merged.rotate(360, axis=(0, 0, 1), duration=2, easing="ease_in_out")
s.play()

merged.change_color("gold", duration=1.5, easing="ease_in_out")
s.play()

merged.scale_to(0.01, duration=1, easing="ease_in_back")
s.play()

print("[PASS] geometry_transform complete!")
