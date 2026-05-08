import unreal
import sys
import os

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPTS_DIR)

from uemotion import Scene

PROJECT_DIR = os.path.abspath(os.path.join(SCRIPTS_DIR, '..'))
OUTPUT_BASE = os.path.join(PROJECT_DIR, "Saved", "UEMotionTest", "full_pipeline").replace("\\", "/")

NUM_STEPS = 5
STEP_SIZE = 10
INITIAL_Z = 50.0
FPS = 30
MOVE_DURATION = 1.0

print("=" * 64)
print("  UEMotion Full Pipeline Test")
print(f"  Cube moves along y=x, step={STEP_SIZE} units, {NUM_STEPS} steps")
print("=" * 64)
print()

print("[1/4] Creating scene...")
s = Scene("y_equals_x", 1920, 1080)
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.point_light(location=(0, 0, 400), color="white", intensity=3000)
s.camera.position = (-300, -500, 400)
s.camera.look_at((NUM_STEPS * STEP_SIZE / 2.0, NUM_STEPS * STEP_SIZE / 2.0, INITIAL_Z))

print("[2/4] Creating cube and animating along y=x...")
box = s.cube(30, color="cyan", location=(0, 0, INITIAL_Z))

for i in range(1, NUM_STEPS + 1):
    target_x = i * STEP_SIZE
    target_y = i * STEP_SIZE
    box.move_to((target_x, target_y, INITIAL_Z), duration=MOVE_DURATION, easing="linear")
    s.play()

total_duration = NUM_STEPS * MOVE_DURATION
print(f"  Animation recorded: {NUM_STEPS} steps, {total_duration}s total")
print(f"  LevelSequence tracks written to Sequencer timeline")

print()
print("[3/4] Rendering frames via MoviePipeline...")

frames_dir = os.path.join(OUTPUT_BASE, "y_equals_x_frames").replace("\\", "/")
os.makedirs(frames_dir, exist_ok=True)

def on_render_done(success):
    print()
    print("[4/4] Checking output images...")

    img_count = 0
    if os.path.isdir(frames_dir):
        png_files = sorted([f for f in os.listdir(frames_dir) if f.lower().endswith('.png')])
        img_count = len(png_files)
        for f in png_files[:10]:
            fpath = os.path.join(frames_dir, f)
            fsize = os.path.getsize(fpath)
            print(f"  -> {f} ({fsize} bytes)")
        if len(png_files) > 10:
            print(f"  ... and {len(png_files) - 10} more")
    else:
        print(f"  [MISSING] Directory not found: {frames_dir}")

    print()
    print("=" * 64)
    if success and img_count > 0:
        print(f"  RESULT: PASSED - {img_count} image(s) generated!")
        print(f"  Output: {frames_dir}")
    else:
        print(f"  RESULT: {'FAILED (render error)' if not success else 'NO IMAGES'}")
    print("=" * 64)

    s.set_auto_cleanup(False)

s.on_render_finished(on_render_done)
s._ue.render_frames(frames_dir, total_duration, FPS)

print(f"  Rendering to: {frames_dir}")
print("  Waiting for MoviePipeline callback...")
