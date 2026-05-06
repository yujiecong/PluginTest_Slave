import unreal
import sys
import os
import math

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, vec

s = Scene(1920, 1080)
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.point_light(location=(0, 0, 400), color="white", intensity=3000)
s.camera.position = (-200, -500, 350)
s.camera.look_at((0, 0, 50))

num_points = 40
x_start = -300
x_end = 300
x_range = x_end - x_start

spheres = []
for i in range(num_points):
    x = x_start + (x_range / (num_points - 1)) * i
    t = (x - x_start) / x_range
    y = 100 * math.sin(t * 2 * math.pi)
    sp = s.sphere(8, color="cyan", location=(x, y, 30))
    if sp:
        spheres.append(sp)

s.play()

for i, sp in enumerate(spheres):
    x = x_start + (x_range / (num_points - 1)) * i
    t = (x - x_start) / x_range
    y = 100 * math.cos(t * 2 * math.pi)
    sp.move_to((x, y, 30), duration=2, easing="ease_in_out")

s.play()

print("[PASS] math_visualization complete!")
