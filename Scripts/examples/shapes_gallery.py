import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, LEFT, RIGHT, UP, DOWN, MVector

s = Scene("shapes_gallery", 1920, 1080, mode="2d")
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.point_light(location=ORIGIN, color="white", intensity=3000)

positions = [
    LEFT * 4 + DOWN * 3, DOWN * 3, RIGHT * 4 + DOWN * 3,
    LEFT * 4 + UP, ORIGIN, RIGHT * 4 + UP,
]

s.sphere(0.8, color="red", location=positions[0])
s.cube(1.0, color="blue", location=positions[1])
s.cylinder(0.6, 1.6, color="green", location=positions[2])
s.cone(0.7, 1.6, color="yellow", location=positions[3])
s.plane(1.6, 1.6, color="gray", location=positions[4])
s.torus(0.8, 0.24, color="purple", location=positions[5])

s.play()
print("[PASS] shapes_gallery complete!")
