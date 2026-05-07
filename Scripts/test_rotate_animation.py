import unreal
import math

def test_rotate_animation():
    passed = 0
    failed = 0

    scene = unreal.UEMotionScene()
    scene.initialize(1920, 1080)

    cube = scene.create_cube(50)

    rotate = unreal.UEMotionRotateAnimation()
    rotate.set_target_mobject(cube)
    rotate.set_rotation_angle(90.0)
    rotate.set_axis([0, 0, 1])
    rotate.set_duration(1.0)

    scene.play(rotate)

    for i in range(10):
        scene.tick(0.1)

    final_rot = cube.get_rotation()
    yaw_val = final_rot.z

    if abs(yaw_val - 90.0) < 5.0:
        unreal.log(f"[PASS] RotateAnimation Z-axis: yaw={yaw_val:.1f} (expected ~90)")
        passed += 1
    else:
        unreal.log(f"[FAIL] RotateAnimation Z-axis: yaw={yaw_val:.1f} (expected ~90)")
        failed += 1

    scene.stop_all()

    rotate2 = unreal.UEMotionRotateAnimation()
    rotate2.set_target_mobject(cube)
    rotate2.set_rotation_angle(360.0)
    rotate2.set_axis([1, 0, 0])
    rotate2.set_duration(1.0)

    cube.set_rotation(unreal.Rotator(0, 0, 0))
    scene.play(rotate2)

    for i in range(10):
        scene.tick(0.1)

    final_rot2 = cube.get_rotation()
    pitch_val = final_rot2.x

    if abs(pitch_val) < 10.0 or abs(pitch_val - 360.0) < 10.0:
        unreal.log(f"[PASS] RotateAnimation X-axis 360: pitch={pitch_val:.1f} (expected ~0 or ~360)")
        passed += 1
    else:
        unreal.log(f"[FAIL] RotateAnimation X-axis 360: pitch={pitch_val:.1f} (expected ~0 or ~360)")
        failed += 1

    scene.destroy()

    unreal.log(f"\n=== RotateAnimation Test: {passed} passed, {failed} failed ===")
    return failed == 0

test_rotate_animation()
