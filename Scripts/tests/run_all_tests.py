import unittest
import sys
import os
import subprocess

try:
    TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:
    import inspect
    try:
        TESTS_DIR = os.path.dirname(os.path.abspath(inspect.currentframe().f_code.co_filename))
    except (NameError, AttributeError):
        TESTS_DIR = os.getcwd()

sys.path.insert(0, TESTS_DIR)
sys.path.insert(0, os.path.join(TESTS_DIR, '..'))

from test_framework import JsonReportRunner


def main():
    filter_pattern = "test_"
    for arg in sys.argv[1:]:
        if arg.startswith("--filter="):
            filter_pattern = arg.split("=", 1)[1]

    loader = unittest.TestLoader()
    loader.testNamePattern = filter_pattern if filter_pattern != "test_" else "test*"
    suite = loader.discover(TESTS_DIR, pattern="test_*.py")

    print("=" * 64)
    print("  UEMotion Automated Test Suite")
    print("=" * 64)

    runner = JsonReportRunner(verbosity=2)
    result = runner.run(suite)

    print("\n" + "=" * 64)
    if result.wasSuccessful():
        print("  Result: ALL TESTS PASSED")
    else:
        print(f"  Result: {len(result.failures)} FAILURES, {len(result.errors)} ERRORS")
    print("=" * 64)

    return 0 if result.wasSuccessful() else 1


def cleanup():
    print("\n[Cleanup] Shutting down Unreal Editor...")
    try:
        import unreal
        unreal.SystemLibrary.quit_game(unreal.EditorLevelLibrary.get_editor_world())
    except Exception:
        pass

    try:
        subprocess.run(
            ["taskkill", "/f", "/im", "UnrealEditor.exe"],
            capture_output=True, timeout=10
        )
        print("[Cleanup] Unreal Editor terminated.")
    except Exception as e:
        print(f"[Cleanup] Failed to kill UnrealEditor: {e}")


exit_code = 1
try:
    exit_code = main()
finally:
    cleanup()
