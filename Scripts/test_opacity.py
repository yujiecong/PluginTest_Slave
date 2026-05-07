import unreal

def test_opacity():
    passed = 0
    failed = 0

    scene = unreal.UEMotionScene()
    scene.initialize(1920, 1080)

    sphere = scene.create_sphere(50)

    initial_opacity = sphere.get_opacity()
    if abs(initial_opacity - 1.0) < 0.01:
        unreal.log(f"[PASS] Default opacity: {initial_opacity:.2f}")
        passed += 1
    else:
        unreal.log(f"[FAIL] Default opacity: {initial_opacity:.2f} (expected 1.0)")
        failed += 1

    sphere.set_opacity(0.5)
    if abs(sphere.get_opacity() - 0.5) < 0.01:
        unreal.log(f"[PASS] Set opacity 0.5: {sphere.get_opacity():.2f}")
        passed += 1
    else:
        unreal.log(f"[FAIL] Set opacity 0.5: {sphere.get_opacity():.2f}")
        failed += 1

    sphere.set_opacity(0.0)
    if abs(sphere.get_opacity() - 0.0) < 0.01:
        unreal.log(f"[PASS] Set opacity 0.0: {sphere.get_opacity():.2f}")
        passed += 1
    else:
        unreal.log(f"[FAIL] Set opacity 0.0: {sphere.get_opacity():.2f}")
        failed += 1

    sphere.set_opacity(-1.0)
    if abs(sphere.get_opacity() - 0.0) < 0.01:
        unreal.log(f"[PASS] Clamped negative opacity: {sphere.get_opacity():.2f}")
        passed += 1
    else:
        unreal.log(f"[FAIL] Clamped negative opacity: {sphere.get_opacity():.2f}")
        failed += 1

    sphere.set_opacity(2.0)
    if abs(sphere.get_opacity() - 1.0) < 0.01:
        unreal.log(f"[PASS] Clamped over-1 opacity: {sphere.get_opacity():.2f}")
        passed += 1
    else:
        unreal.log(f"[FAIL] Clamped over-1 opacity: {sphere.get_opacity():.2f}")
        failed += 1

    sphere.set_opacity(1.0)

    cube = scene.create_cube(50)
    cube.set_color(unreal.LinearColor(1.0, 0.0, 0.0, 1.0))
    cube.set_opacity(0.7)

    if abs(cube.get_opacity() - 0.7) < 0.01:
        unreal.log(f"[PASS] Cube opacity after color+opacity: {cube.get_opacity():.2f}")
        passed += 1
    else:
        unreal.log(f"[FAIL] Cube opacity after color+opacity: {cube.get_opacity():.2f}")
        failed += 1

    scene.destroy()

    unreal.log(f"\n=== Opacity Test: {passed} passed, {failed} failed ===")
    return failed == 0

test_opacity()
