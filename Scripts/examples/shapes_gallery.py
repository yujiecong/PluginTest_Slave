import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene

s = Scene("shapes_gallery", 1920, 1080)
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.point_light(location=(0, 0, 300), color="white", intensity=3000)
s.camera.position = (-500, -600, 400)
s.camera.look_at((0, 0, 0))

positions = [
    (-200, -150, 0), (0, -150, 0), (200, -150, 0),
    (-200, 50, 0), (0, 50, 0), (200, 50, 0),
]

s.sphere(40, color="red", location=positions[0])
s.cube(50, color="blue", location=positions[1])
s.cylinder(30, 80, color="green", location=positions[2])
s.cone(35, 80, color="yellow", location=positions[3])
s.plane(80, 80, color="gray", location=positions[4])
s.torus(40, 12, color="purple", location=positions[5])

s.play()
print("[PASS] shapes_gallery complete!")
