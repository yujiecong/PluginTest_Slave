import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene

s = Scene("hello_sphere", 1920, 1080)
s.directional_light(direction=(0, -1, -1), color="white", intensity=8)
s.camera.position = (-350, -450, 250)
s.camera.look_at((0, 0, 0))

ball = s.sphere(60, color="red", location=(0, 0, 50))
ball.move_to((200, 0, 50), duration=2, easing="ease_in_out")

s.play()
print("[PASS] hello_sphere complete!")
