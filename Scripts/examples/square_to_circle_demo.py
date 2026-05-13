import unreal
import sys
import os
import math

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, PINK, PI, Animation

s = Scene("square_to_circle", 1920, 1080, mode="2d")
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)

circle = s.circle(radius=1.5, color=PINK, opacity=0.7)
circle.opacity = 0.0

square = s.square(side_length=2.6, color=PINK, opacity=0.7)
square.rotate(PI / 4)

s.play()

square.scale_to(1.0, duration=0.8, easing="ease_out_cubic")
square.fade_in(duration=0.8, easing="ease_out_cubic")
s.play()

s.transform(square, circle, duration=2.0, easing="ease_in_out")
s.play()

square.fade_out(duration=1.0, easing="ease_in")
s.play()

print("[PASS] SquareToCircle Transform complete!")
print("  - Square (rotated) created and displayed")
print("  - Transform: Square → Circle over 2 seconds")
print("  - Final shape faded out")
