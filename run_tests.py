#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
UEMotion 测试运行器

使用 -ExecCmds 方式调用 UE 执行 Python 测试脚本，
自动清理临时编译产物。

使用方法：
    python run_tests.py [--clean] [--dry-run]

参数：
    --clean    运行前清理 Intermediate/Binaries/Packaged 临时产物
    --dry-run  仅显示将要执行的命令，不实际运行
"""

import os
import sys
import shutil
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent
PLUGIN_DIR = PROJECT_ROOT / "Plugins" / "UEMotionPlugin"
TEST_SCRIPT = PROJECT_ROOT / "Scripts" / "tests" / "run_all_tests.py"

UE_EDITOR = r"C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe"
PROJECT_FILE = str(PROJECT_ROOT / "PluginTest.uproject")

TEMP_DIRS = [
    PLUGIN_DIR / "Intermediate",
    PLUGIN_DIR / "Binaries",
    PLUGIN_DIR / "Packaged",
    PROJECT_ROOT / "Intermediate",
]


def clean_temp_dirs(dry_run=False):
    print("\n[清理] 删除临时编译产物...")
    for d in TEMP_DIRS:
        if d.exists():
            if dry_run:
                print(f"  [DRY-RUN] 将删除: {d.relative_to(PROJECT_ROOT)}")
            else:
                try:
                    shutil.rmtree(d)
                    print(f"  ✓ 已删除: {d.relative_to(PROJECT_ROOT)}")
                except Exception as e:
                    print(f"  ✗ 删除失败: {d.relative_to(PROJECT_ROOT)} ({e})")
        else:
            print(f"  - 跳过: {d.relative_to(PROJECT_ROOT)} (不存在)")


def run_tests(dry_run=False):
    ue_path = UE_EDITOR
    project_path = PROJECT_FILE
    py_path = str(TEST_SCRIPT).replace("\\", "/")

    ue_cmd = '"{}" "{}" -ExecCmds="py \'{}\'" -TestExit -NoSound -Unattended -NoSplash'.format(
        ue_path, project_path, py_path
    )

    print("\n" + "=" * 60)
    print("  UEMotion Test Runner")
    print("=" * 60)
    print(f"\n  UE Editor : {ue_path}")
    print(f"  Project   : {project_path}")
    print(f"  Test      : {py_path}")
    print(f"\n  Command:")
    print(f"  {ue_cmd}")
    print()

    if dry_run:
        print("  [DRY-RUN] 仅显示命令，不执行")
        return 0

    try:
        result = subprocess.run(
            ue_cmd,
            shell=True,
            cwd=str(PROJECT_ROOT),
        )
        exit_code = result.returncode
    except Exception as e:
        print(f"\n  ✗ 执行失败: {e}")
        exit_code = -1

    print("\n" + "=" * 60)
    if exit_code == 0:
        print("  ✅ TESTS PASSED")
    else:
        print(f"  ❌ TESTS FAILED (Exit Code: {exit_code})")
    print("=" * 60)

    return exit_code


def main():
    dry_run = "--dry-run" in sys.argv
    do_clean = "--clean" in sys.argv

    if do_clean:
        clean_temp_dirs(dry_run)

    exit_code = run_tests(dry_run)
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
