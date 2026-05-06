import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, Animation

s = Scene(1920, 1080)
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.camera.position = (-400, -500, 300)
s.camera.look_at((0, 0, 0))

sphere = s.sphere(40, color="red", location=(-200, 0, 50))
sphere.move_to((200, 0, 50), duration=2, easing="ease_in_out")

cube = s.cube(50, color="blue", location=(0, 0, 50))
cube.rotate(720, axis=(0, 0, 1), duration=3, easing="ease_out_cubic")

cylinder = s.cylinder(30, 80, color="green", location=(0, -200, 50))
cylinder.scale_to(2.0, duration=2, easing="ease_in_out")

s.play()

sphere2 = s.sphere(35, color="yellow", location=(0, 200, 50))
sphere2.fade_in(duration=1.5, easing="ease_in_out")

s.play()

cube2 = s.cube(45, color="cyan", location=(0, 0, 50))
cube2.change_color("magenta", duration=2, easing="ease_in_out")

s.play()

sphere3 = s.sphere(30, color="orange", location=(0, 0, 50))
sphere3.fade_out(duration=1.5, easing="ease_in")

s.play()

print("[PASS] animation_showcase complete!")
