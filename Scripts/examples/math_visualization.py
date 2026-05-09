import unreal
import sys
import os
import math

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, MVector

s = Scene("math_visualization", 1920, 1080, mode="2d")
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.point_light(location=ORIGIN, color="white", intensity=3000)

num_points = 40
x_start = -6.0
x_end = 6.0
x_range = x_end - x_start

spheres = []
for i in range(num_points):
    x = x_start + (x_range / (num_points - 1)) * i
    t = (x - x_start) / x_range
    y = 2.0 * math.sin(t * 2 * math.pi)
    sp = s.dot(color="cyan", location=MVector(x, y, 0))
    if sp:
        spheres.append(sp)

s.play()

for i, sp in enumerate(spheres):
    x = x_start + (x_range / (num_points - 1)) * i
    t = (x - x_start) / x_range
    y = 2.0 * math.cos(t * 2 * math.pi)
    sp.move_to(MVector(x, y, 0), duration=2, easing="ease_in_out")

s.play()

print("[PASS] math_visualization complete!")
