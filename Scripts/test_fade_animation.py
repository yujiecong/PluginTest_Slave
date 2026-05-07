import unreal

def test_fade_animation():
    passed = 0
    failed = 0

    scene = unreal.UEMotionScene()
    scene.initialize(1920, 1080)

    sphere = scene.create_sphere(50)

    fade_out = unreal.UEMotionFadeAnimation()
    fade_out.set_target_mobject(sphere)
    fade_out.set_fade_out(True)
    fade_out.set_duration(1.0)

    initial_opacity = sphere.get_opacity()
    if abs(initial_opacity - 1.0) < 0.01:
        unreal.log(f"[PASS] Initial opacity: {initial_opacity:.2f}")
        passed += 1
    else:
        unreal.log(f"[FAIL] Initial opacity: {initial_opacity:.2f} (expected 1.0)")
        failed += 1

    scene.play(fade_out)

    for i in range(10):
        scene.tick(0.1)

    final_opacity = sphere.get_opacity()
    if final_opacity < 0.05:
        unreal.log(f"[PASS] FadeOut: opacity={final_opacity:.2f} (expected ~0)")
        passed += 1
    else:
        unreal.log(f"[FAIL] FadeOut: opacity={final_opacity:.2f} (expected ~0)")
        failed += 1

    scene.stop_all()

    sphere.set_opacity(1.0)

    fade_in = unreal.UEMotionFadeAnimation()
    fade_in.set_target_mobject(sphere)
    fade_in.set_fade_in(True)
    fade_in.set_duration(1.0)

    scene.play(fade_in)

    opacity_at_start = sphere.get_opacity()
    if opacity_at_start < 0.05:
        unreal.log(f"[PASS] FadeIn start: opacity={opacity_at_start:.2f} (expected ~0)")
        passed += 1
    else:
        unreal.log(f"[FAIL] FadeIn start: opacity={opacity_at_start:.2f} (expected ~0)")
        failed += 1

    for i in range(10):
        scene.tick(0.1)

    final_opacity_in = sphere.get_opacity()
    if final_opacity_in > 0.95:
        unreal.log(f"[PASS] FadeIn end: opacity={final_opacity_in:.2f} (expected ~1)")
        passed += 1
    else:
        unreal.log(f"[FAIL] FadeIn end: opacity={final_opacity_in:.2f} (expected ~1)")
        failed += 1

    mid_opacity = 0.5
    sphere.set_opacity(1.0)

    fade_half = unreal.UEMotionFadeAnimation()
    fade_half.set_target_mobject(sphere)
    fade_half.set_fade_out(True)
    fade_half.set_duration(1.0)

    scene.play(fade_half)

    for i in range(5):
        scene.tick(0.1)

    mid_val = sphere.get_opacity()
    if 0.3 < mid_val < 0.7:
        unreal.log(f"[PASS] FadeOut mid-point: opacity={mid_val:.2f} (expected ~0.5)")
        passed += 1
    else:
        unreal.log(f"[FAIL] FadeOut mid-point: opacity={mid_val:.2f} (expected ~0.5)")
        failed += 1

    scene.destroy()

    unreal.log(f"\n=== FadeAnimation Test: {passed} passed, {failed} failed ===")
    return failed == 0

test_fade_animation()
