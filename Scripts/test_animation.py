import unreal

def test_full_animation():
    print("[TEST] Starting Phase 2 Animation test...")

    scene = unreal.PyManimScene()
    scene.initialize(1920, 1080)

    scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 8)
    camera = scene.get_camera()
    camera.set_position(-350, -450, 250)
    camera.look_at(unreal.Vector(0, 0, 0))

    sphere = scene.create_sphere(50)
    sphere.set_color(unreal.LinearColor(1, 0.3, 0.3, 1))

    move = unreal.PyManimMoveAnimation()
    move.set_target_mobject(sphere)
    move.set_target(unreal.Vector(250, 0, 50))
    move.set_duration(2.0)
    move.set_easing("ease_in_out")

    cube = scene.create_cube(60)
    cube.set_color(unreal.LinearColor(0.3, 0.5, 1, 1))

    rotate = unreal.PyManimRotateAnimation()
    rotate.set_target_mobject(cube)
    rotate.set_rotation_angle(720)
    rotate.set_axis(unreal.Vector(0, 0, 1))
    rotate.set_duration(4.0)

    group = unreal.PyManimGroupAnimation()
    group.add_animation(move)
    group.add_animation(rotate)
    group.set_play_mode(False)

    scene.play(group)

    for frame in range(121):
        scene.tick(1.0 / 30.0)
        if frame % 30 == 0:
            s_loc = sphere.get_location()
            print(f"Frame {frame}: sphere_x={s_loc.x:.1f}")

    print("[PASS] Phase 2 Animation test complete!")

if __name__ == "__main__":
    test_full_animation()
