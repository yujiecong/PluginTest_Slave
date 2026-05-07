import os
import re
import shutil

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

REPLACEMENTS_TEXT = [
    ("uemotion_y_equals_x", "uemotion_y_equals_x"),
    ("uemotion_dir", "uemotion_dir"),
    ("from uemotion import", "from uemotion import"),
    ("import uemotion", "import uemotion"),
    ("uemotion module not available", "uemotion module not available"),
    ("UEMotionPluginModule", "UEMotionPluginModule"),
    ("UEMotionPlugin.Build.cs", "UEMotionPlugin.Build.cs"),
    ("UEMotionPlugin.uplugin", "UEMotionPlugin.uplugin"),
    ("UEMotionPlugin", "UEMotionPlugin"),
    ("AUEMotionSceneActor", "AUEMotionSceneActor"),
    ("UEMotionBlueprintLibrary", "UEMotionBlueprintLibrary"),
    ("UEMotionCommandlet", "UEMotionCommandlet"),
    ("UEMotionSequence", "UEMotionSequence"),
    ("UEMotionRenderer", "UEMotionRenderer"),
    ("UEMotionGroupAnimation", "UEMotionGroupAnimation"),
    ("UEMotionColorAnimation", "UEMotionColorAnimation"),
    ("UEMotionFadeAnimation", "UEMotionFadeAnimation"),
    ("UEMotionScaleAnimation", "UEMotionScaleAnimation"),
    ("UEMotionRotateAnimation", "UEMotionRotateAnimation"),
    ("UEMotionMoveAnimation", "UEMotionMoveAnimation"),
    ("UEMotionWaitAnimation", "UEMotionWaitAnimation"),
    ("UEMotionAnimation", "UEMotionAnimation"),
    ("UEMotionCamera", "UEMotionCamera"),
    ("UEMotionMobject", "UEMotionMobject"),
    ("UEMotionScene", "UEMotionScene"),
    ("LogUEMotion", "LogUEMotion"),
    ("UEMotion|", "UEMotion|"),
    ("UEMotion", "UEMotion"),
    ("uemotion/", "uemotion/"),
    ("uemotion\\", "uemotion\\"),
    ("uemotion", "uemotion"),
]

RENAME_DIRS = [
    (os.path.join("Scripts", "uemotion"), os.path.join("Scripts", "uemotion")),
]

RENAME_FILES = [
    (os.path.join(".trae", "documents", "uemotion-automated-test-plan.md"),
     os.path.join(".trae", "documents", "uemotion-automated-test-plan.md")),
]

SKIP_DIRS = {
    ".git", "Binaries", "Intermediate", "Saved", "DerivedDataCache",
    ".vs", "x64", "Win64", "Build",
}

TEXT_EXTENSIONS = {
    ".py", ".bat", ".md", ".txt", ".json", ".ini", ".cfg",
    ".h", ".cpp", ".cs", ".uplugin", ".xml", ".yaml", ".yml",
    ".html", ".css", ".js", ".ts",
}


def should_skip_dir(dirname):
    return dirname in SKIP_DIRS


def is_text_file(filepath):
    _, ext = os.path.splitext(filepath)
    return ext.lower() in TEXT_EXTENSIONS


def replace_in_text(text):
    for old, new in REPLACEMENTS_TEXT:
        text = text.replace(old, new)
    return text


def process_file(filepath):
    try:
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            original = f.read()
    except Exception as e:
        print(f"  [SKIP] Cannot read: {filepath} ({e})")
        return False

    updated = replace_in_text(original)
    if updated == original:
        return False

    try:
        with open(filepath, "w", encoding="utf-8", errors="ignore") as f:
            f.write(updated)
        return True
    except Exception as e:
        print(f"  [ERROR] Cannot write: {filepath} ({e})")
        return False


def scan_and_replace(root):
    changed_files = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if not should_skip_dir(d)]

        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            if not is_text_file(filepath):
                continue
            if process_file(filepath):
                rel = os.path.relpath(filepath, root)
                changed_files.append(rel)
                print(f"  [OK] {rel}")

    return changed_files


def rename_directories(root):
    for old_rel, new_rel in RENAME_DIRS:
        old_path = os.path.join(root, old_rel)
        new_path = os.path.join(root, new_rel)
        if os.path.isdir(old_path):
            if os.path.exists(new_path):
                for item in os.listdir(old_path):
                    src = os.path.join(old_path, item)
                    dst = os.path.join(new_path, item)
                    if os.path.exists(dst):
                        os.remove(dst)
                    shutil.move(src, dst)
                os.rmdir(old_path)
                print(f"  [MERGE] {old_rel} -> {new_rel} (merged contents)")
            else:
                shutil.move(old_path, new_path)
                print(f"  [RENAME] {old_rel} -> {new_rel}")
        else:
            print(f"  [SKIP] {old_rel} does not exist")


def rename_files(root):
    for old_rel, new_rel in RENAME_FILES:
        old_path = os.path.join(root, old_rel)
        new_path = os.path.join(root, new_rel)
        if os.path.isfile(old_path):
            os.makedirs(os.path.dirname(new_path), exist_ok=True)
            shutil.move(old_path, new_path)
            print(f"  [RENAME] {old_rel} -> {new_rel}")
        else:
            print(f"  [SKIP] {old_rel} does not exist")


def main():
    print("=" * 64)
    print("  Rename: uemotion -> uemotion / UEMotion -> UEMotion")
    print(f"  Project root: {PROJECT_ROOT}")
    print("=" * 64)
    print()

    print("[1/3] Scanning and replacing text in files...")
    changed = scan_and_replace(PROJECT_ROOT)
    print(f"\n  Total files modified: {len(changed)}")
    print()

    print("[2/3] Renaming directories...")
    rename_directories(PROJECT_ROOT)
    print()

    print("[3/3] Renaming files...")
    rename_files(PROJECT_ROOT)
    print()

    print("=" * 64)
    print("  Done!")
    print("=" * 64)


if __name__ == "__main__":
    main()
