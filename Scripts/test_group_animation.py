import unreal

def test_group_animation():
    passed = 0
    failed = 0

    scene = unreal.UEMotionScene()
    scene.initialize("test_group", 1920, 1080)

    cube = scene.create_cube(50)
    cube.set_location([0, 0, 0])

    move1 = unreal.UEMotionMoveAnimation()
    move1.set_target_mobject(cube)
    move1.set_start([0, 0, 0])
    move1.set_target([100, 0, 0])
    move1.set_duration(1.0)

    move2 = unreal.UEMotionMoveAnimation()
    move2.set_target_mobject(cube)
    move2.set_start([100, 0, 0])
    move2.set_target([100, 100, 0])
    move2.set_duration(1.0)

    group = unreal.UEMotionGroupAnimation()
    group.set_play_mode(True)
    group.add_animation(move1)
    group.add_animation(move2)

    total_duration = group.get_duration()
    if abs(total_duration - 2.0) < 0.01:
        unreal.log(f"[PASS] Sequential total duration: {total_duration:.2f} (expected 2.0)")
        passed += 1
    else:
        unreal.log(f"[FAIL] Sequential total duration: {total_duration:.2f} (expected 2.0)")
        failed += 1

    scene.play(group)

    for i in range(5):
        scene.tick(0.1)

    loc_after_half_a = cube.get_location()
    if loc_after_half_a.x > 20:
        unreal.log(f"[PASS] After 0.5s: x={loc_after_half_a.x:.1f} (should be moving along X)")
        passed += 1
    else:
        unreal.log(f"[FAIL] After 0.5s: x={loc_after_half_a.x:.1f} (should be moving along X)")
        failed += 1

    for i in range(5):
        scene.tick(0.1)

    loc_after_a = cube.get_location()
    if loc_after_a.x > 80 and loc_after_a.y < 20:
        unreal.log(f"[PASS] After 1.0s: x={loc_after_a.x:.1f}, y={loc_after_a.y:.1f} (first anim finishing)")
        passed += 1
    else:
        unreal.log(f"[FAIL] After 1.0s: x={loc_after_a.x:.1f}, y={loc_after_a.y:.1f} (first anim finishing)")
        failed += 1

    for i in range(10):
        scene.tick(0.1)

    loc_after_b = cube.get_location()
    if loc_after_b.y > 80:
        unreal.log(f"[PASS] After 2.0s: x={loc_after_b.x:.1f}, y={loc_after_b.y:.1f} (second anim finishing)")
        passed += 1
    else:
        unreal.log(f"[FAIL] After 2.0s: x={loc_after_b.x:.1f}, y={loc_after_b.y:.1f} (second anim finishing)")
        failed += 1

    scene.stop_all()
    scene.destroy()

    scene2 = unreal.UEMotionScene()
    scene2.initialize("test_group2", 1920, 1080)

    cube2 = scene2.create_cube(50)
    cube2.set_location([0, 0, 0])

    move_a = unreal.UEMotionMoveAnimation()
    move_a.set_target_mobject(cube2)
    move_a.set_start([0, 0, 0])
    move_a.set_target([100, 0, 0])
    move_a.set_duration(1.0)

    move_b = unreal.UEMotionMoveAnimation()
    move_b.set_target_mobject(cube2)
    move_b.set_start([0, 0, 0])
    move_b.set_target([0, 100, 0])
    move_b.set_duration(1.0)

    parallel_group = unreal.UEMotionGroupAnimation()
    parallel_group.set_play_mode(False)
    parallel_group.add_animation(move_a)
    parallel_group.add_animation(move_b)

    scene2.play(parallel_group)

    for i in range(10):
        scene2.tick(0.1)

    loc_parallel = cube2.get_location()
    if loc_parallel.x > 50 and loc_parallel.y > 50:
        unreal.log(f"[PASS] Parallel: x={loc_parallel.x:.1f}, y={loc_parallel.y:.1f} (both moving)")
        passed += 1
    else:
        unreal.log(f"[FAIL] Parallel: x={loc_parallel.x:.1f}, y={loc_parallel.y:.1f} (both moving)")
        failed += 1

    scene2.destroy()

    unreal.log(f"\n=== GroupAnimation Test: {passed} passed, {failed} failed ===")
    return failed == 0

test_group_animation()
