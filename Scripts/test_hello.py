import unreal

def test_hello_sphere():
    print("[TEST] Starting Hello Sphere test...")

    scene = unreal.PyManimScene()
    assert scene is not None, "Scene creation failed"

    scene.initialize(1920, 1080)
    assert scene.is_initialized(), "Scene initialization failed"

    sphere = scene.create_sphere(80)
    assert sphere is not None, "Sphere creation failed"
    sphere.set_color(unreal.LinearColor(1, 0.2, 0.2, 1))
    sphere.set_location(unreal.Vector(0, 0, 50))

    cube = scene.create_cube(60)
    assert cube is not None, "Cube creation failed"
    cube.set_color(unreal.LinearColor(0.2, 0.4, 1, 1))
    cube.set_location(unreal.Vector(150, 0, 0))

    plane = scene.create_plane(400)
    assert plane is not None, "Plane creation failed"
    plane.set_location(unreal.Vector(0, 0, -100))
    plane.set_color(unreal.LinearColor(0.15, 0.15, 0.15, 1))

    cylinder = scene.create_cylinder(40, 120)
    assert cylinder is not None, "Cylinder creation failed"
    cylinder.set_color(unreal.LinearColor(0.2, 0.8, 0.3, 1))
    cylinder.set_location(unreal.Vector(-150, 0, 0))

    scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 8)

    all_objs = scene.get_all_mobjects()
    assert len(all_objs) >= 4, f"Not enough mobjects: {len(all_objs)}"

    print(f"[PASS] Created {len(all_objs)} objects")
    print("[PASS] Hello Sphere test complete!")

if __name__ == "__main__":
    test_hello_sphere()
