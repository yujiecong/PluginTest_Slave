import unreal
import sys
import os
import traceback

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPTS_DIR)

from uemotion import Scene


PROJECT_DIR = os.path.abspath(os.path.join(SCRIPTS_DIR, '..'))
OUTPUT_BASE = os.path.join(PROJECT_DIR, "Saved", "UEMotionTest", "full_pipeline").replace("\\", "/")

SCENE_NAME = "y_equals_x"
RESOLUTION_W = 1920
RESOLUTION_H = 1080

NUM_STEPS = 5
STEP_SIZE = 10
INITIAL_Z = 0.0
FPS = 30
MOVE_DURATION = 1.0
CUBE_SIZE = 30
CUBE_COLOR = "cyan"

CENTER_X = NUM_STEPS * STEP_SIZE / 2.0
CENTER_Y = CENTER_X
CAMERA_HEIGHT = 800.0
LIGHT_INTENSITY = 3.0
POINT_LIGHT_HEIGHT = 500.0
POINT_LIGHT_INTENSITY = 2000.0

TOTAL_DURATION = NUM_STEPS * MOVE_DURATION
EXPECTED_FRAMES = int(TOTAL_DURATION * FPS) + 1
EXPECTED_MIN_FILE_SIZE = 1_000_000
EXPECTED_MAX_FILE_SIZE = 3_000_000
FRAMES_DIR_NAME = f"{SCENE_NAME}_frames"

test_results = []
test_failures = []


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
        unreal.SystemLibrary.quit_editor(unreal.QuitPreference.QUIT)
    except Exception:
        pass


def main():
    print_header("  UEMotion Full Pipeline Smoke Test")

    print(f"  Scene        : {SCENE_NAME}")
    print(f"  Resolution   : {RESOLUTION_W}x{RESOLUTION_H}")
    print(f"  Animation    : {NUM_STEPS} steps x {MOVE_DURATION}s = {TOTAL_DURATION}s")
    print(f"  FPS          : {FPS}")
    print(f"  Expected     : {EXPECTED_FRAMES} frames")
    print(f"  Path         : y=x from (0,0) to ({NUM_STEPS*STEP_SIZE},{NUM_STEPS*STEP_SIZE})")
    print(f"  Camera       : top-down at Z={CAMERA_HEIGHT}")
    print()

    print("[1/4] Creating scene...")
    s = Scene(SCENE_NAME, RESOLUTION_W, RESOLUTION_H)
    check(s is not None, "Scene object created")
    check(s.name == SCENE_NAME, f"Scene name = '{SCENE_NAME}'")

    s.directional_light(direction=(0, 0, -1), color="white", intensity=LIGHT_INTENSITY)
    s.point_light(location=(CENTER_X, CENTER_Y, POINT_LIGHT_HEIGHT), color="white", intensity=POINT_LIGHT_INTENSITY)

    cam_pos = (CENTER_X, CENTER_Y, CAMERA_HEIGHT)
    cam_look = (CENTER_X, CENTER_Y, INITIAL_Z)
    s.camera.position = cam_pos
    s.camera.look_at(cam_look)
    check(abs(s.camera.position.z - CAMERA_HEIGHT) < 0.01, f"Camera Z = {CAMERA_HEIGHT} (top-down)")

    print("[2/4] Creating cube and animating along y=x...")
    box = s.cube(CUBE_SIZE, color=CUBE_COLOR, location=(0, 0, INITIAL_Z))
    check(box is not None, "Cube Mobject created")

    for i in range(1, NUM_STEPS + 1):
        target_x = i * STEP_SIZE
        target_y = i * STEP_SIZE
        box.move_to((target_x, target_y, INITIAL_Z), duration=MOVE_DURATION, easing="linear")
        s.play()
        box.location = (target_x, target_y, INITIAL_Z)

    final_pos = box.location
    expected_final = (NUM_STEPS * STEP_SIZE, NUM_STEPS * STEP_SIZE, INITIAL_Z)
    check(
        abs(final_pos.x - expected_final[0]) < 0.01
        and abs(final_pos.y - expected_final[1]) < 0.01,
        f"Cube final position = ({expected_final[0]}, {expected_final[1]}, {INITIAL_Z})"
    )

    print(f"  Animation: {NUM_STEPS} steps, {TOTAL_DURATION}s total, {EXPECTED_FRAMES} expected frames")

    print("[3/4] Rendering frames via MoviePipeline...")

    frames_dir = os.path.join(OUTPUT_BASE, FRAMES_DIR_NAME).replace("\\", "/")
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
    print(f"  Waiting for MoviePipeline callback ({EXPECTED_FRAMES} frames expected)...")


try:
    main()
except Exception as e:
    print()
    print("=" * 64)
    print(f"  >>> FATAL ERROR <<<")
    print(f"  {type(e).__name__}: {e}")
    print()
    traceback.print_exc()
    print("=" * 64)
    shutdown_ue(1)
