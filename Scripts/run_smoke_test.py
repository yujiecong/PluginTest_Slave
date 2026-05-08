"""
UEMotion Smoke Test Runner

Launches UE Editor as a subprocess, waits for it to exit, then checks results.
Synchronous execution - no async waiting needed.
"""

import subprocess
import sys
import os
import glob
import time

UE_EDITOR = r"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
PROJECT_DIR = r"c:\Users\42458\Documents\Unreal Projects\PluginTest"
PROJECT = os.path.join(PROJECT_DIR, "PluginTest.uproject")
TEST_SCRIPT = os.path.join(PROJECT_DIR, "Scripts", "run_full_pipeline_test.py")
OUTPUT_DIR = os.path.join(PROJECT_DIR, "Saved", "UEMotionTest", "full_pipeline")
LOG_FILE = os.path.join(PROJECT_DIR, "Saved", "Logs", "PluginTest.log")

EXPECTED_FRAMES = 151
EXPECTED_MIN_FILE_SIZE = 400_000
EXPECTED_MAX_FILE_SIZE = 2_000_000


def clean_output():
    if os.path.isdir(OUTPUT_DIR):
        import shutil
        shutil.rmtree(OUTPUT_DIR)
        print("[CLEAN] Removed previous output")


def launch_ue():
    if not os.path.isfile(UE_EDITOR):
        print(f"[ERROR] UnrealEditor.exe not found: {UE_EDITOR}")
        sys.exit(1)

    if not os.path.isfile(TEST_SCRIPT):
        print(f"[ERROR] Test script not found: {TEST_SCRIPT}")
        sys.exit(1)

    cmd = [
        UE_EDITOR,
        PROJECT,
        "-ExecCmds=py " + TEST_SCRIPT,
        "-NoSound",
        "-Unattended",
        "-NoSplash",
    ]

    print("[LAUNCH] Starting UE5 Editor (synchronous)...")
    print(f"  Project : {PROJECT}")
    print(f"  Script  : {TEST_SCRIPT}")
    print()

    start_time = time.time()

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    exit_code = proc.wait()

    elapsed = time.time() - start_time
    print(f"\n[EXIT] UE Editor exited with code {exit_code} ({elapsed:.1f}s)")

    return exit_code


def validate_output():
    passed = 0
    failed = 0

    def check(condition, desc):
        nonlocal passed, failed
        if condition:
            print(f"  [PASS] {desc}")
            passed += 1
        else:
            print(f"  [FAIL] {desc}")
            failed += 1

    print("\n[VALIDATE] Checking output...")

    frames_dir = os.path.join(OUTPUT_DIR, "y_equals_x_frames").replace("\\", "/")
    check(os.path.isdir(frames_dir), f"Output directory exists: {frames_dir}")

    if os.path.isdir(frames_dir):
        png_files = sorted(glob.glob(os.path.join(frames_dir, "*.png")))
        frame_count = len(png_files)
        check(frame_count == EXPECTED_FRAMES,
              f"Frame count = {frame_count} (expected {EXPECTED_FRAMES})")

        if png_files:
            first = os.path.basename(png_files[0])
            last = os.path.basename(png_files[-1])
            check(first.startswith("LS_"), f"First frame: {first}")
            check(last.startswith("LS_"), f"Last frame: {last}")

            sizes_ok = True
            for f in png_files:
                size = os.path.getsize(f)
                if size < EXPECTED_MIN_FILE_SIZE or size > EXPECTED_MAX_FILE_SIZE:
                    sizes_ok = False
                    print(f"    Outlier: {os.path.basename(f)} = {size:,} bytes")
            check(sizes_ok, f"File sizes in range [{EXPECTED_MIN_FILE_SIZE:,}, {EXPECTED_MAX_FILE_SIZE:,}]")
    else:
        failed += 3

    return passed, failed


def check_log():
    if not os.path.isfile(LOG_FILE):
        print("[WARN] Log file not found")
        return

    with open(LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    if "FATAL" in content or "ACCESS_VIOLATION" in content:
        print("[CRASH] UE crashed during execution!")
        for line in content.splitlines():
            if "Fatal" in line or "ACCESS_VIOLATION" in line or "Error:" in line:
                if "UEMotion" in line or "Callstack" in line or "Fatal" in line:
                    print(f"  {line.strip()}")
    elif "ALL CHECKS PASSED" in content:
        print("[LOG] Python test script reported: ALL CHECKS PASSED")
    else:
        print("[LOG] Checking Python test results in log...")
        for line in content.splitlines():
            if "[PASS]" in line or "[FAIL]" in line:
                print(f"  {line.strip()}")


def main():
    print("=" * 60)
    print("  UEMotion Smoke Test Runner")
    print("=" * 60)
    print()

    clean_output()

    exit_code = launch_ue()

    check_log()

    passed, failed = validate_output()

    print()
    print("=" * 60)
    total = passed + failed
    if failed == 0 and exit_code == 0:
        print(f"  RESULT: ALL CHECKS PASSED ({passed}/{total})")
    else:
        print(f"  RESULT: FAILED ({passed}/{total} passed, {failed} failed, exit={exit_code})")
    print("=" * 60)

    sys.exit(0 if failed == 0 and exit_code == 0 else 1)


if __name__ == "__main__":
    main()
