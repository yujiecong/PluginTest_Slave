import unreal
import sys
import os

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPTS_DIR)

from uemotion import Scene

print("=" * 64)
print("  UEMotion: Test Level + Sequence Creation")
print("=" * 64)
print()

print("[1/3] Creating scene...")
s = Scene("test_level_seq", 1920, 1080)

print(f"  Scene name: {s.name}")
print(f"  Scene world: {s.scene_world}")
print(f"  Level sequence: {s.level_sequence}")

ls = s.level_sequence
if ls:
    try:
        rate = ls.get_display_rate()
        print(f"  Display rate: {rate.numerator}/{rate.denominator}")
    except Exception as e:
        print(f"  Could not get display rate: {e}")
    try:
        pr = ls.get_playback_range()
        print(f"  Playback range: {pr.start_frame} - {pr.end_frame}")
    except Exception as e:
        print(f"  Could not get playback range: {e}")

print()
print("[2/3] Creating cube and animating...")
box = s.cube(30, color="cyan", location=(0, 0, 50))
if box:
    print(f"  Cube created at {box.location}")
    box.move_to((100, 100, 50), duration=1.0, easing="linear")
    s.play()
    print(f"  Animation recorded to LevelSequence")
else:
    print("  FAILED: Cube not created")

print()
print("[3/3] Saving assets...")
s.save_assets()

print()
print("=" * 64)
print("  Check Content Browser:")
print("    /Game/UEMotion/Scenes/test_level_seq")
print("    /Game/UEMotion/Sequences/LS_test_level_seq")
print("=" * 64)
