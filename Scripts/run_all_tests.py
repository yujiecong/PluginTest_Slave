import unreal

def run_all_tests():
    all_passed = True

    unreal.log("=" * 60)
    unreal.log("UEMotion Plugin - Full Test Suite")
    unreal.log("=" * 60)

    unreal.log("\n--- Test 1: Rotate Animation ---")
    try:
        exec(open("Scripts/test_rotate_animation.py").read())
    except Exception as e:
        unreal.log(f"[ERROR] RotateAnimation test crashed: {e}")
        all_passed = False

    unreal.log("\n--- Test 2: Fade Animation ---")
    try:
        exec(open("Scripts/test_fade_animation.py").read())
    except Exception as e:
        unreal.log(f"[ERROR] FadeAnimation test crashed: {e}")
        all_passed = False

    unreal.log("\n--- Test 3: Group Animation ---")
    try:
        exec(open("Scripts/test_group_animation.py").read())
    except Exception as e:
        unreal.log(f"[ERROR] GroupAnimation test crashed: {e}")
        all_passed = False

    unreal.log("\n--- Test 4: Auto Tick ---")
    try:
        exec(open("Scripts/test_auto_tick.py").read())
    except Exception as e:
        unreal.log(f"[ERROR] Auto-Tick test crashed: {e}")
        all_passed = False

    unreal.log("\n--- Test 5: Opacity ---")
    try:
        exec(open("Scripts/test_opacity.py").read())
    except Exception as e:
        unreal.log(f"[ERROR] Opacity test crashed: {e}")
        all_passed = False

    unreal.log("\n" + "=" * 60)
    if all_passed:
        unreal.log("ALL TESTS COMPLETED")
    else:
        unreal.log("SOME TESTS HAD ERRORS - check logs above")
    unreal.log("=" * 60)

run_all_tests()
