import unreal
import sys
import os
import traceback

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPTS_DIR)

from uemotion import Scene
from uemotion.constants import *
from uemotion import MVector


PROJECT_DIR = os.path.abspath(os.path.join(SCRIPTS_DIR, '..'))
OUTPUT_BASE = os.path.join(PROJECT_DIR, "Saved", "UEMotionTest", "full_pipeline").replace("\\", "/")

SCENE_NAME = "y_equals_x"
RESOLUTION_W = DEFAULT_PIXEL_WIDTH
RESOLUTION_H = DEFAULT_PIXEL_HEIGHT

NUM_STEPS = 5
STEP_SIZE = 1.0
MOVE_DURATION = 1.0
FPS = DEFAULT_FPS
CUBE_SIZE = DEFAULT_CUBE_SIDE
CUBE_COLOR = "cyan"

START_POS = DL
END_POS = UR

TOTAL_DURATION = NUM_STEPS * MOVE_DURATION
EXPECTED_FRAMES = int(TOTAL_DURATION * FPS) + 1
EXPECTED_MIN_FILE_SIZE = 400_000
EXPECTED_MAX_FILE_SIZE = 2_000_000
FRAMES_DIR_NAME = f"{SCENE_NAME}_frames"

test_results = []
test_failures = []
s = None


def check(condition, description):
    status = "PASS" if condition else "FAIL"
    test_results.append((status, description))
    if not condition:
        test_failures.append(description)
    print(f"    [{status}] {description}")


def print_header(title):
    print()
    print("=" * 64)
    print(f"  {title}")
    print("=" * 64)


def print_summary():
    passed = sum(1 for s, _ in test_results if s == "PASS")
    failed = sum(1 for s, _ in test_results if s == "FAIL")
    total = len(test_results)

    print_header("  UEMotion Smoke Test - Final Report")

    print(f"  Total Checks : {total}")
    print(f"  Passed       : {passed}")
    print(f"  Failed       : {failed}")
    print()

    if failed > 0:
        print("  Failure Details:")
        for i, desc in enumerate(test_failures, 1):
            print(f"    {i}. {desc}")

    print()

    if failed == 0:
        print(f"  >>> RESULT: ALL {total} CHECKS PASSED <<<")
        print(f"  Output: {os.path.join(OUTPUT_BASE, FRAMES_DIR_NAME)}")
        print("=" * 64)
        return 0
    else:
        print(f"  >>> RESULT: {failed} CHECK(S) FAILED <<<")
        print("=" * 64)
        return 1


def shutdown_ue(exit_code=1):
    unreal.log("UEMotion Smoke Test: Shutting down UE Editor...")
    try:
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    except Exception:
        pass
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception:
        pass


print_header("  UEMotion Full Pipeline Smoke Test (2D Orthographic)")

print(f"  Scene        : {SCENE_NAME}")
print(f"  Resolution   : {RESOLUTION_W}x{RESOLUTION_H} (1:1)")
print(f"  Frame Size   : {FRAME_WIDTH} x {FRAME_HEIGHT} UEMotion units")
print(f"  Animation    : {NUM_STEPS} steps x {MOVE_DURATION}s = {TOTAL_DURATION}s")
print(f"  FPS          : {FPS}")
print(f"  Expected     : {EXPECTED_FRAMES} frames")
print(f"  Path         : y=x from {START_POS} (DL/Q3) to {END_POS} (UR/Q1)")
print(f"  Camera       : orthographic, OrthoWidth={FRAME_WIDTH * SCALE_FACTOR}")
print(f"  Cube Size    : {CUBE_SIZE} UEMotion units ({CUBE_SIZE * SCALE_FACTOR} UE cm)")
print()

