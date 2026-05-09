import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, RIGHT

s = Scene("hello_sphere", 1920, 1080, mode="2d")
s.directional_light(direction=(0, -1, -1), color="white", intensity=8)

ball = s.sphere(1.2, color="red", location=ORIGIN)
ball.move_to(RIGHT * 4, duration=2, easing="ease_in_out")

s.play()
print("[PASS] hello_sphere complete!")
