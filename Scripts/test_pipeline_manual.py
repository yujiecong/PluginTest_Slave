import unreal
import sys
import os
import time

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPTS_DIR)
sys.path.insert(0, os.path.join(SCRIPTS_DIR, 'tests'))

from pymanim import Scene

PROJECT_DIR = os.path.abspath(os.path.join(SCRIPTS_DIR, '..'))
OUTPUT_BASE = os.path.join(PROJECT_DIR, "Saved", "UEMotionTest", "full_pipeline").replace("\\", "/")

print("=" * 64)
print("  UEMotion Full Pipeline Test")
print("  Cube moves along y=x, step=10 units")
print("=" * 64)
print()

num_steps = 5
step_size = 10
initial_z = 50.0

print("[1/4] Creating scene...")
s = Scene(1920, 1080)
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.point_light(location=(0, 0, 400), color="white", intensity=3000)
s.camera.position = (-300, -500, 400)
s.camera.look_at((num_steps * step_size / 2.0, num_steps * step_size / 2.0, initial_z))

print("[2/4] Creating cube and animating along y=x...")
box = s.cube(30, color="cyan", location=(0, 0, initial_z))

for i in range(1, num_steps + 1):
    target_x = i * step_size
    target_y = i * step_size
    box.move_to((target_x, target_y, initial_z), duration=1.0, easing="linear")
    s.play()
    loc = box.location
    print(f"  Step {i}: Cube at ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")

loc = box.location
print(f"\n  Final position: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
print(f"  Expected:       ({num_steps * step_size}, {num_steps * step_size}, {initial_z})")

x_ok = abs(loc.x - num_steps * step_size) < 3.0
y_ok = abs(loc.y - num_steps * step_size) < 3.0
z_ok = abs(loc.z - initial_z) < 3.0

if x_ok and y_ok and z_ok:
    print("  [PASS] Position correct!")
else:
    print(f"  [FAIL] Position mismatch! x_ok={x_ok} y_ok={y_ok} z_ok={z_ok}")

print()
print("[3/4] Rendering frames via MoviePipeline...")

frames_dir = OUTPUT_BASE + "/y_equals_x_steps"
os.makedirs(frames_dir, exist_ok=True)

s._ue.render_frames(frames_dir, 2.0, 30)

print("  Waiting for MoviePipeline to finish rendering...")
for wait in range(120):
    time.sleep(1)
    if not s._ue.has_active_animations():
        pass
    png_files = []
    if os.path.isdir(frames_dir):
        png_files = [f for f in os.listdir(frames_dir) if f.lower().endswith('.png')]
    if len(png_files) > 0:
        print(f"  Found {len(png_files)} image(s) after {wait + 1}s")
        break
    if wait % 10 == 9:
        print(f"  ... still waiting ({wait + 1}s)")

print()
print("[4/4] Checking output images...")

img_count = 0
if os.path.isdir(frames_dir):
    png_files = [f for f in os.listdir(frames_dir) if f.lower().endswith('.png')]
    img_count = len(png_files)
    for f in sorted(png_files)[:10]:
        fpath = os.path.join(frames_dir, f)
        fsize = os.path.getsize(fpath)
        print(f"  -> {f} ({fsize} bytes)")
    if len(png_files) > 10:
        print(f"  ... and {len(png_files) - 10} more")
else:
    print(f"  [MISSING] Directory not found: {frames_dir}")

print()
print("=" * 64)
if img_count > 0:
    print(f"  RESULT: PASSED - {img_count} image(s) generated!")
    print(f"  Output: {frames_dir}")
else:
    print("  RESULT: FAILED - No images generated")
    print("  (MoviePipeline may need viewport/rendering support)")
print("=" * 64)

s.destroy()