try:
    print("[1/4] Creating scene (2D orthographic mode)...")
    s = Scene(SCENE_NAME, RESOLUTION_W, RESOLUTION_H, mode="2d")
    check(s is not None, "Scene object created")
    check(s.name == SCENE_NAME, f"Scene name = '{SCENE_NAME}'")
    check(abs(s.frame_width - FRAME_WIDTH) < 0.01, f"Frame width = {FRAME_WIDTH} (1:1)")
    check(abs(s.frame_height - FRAME_HEIGHT) < 0.01, f"Frame height = {FRAME_HEIGHT} (1:1)")
    check(s.mode == "2d", "Scene mode = '2d'")

    check(s.camera.projection_mode == PROJECTION_ORTHOGRAPHIC,
          f"Camera projection = orthographic (mode={s.camera.projection_mode})")

    expected_ortho_width = FRAME_WIDTH * SCALE_FACTOR
    check(abs(s.camera.ortho_width - expected_ortho_width) < 1.0,
          f"Ortho width = {expected_ortho_width} (actual={s.camera.ortho_width:.1f})")

    s.directional_light(direction=(0, 0, -1), color="white", intensity=3.0)
    s.point_light(location=ORIGIN, color="white", intensity=2000.0)

    cam_z_expected = motion_to_ue(DEFAULT_CAMERA_Z)
    check(abs(s.camera.position.z - cam_z_expected) < 0.01,
          f"Camera at Z = {cam_z_expected}")

    print("[2/4] Creating cube at DL (Q3) and animating along y=x to UR (Q1)...")
    box = s.cube(CUBE_SIZE, color=CUBE_COLOR, location=START_POS)
    check(box is not None, f"Cube Mobject created at {START_POS} (DL/Q3)")

    box_loc = box.location
    check(isinstance(box_loc, MVector), "Mobject.location returns MVector")
    check(abs(box_loc.x - START_POS.x) < 0.01 and abs(box_loc.y - START_POS.y) < 0.01,
          f"Cube location = {START_POS} (UEMotion units)")

    for i in range(1, NUM_STEPS + 1):
        t = i / NUM_STEPS
        target_x = START_POS.x + t * (END_POS.x - START_POS.x)
        target_y = START_POS.y + t * (END_POS.y - START_POS.y)
        box.move_to(MVector(target_x, target_y, 0.0), duration=MOVE_DURATION, easing="linear")
        s.play()
        box.location = MVector(target_x, target_y, 0.0)

    final_pos = box.location
    check(
        abs(final_pos.x - END_POS.x) < 0.01
        and abs(final_pos.y - END_POS.y) < 0.01,
        f"Cube final position = {END_POS} (UR/Q1)"
    )

    bb = box.get_bounding_box()
    check(bb is not None and 'min' in bb and 'max' in bb,
          "Mobject.get_bounding_box() works")
    check(isinstance(box.width, float) and box.width > 0,
          f"Mobject.width = {box.width:.4f} UEMotion units")

    print(f"  Animation: {NUM_STEPS} steps, {TOTAL_DURATION}s total, {EXPECTED_FRAMES} expected frames")

    print("[3/4] Rendering frames via MoviePipeline...")

    frames_dir = os.path.join(OUTPUT_BASE, FRAMES_DIR_NAME).replace("\\", "/")
    if os.path.isdir(frames_dir):
        old_files = [f for f in os.listdir(frames_dir) if f.lower().endswith('.png')]
        for f in old_files:
            os.remove(os.path.join(frames_dir, f))
        if old_files:
            print(f"  Cleaned {len(old_files)} old frame(s) from previous run")
    os.makedirs(frames_dir, exist_ok=True)
    check(os.path.isdir(frames_dir), f"Output directory exists: {frames_dir}")

    def on_render_done(success):
        print()
        print("[4/4] Validating output images...")

        check(success, "MoviePipeline render callback: success=True")

        img_count = 0
        min_size = None
        max_size = None
        png_files = []

        if os.path.isdir(frames_dir):
            png_files = sorted([f for f in os.listdir(frames_dir) if f.lower().endswith('.png')])
            img_count = len(png_files)

            for f in png_files[:5]:
                fpath = os.path.join(frames_dir, f)
                fsize = os.path.getsize(fpath)
                if min_size is None or fsize < min_size:
                    min_size = fsize
                if max_size is None or fsize > max_size:
                    max_size = fsize
                print(f"    -> {f} ({fsize:,} bytes)")
            if len(png_files) > 5:
                print(f"    ... and {len(png_files) - 5} more files")
        else:
            check(False, f"Output directory missing after render: {frames_dir}")

        check(img_count == EXPECTED_FRAMES,
              f"Frame count = {img_count} (expected {EXPECTED_FRAMES})")

        if img_count > 0 and min_size is not None:
            size_ok = EXPECTED_MIN_FILE_SIZE <= min_size <= EXPECTED_MAX_FILE_SIZE
            check(size_ok,
                  f"File sizes valid: min={min_size:,} max={max_size:,} bytes "
                  f"(range [{EXPECTED_MIN_FILE_SIZE:,}, {EXPECTED_MAX_FILE_SIZE:,}])")

        if png_files:
            first_name = png_files[0]
            last_name = png_files[-1]
            check(first_name.endswith(".png"), f"First file name format OK: {first_name}")
            check(last_name.endswith(".png"), f"Last file name format OK: {last_name}")

        exit_code = print_summary()

        s.set_auto_cleanup(False)

        shutdown_ue(exit_code)

    s.on_render_finished(on_render_done)
    s._ue.render_frames(frames_dir, TOTAL_DURATION, FPS)

    print(f"  Rendering to: {frames_dir}")
    print(f"  Waiting for MoviePipeline to finish ({EXPECTED_FRAMES} frames expected)...")

except Exception as e:
    print()
    print("=" * 64)
    print(f"  >>> FATAL ERROR <<<")
    print(f"  {type(e).__name__}: {e}")
    print()
    traceback.print_exc()
    print("=" * 64)
    shutdown_ue(1)
