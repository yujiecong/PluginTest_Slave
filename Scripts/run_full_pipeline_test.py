import unreal
import sys
import os
import unittest
import time
import glob

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'tests'))
from test_framework import UEMotionTestCase, JsonReportRunner

OUTPUT_BASE = "D:/UEMotionTest/full_pipeline"


def check_output_images():
    results = {}

    steps_dir = os.path.join(OUTPUT_BASE, "y_equals_x_steps")
    if os.path.isdir(steps_dir):
        png_files = []
        for root, dirs, files in os.walk(steps_dir):
            for f in files:
                if f.lower().endswith('.png'):
                    png_files.append(os.path.join(root, f))
        results["y_equals_x_steps"] = {
            "found": len(png_files),
            "files": png_files
        }
    else:
        results["y_equals_x_steps"] = {"found": 0, "files": [], "error": "directory not found"}

    seq_dir = os.path.join(OUTPUT_BASE, "y_equals_x_sequence")
    if os.path.isdir(seq_dir):
        png_files = [f for f in os.listdir(seq_dir) if f.lower().endswith('.png')]
        results["y_equals_x_sequence"] = {
            "found": len(png_files),
            "files": [os.path.join(seq_dir, f) for f in png_files]
        }
    else:
        results["y_equals_x_sequence"] = {"found": 0, "files": [], "error": "directory not found"}

    pymanim_dir = os.path.join(OUTPUT_BASE, "pymanim_y_equals_x")
    if os.path.isdir(pymanim_dir):
        png_files = [f for f in os.listdir(pymanim_dir) if f.lower().endswith('.png')]
        results["pymanim_y_equals_x"] = {
            "found": len(png_files),
            "files": [os.path.join(pymanim_dir, f) for f in png_files]
        }
    else:
        results["pymanim_y_equals_x"] = {"found": 0, "files": [], "error": "directory not found"}

    return results


def main():
    print("=" * 64)
    print("  UEMotion Full Pipeline Test Runner")
    print("=" * 64)

    loader = unittest.TestLoader()
    suite = loader.loadTestsFromName('test_16_full_pipeline.TestFullPipeline',
                                     module=__import__('test_16_full_pipeline'))

    print(f"\n[INFO] Loaded {suite.countTestCases()} test cases")
    print("[INFO] Running tests...\n")

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    print("\n" + "=" * 64)
    print("  Checking output images...")
    print("=" * 64)

    time.sleep(2)

    img_results = check_output_images()
    total_images = 0
    for category, info in img_results.items():
        count = info["found"]
        total_images += count
        status = "OK" if count > 0 else "MISSING"
        print(f"  [{status}] {category}: {count} image(s)")
        if count > 0:
            for f in info["files"][:5]:
                print(f"         -> {f}")
            if count > 5:
                print(f"         ... and {count - 5} more")

    print(f"\n  Total images generated: {total_images}")

    if result.wasSuccessful() and total_images > 0:
        print("\n  Result: ALL TESTS PASSED (images generated)")
    elif result.wasSuccessful() and total_images == 0:
        print("\n  Result: TESTS PASSED but NO IMAGES generated")
    else:
        print(f"\n  Result: {len(result.failures)} FAILURES, {len(result.errors)} ERRORS")

    print("=" * 64)

    return 0 if result.wasSuccessful() else 1


exit_code = 1
try:
    exit_code = main()
except Exception as e:
    print(f"[FATAL] Test runner crashed: {e}")
    import traceback
    traceback.print_exc()
finally:
    try:
        unreal.SystemLibrary.quit_game(unreal.EditorLevelLibrary.get_editor_world())
    except Exception:
        pass
