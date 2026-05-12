import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, UP, DOWN, LEFT, RIGHT, MVector

s = Scene("cube_basic_animations", 1920, 1080, mode="2d")
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
# s.point_light(location=ORIGIN, color="white", intensity=5000)  # Disabled for dark background

FPS = 30
FRAME_DURATION = 1.0

print("=" * 60)
print("UEMotion Cube Basic Animations Showcase (2D Orthographic)")
print(f"Animation Duration: {FRAME_DURATION}s ({FPS} frames @ {FPS}fps)")
print("=" * 60)

cube = s.cube(color="#3498db", location=ORIGIN)

print("\n[1/6] MOVE_TO: Moving cube from center to RIGHT")
cube.move_to(RIGHT * 3, duration=FRAME_DURATION, easing="ease_in_out")
s.play()
s.wait(0.5)

print("[2/6] SHIFT: Shifting cube UP by offset")
cube.shift(UP * 2, duration=FRAME_DURATION, easing="ease_out")
s.play()
s.wait(0.5)

print("[3/6] ROTATE: Rotating cube 360 degrees around Z axis")
cube.rotate(360, axis=(0, 0, 1), duration=FRAME_DURATION, easing="linear")
s.play()
s.wait(0.5)

print("[4/6] SCALE_TO: Scaling cube to 2x size")
cube.scale_to(2.0, duration=FRAME_DURATION, easing="ease_in_out")
s.play()
s.wait(0.5)

print("[5/6] FADE_OUT + FADE_IN: Fade out then fade back in")
cube.fade_out(duration=FRAME_DURATION, easing="ease_in")
s.play()
s.wait(0.3)
cube.fade_in(duration=FRAME_DURATION, easing="ease_out")
s.play()
s.wait(0.5)

print("[6/6] CHANGE_COLOR: Transitioning from blue to orange to green")
cube.change_color("#e74c3c", duration=FRAME_DURATION, easing="ease_in_out")
s.play()
s.wait(0.3)
cube.change_color("#2ecc71", duration=FRAME_DURATION, easing="ease_in_out")
s.play()

print("\n" + "=" * 60)
print("All 6 basic animations completed successfully!")
print("=" * 60)
