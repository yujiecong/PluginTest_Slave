import unreal
import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN, RIGHT

s = Scene("visual_cube_check", 1920, 1080, mode="2d")
s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
s.point_light(location=ORIGIN, color="white", intensity=5000)

cube = s.cube(color="#3498db", location=ORIGIN)

if cube:
    ue_mob = cube._ue
    print(f"=== STEP 1: Cube created (no Sequencer) ===")
    print(f"  Opacity:  {ue_mob.get_opacity():.4f}")
    print(f"  Visible:  {ue_mob.get_visibility()}")

print("\n=== STEP 2: move_to + play (Sequencer binding) ===")
cube.move_to(RIGHT * 2, duration=2.0)
s.play()

if cube:
    ue_mob = cube._ue
    print(f"  Opacity:  {ue_mob.get_opacity():.4f}")
    print(f"  Visible:  {ue_mob.get_visibility()}")

print("\n=== STEP 3: fade_out + play (Opacity track) ===")
cube.fade_out(duration=2.0)
s.play()

# if cube:
#     ue_mob = cube._ue
#     print(f"  Opacity:  {ue_mob.get_opacity():.4f}")
#     print(f"  Visible:  {ue_mob.get_visibility()}")

# print("\n=== STEP 4: fade_in + play ===")
# cube.fade_in(duration=2.0)
# s.play()

# if cube:
#     ue_mob = cube._ue
#     print(f"  Opacity:  {ue_mob.get_opacity():.4f}")
#     print(f"  Visible:  {ue_mob.get_visibility()}")

# print("\nAll steps done.")
