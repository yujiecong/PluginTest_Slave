#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PyManim → UEMotion 全局重命名脚本 (v2.0)

功能：
1. 替换所有源代码文件中的 PyManim → UEMotion (内容)
2. 重命名文件名中的 PyManim → UEMotion
3. 重命名目录名中的 PyManimPlugin → UEMotionPlugin
4. 更新 .uplugin 配置文件
5. 更新 Python 脚本中的引用
6. 清理 Intermediate/Build 目录以便重新编译

使用方法：
    python rename_to_uemotion.py [--dry-run] [--force]

参数：
    --dry-run  仅显示将要执行的操作，不实际修改任何文件
    --force    强制执行，自动处理目录冲突（合并后删除旧目录）
"""

import os
import re
import sys
import shutil
from pathlib import Path
from datetime import datetime

PROJECT_ROOT = Path(r"c:\Users\42458\Documents\Unreal Projects\PluginTest")
PLUGIN_ROOT = PROJECT_ROOT / "Plugins" / "PyManimPlugin"
SCRIPTS_ROOT = PROJECT_ROOT / "Scripts"

REPLACE_PATTERNS = [
    ("PyManim", "UEMotion"),
    ("pymanim", "uemotion"),
    ("PYMANIM", "UEMOTION"),
]

EXTENSIONS_TO_PROCESS = {
    ".cpp", ".h", ".cs", ".uplugin", ".build.cs",
    ".py", ".bat", ".json", ".md",
}

DIR_RENAME_MAP = {
    "PyManimPlugin": "UEMotionPlugin",
    "pymanim": "uemotion",
}

EXCLUDE_DIRS = {
    "Intermediate", "Binaries", ".git", "__pycache__",
    "Saved", "DerivedDataCache", ".trae",
}


def log(message: str, level: str = "INFO"):
    """统一日志输出"""
    timestamp = datetime.now().strftime("%H:%M:%S")
    print(f"  [{timestamp}][{level}] {message}")


def should_process_file(filepath: Path) -> bool:
    if not filepath.is_file():
        return False
    ext = filepath.suffix.lower()
    return ext in EXTENSIONS_TO_PROCESS


def should_process_dir(dirpath: Path) -> bool:
    name = dirpath.name
    return name not in EXCLUDE_DIRS and not name.startswith(".")


def replace_in_file(filepath: Path, dry_run: bool = False) -> int:
    """替换文件内容中的 PyManim → UEMotion，返回替换次数"""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception as e:
        log(f"无法读取 {filepath.name}: {e}", "WARN")
        return 0

    new_content = content
    total_replacements = 0

    for old, new in REPLACE_PATTERNS:
        count = new_content.count(old)
        if count > 0:
            new_content = new_content.replace(old, new)
            total_replacements += count

    if total_replacements > 0:
        rel_path = filepath.relative_to(PROJECT_ROOT)
        if dry_run:
            log(f"[DRY-RUN] 将修改 {rel_path} ({total_replacements} 处)", "INFO")
        else:
            try:
                filepath.write_text(new_content, encoding="utf-8")
                log(f"✓ 已修改 {rel_path} ({total_replacements} 处)", "OK")
            except Exception as e:
                log(f"写入失败 {rel_path}: {e}", "ERROR")

    return total_replacements


def rename_file_safe(filepath: Path, dry_run: bool = False) -> bool:
    """安全地重命名文件"""
    old_name = filepath.name
    new_name = old_name

    for old, new in REPLACE_PATTERNS:
        if old in new_name:
            new_name = new_name.replace(old, new)

    if new_name != old_name:
        new_path = filepath.parent / new_name
        rel_old = filepath.relative_to(PROJECT_ROOT)

        if new_path.exists():
            if dry_run:
                log(f"[DRY-RUN] 文件已存在，将覆盖: {new_name}", "WARN")
            else:
                log(f"目标文件已存在，跳过: {new_path}", "WARN")
            return False

        if dry_run:
            log(f"[DRY-RUN] 重命名: {old_name} → {new_name}", "INFO")
        else:
            try:
                filepath.rename(new_path)
                log(f"✓ 重命名: {old_name} → {new_name}", "OK")
                return True
            except Exception as e:
                log(f"重命名失败 {old_name}: {e}", "ERROR")
    return False


def rename_directory_safe(dirpath: Path, dry_run: bool = False, force: bool = False) -> bool:
    """安全地重命名目录（带冲突检测）"""
    old_name = dirpath.name
    new_name = old_name

    for old, new in DIR_RENAME_MAP.items():
        if old in new_name:
            new_name = new_name.replace(old, new)

    if new_name == old_name:
        return False

    new_path = dirpath.parent / new_name
    rel_path = dirpath.relative_to(PROJECT_ROOT)

    if new_path.exists():
        if force:
            if dry_run:
                log(f"[DRY-RUN] 目标目录已存在，强制模式将合并后删除: {new_name}", "WARN")
            else:
                log(f"⚠ 目标目录 {new_name} 已存在，尝试合并...", "WARN")

                try:
                    import filecmp

                    def merge_dirs(src, dst):
                        for item in src.iterdir():
                            target = dst / item.name
                            if item.is_dir():
                                if not target.exists():
                                    shutil.copytree(str(item), str(target))
                                else:
                                    merge_dirs(item, target)
                            else:
                                if not target.exists() or not filecmp.cmp(str(item), str(target), shallow=False):
                                    shutil.copy2(str(item), str(target))

                    merge_dirs(dirpath, new_path)
                    shutil.rmtree(str(dirpath))
                    log(f"✓ 合并完成并删除旧目录: {old_name}", "OK")
                    return True
                except Exception as e:
                    log(f"合并失败: {e}", "ERROR")
                    return False
        else:
            if dry_run:
                log(f"[DRY-RUN] ⚠ 冲突: 目录 {new_name} 已存在 (使用 --force 解决)", "WARN")
            else:
                log(f"✗ 跳过: 目标目录 {new_name} 已存在 (使用 --force 覆盖)", "ERROR")
            return False

    if dry_run:
        log(f"[DRY-RUN] 重命名目录: {rel_path} → {new_name}", "INFO")
    else:
        try:
            dirpath.rename(new_path)
            log(f"✓ 重命名目录: {old_name} → {new_name}", "OK")
            return True
        except Exception as e:
            log(f"重命名目录失败 {old_name}: {e}", "ERROR")

    return False


def process_files_in_directory(root: Path, dry_run: bool = False):
    """递归处理目录下的所有文件（不重命名目录）"""
    total_replacements = 0

    for dirpath, dirs, files in os.walk(root):
        dir_path = Path(dirpath)

        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS and not d.startswith(".")]

        for filename in sorted(files):
            filepath = dir_path / filename
            if should_process_file(filepath):
                total_replacements += replace_in_file(filepath, dry_run)

    return total_replacements


def process_all_renames(root: Path, dry_run: bool = False, force: bool = False):
    """处理所有文件和目录的重命名（从深层到浅层）"""

    all_dirs = []

    for dirpath, dirs, files in os.walk(root):
        dir_path = Path(dirpath)
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        all_dirs.append(dir_path)

    sorted_dirs = sorted(all_dirs, key=lambda x: len(x.parts), reverse=True)

    for dir_path in sorted_dirs:
        dirname = dir_path.name
        if dirname in DIR_RENAME_MAP or any(old in dirname for old in DIR_RENAME_MAP.keys()):
            rename_directory_safe(dir_path, dry_run, force)


def clean_intermediate(dry_run: bool = False):
    """清理 Intermediate 和 Binaries 目录以强制重新编译"""

    plugin_paths = [
        PROJECT_ROOT / "Plugins" / "PyManimPlugin",
        PROJECT_ROOT / "Plugins" / "UEMotionPlugin",
    ]

    for plugin_base in plugin_paths:
        if not plugin_base.exists():
            continue

        for subdir in ["Intermediate", "Binaries"]:
            target_dir = plugin_base / subdir
            if target_dir.exists():
                rel_path = target_dir.relative_to(PROJECT_ROOT)
                if dry_run:
                    log(f"[DRY-RUN] 将删除: {rel_path}", "INFO")
                else:
                    try:
                        shutil.rmtree(target_dir)
                        log(f"✓ 已删除: {rel_path}", "OK")
                    except Exception as e:
                        log(f"删除失败 {rel_path}: {e}", "ERROR")


def update_uplugin(dry_run: bool = False):

    plugin_bases = [
        PROJECT_ROOT / "Plugins" / "PyManimPlugin",
        PROJECT_ROOT / "Plugins" / "UEMotionPlugin",
    ]

    uplugin_path = None
    for base in plugin_bases:
        candidate = base / "PyManimPlugin.uplugin"
        if candidate.exists():
            uplugin_path = candidate
            break
        candidate = base / "UEMotionPlugin.uplugin"
        if candidate.exists():
            uplugin_path = candidate
            break

    if not uplugin_path:
        log("未找到 .uplugin 文件", "WARN")
        return

    try:
        content = uplugin_path.read_text(encoding="utf-8")
    except Exception as e:
        log(f"读取 .uplugin 失败: {e}", "ERROR")
        return

    updates = {
        '"VersionName": "0.1.0"': '"VersionName": "0.2.0"',
        '"VersionName": "0.2.0"': '"VersionName": "0.2.0"',
        '"FriendlyName": "PyManim Plugin"': '"FriendlyName": "UEMotion Plugin"',
        '"Description": "Python-driven 3D rendering framework for UE5 (Manim-like)"':
            '"Description": "Python-driven animation framework for Unreal Engine 5"',
        '"Name": "PyManimPlugin"': '"Name": "UEMotionPlugin"',
    }

    modified = False
    for old, new in updates.items():
        if old in content:
            content = content.replace(old, new)
            modified = True

    if modified:

        if dry_run:
            log(f"[DRY-RUN] 将更新: {uplugin_path.name}", "INFO")
        else:
            try:
                uplugin_path.write_text(content, encoding="utf-8")
                log(f"✓ 已更新: {uplugin_path.name}", "OK")
            except Exception as e:
                log(f"更新 .uplugin 失败: {e}", "ERROR")


def rename_uplugin_file(dry_run: bool = False, force: bool = False):

    plugin_bases = [
        PROJECT_ROOT / "Plugins" / "PyManimPlugin",
        PROJECT_ROOT / "Plugins" / "UEMotionPlugin",
    ]

    for base in plugin_bases:
        old_plugin = base / "PyManimPlugin.uplugin"
        new_plugin = base / "UEMotionPlugin.uplugin"

        if old_plugin.exists() and not new_plugin.exists():
            if dry_run:
                log(f"[DRY-RUN] 重命名: PyManimPlugin.uplugin → UEMotionPlugin.uplugin", "INFO")
            else:
                try:
                    old_plugin.rename(new_plugin)
                    log(f"✓ 重命名: PyManimPlugin.uplugin → UEMotionPlugin.uplugin", "OK")
                except Exception as e:
                    log(f"重命名 .uplugin 失败: {e}", "ERROR")
        elif old_plugin.exists() and new_plugin.exists():
            if force:
                if dry_run:
                    log(f"[DRY-RUN] 删除旧的 PyManimPlugin.uplugin", "WARN")
                else:
                    try:
                        old_plugin.unlink()
                        log(f"✓ 已删除旧的: PyManimPlugin.uplugin", "OK")
                    except Exception as e:
                        log(f"删除失败: {e}", "ERROR")


def main():
    dry_run = "--dry-run" in sys.argv
    force = "--force" in sys.argv

    print("\n" + "=" * 70)
    if dry_run:
        print("  🔍 DRY RUN 模式 - 不会实际修改任何文件")
    else:
        mode_str = " (FORCE)" if force else ""
        print(f"  🚀 开始执行 PyManim → UEMotion 全局重命名{mode_str}")
        if not force:
            print("  💡 提示: 使用 --force 参数可自动解决目录冲突")
    print("=" * 70 + "\n")

    stats = {"files_modified": 0, "dirs_renamed": 0, "errors": 0}

    print("[步骤 1/7] 处理插件 C++ 源代码内容...")
    source_dir = PLUGIN_ROOT / "Source"
    alt_source_dir = PROJECT_ROOT / "Plugins" / "UEMotionPlugin" / "Source"

    actual_source = None
    for sd in [source_dir, alt_source_dir]:
        if sd.exists():
            actual_source = sd
            break

    if actual_source:
        count = process_files_in_directory(actual_source, dry_run)
        stats["files_modified"] += count
        log(f"完成，共修改 {count} 个文件", "INFO")
    else:
        log("未找到 Source 目录", "WARN")

    print("\n[步骤 2/7] 更新 .uplugin 配置...")
    update_uplugin(dry_run)

    print("\n[步骤 3/7] 处理 Python 脚本...")
    if SCRIPTS_ROOT.exists():
        count = process_files_in_directory(SCRIPTS_ROOT, dry_run)
        stats["files_modified"] += count
        log(f"完成，共修改 {count} 个文件", "INFO")
    else:
        log("Scripts 目录不存在", "WARN")

    print("\n[步骤 4/7] 处理根目录批处理脚本和配置...")
    root_files = list(PROJECT_ROOT.glob("*.bat")) + list(PROJECT_ROOT.glob("*.json"))
    for rf in root_files:
        if should_process_file(rf):
            count = replace_in_file(rf, dry_run)
            stats["files_modified"] += count

    print("\n[步骤 5/7] 重命名插件根目录...")

    plugin_root_candidates = [
        PROJECT_ROOT / "Plugins" / "PyManimPlugin",
    ]

    for pr in plugin_root_candidates:
        if pr.exists() and pr.name == "PyManimPlugin":
            if rename_directory_safe(pr, dry_run, force):
                stats["dirs_renamed"] += 1

    print("\n[步骤 6/7] 重命名子目录和文件...")

    source_roots = []
    for pb in [
        PROJECT_ROOT / "Plugins" / "PyManimPlugin",
        PROJECT_ROOT / "Plugins" / "UEMotionPlugin",
    ]:
        if pb.exists():
            sr = pb / "Source"
            if sr.exists():
                source_roots.append(sr)

    for src_root in source_roots:
        process_all_renames(src_root, dry_run, force)

    print("\n[步骤 7/7] 清理编译缓存...")
    clean_intermediate(dry_run)

    print("\n" + "=" * 70)
    if dry_run:
        print("  ✅ DRY RUN 完成！以上为将要执行的操作。")
        print("  📌 如需实际执行，请运行:")
        print("     python rename_to_uemotion.py")
        print("     或 (如有冲突): python rename_to_uemotion.py --force")
    else:
        print(f"  ✅ 重命名完成！")
        print(f"\n  📊 统计信息:")
        print(f"     - 修改文件数: {stats['files_modified']}")
        print(f"     - 重命名目录数: {stats['dirs_renamed']}")
        print(f"\n  📋 后续步骤:")
        print(f"     1. 在 Unreal Engine 中重新打开项目")
        print(f"     2. 等待插件重新编译（首次可能需要几分钟）")
        print(f"     3. 运行测试验证: run_tests.bat")
        print(f"\n  🔄 变更摘要:")
        print(f"     插件目录: PyManimPlugin → UEMotionPlugin")
        print(f"     C++ 类名: UPyManim* → UEMotion*")
        print(f"     Python API: unreal.PyManim* → unreal.UEMotion*")
        print(f"     模块名: pymanim → uemotion")
    print("=" * 70 + "\n")


if __name__ == "__main__":
    main()
