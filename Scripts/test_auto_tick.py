import unreal

def test_auto_tick():
    passed = 0
    failed = 0

    scene = unreal.UEMotionScene()
    scene.initialize("test_auto_tick", 1920, 1080)

    cube = scene.create_cube(50)
    cube.set_location([0, 0, 0])

    move = unreal.UEMotionMoveAnimation()
    move.set_target_mobject(cube)
    move.set_start([0, 0, 0])
    move.set_target([200, 0, 0])
    move.set_duration(1.0)

    scene.play(move)

    has_animations = scene.has_active_animations()
    if has_animations:
        unreal.log(f"[PASS] Animation is active after play()")
        passed += 1
    else:
        unreal.log(f"[FAIL] Animation should be active after play()")
        failed += 1

    import time
    time.sleep(1.2)

    loc_after_wait = cube.get_location()
    if loc_after_wait.x > 150:
        unreal.log(f"[PASS] Auto-Tick moved cube: x={loc_after_wait.x:.1f} (expected ~200)")
        passed += 1
    else:
        unreal.log(f"[FAIL] Auto-Tick did NOT move cube: x={loc_after_wait.x:.1f} (expected ~200)")
        failed += 1

    still_active = scene.has_active_animations()
    if not still_active:
        unreal.log(f"[PASS] Animation finished after duration")
        passed += 1
    else:
        unreal.log(f"[FAIL] Animation should have finished")
        failed += 1

    scene.destroy()

    unreal.log(f"\n=== Auto-Tick Test: {passed} passed, {failed} failed ===")
    return failed == 0

test_auto_tick()
