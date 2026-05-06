import unittest
import unreal
from test_framework import PyManimTestCase


class TestMobject(PyManimTestCase):
    def test_set_get_location(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_location(unreal.Vector(100, 200, 300))
        loc = obj.get_location()
        self.assertNear(loc.x, 100, 0.1)
        self.assertNear(loc.y, 200, 0.1)
        self.assertNear(loc.z, 300, 0.1)
        self.cleanup_scene(scene)

    def test_set_get_color(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_color(unreal.LinearColor(0.5, 0.3, 0.8, 1.0))
        c = obj.get_color()
        self.assertNear(c.r, 0.5, 0.01)
        self.assertNear(c.g, 0.3, 0.01)
        self.assertNear(c.b, 0.8, 0.01)
        self.cleanup_scene(scene)

    def test_set_get_visibility(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        self.assertTrue(obj.get_visibility())
        obj.set_visibility(False)
        self.assertFalse(obj.get_visibility())
        obj.set_visibility(True)
        self.assertTrue(obj.get_visibility())
        self.cleanup_scene(scene)

    def test_set_get_scale(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_scale(unreal.Vector(2, 2, 2))
        s = obj.get_scale()
        self.assertNear(s.x, 2, 0.01)
        self.assertNear(s.y, 2, 0.01)
        self.assertNear(s.z, 2, 0.01)
        self.cleanup_scene(scene)

    def test_set_get_rotation(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        test_rotation = unreal.Rotator(45, 90, 0)
        obj.set_rotation(test_rotation)
        r = obj.get_rotation()
        self.assertIsNotNone(r)
        has_rotation = (abs(r.pitch) > 0 or abs(r.yaw) > 0 or abs(r.roll) > 0)
        self.assertTrue(has_rotation, "Object should have non-zero rotation after setting")
        self.cleanup_scene(scene)

    def test_get_name(self):
        scene = self.make_scene()
        self.assertEqual(scene.create_sphere(50).get_name(), "Sphere")
        self.assertEqual(scene.create_cube(50).get_name(), "Cube")
        self.cleanup_scene(scene)

    def test_destroy(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        self.assertNoCrash(lambda: obj.destroy())
        self.cleanup_scene(scene)

    def test_all_shapes(self):
        scene = self.make_scene()
        shapes = [
            scene.create_sphere(50), scene.create_cube(50),
            scene.create_cylinder(50, 100), scene.create_cone(50, 100),
            scene.create_plane(500, 500), scene.create_torus(80, 25),
        ]
        for s in shapes:
            self.assertIsNotNone(s)
            s.set_color(unreal.LinearColor(1, 0, 0, 1))
            s.set_location(unreal.Vector(100, 0, 0))
            s.set_scale(unreal.Vector(1.5, 1.5, 1.5))
            s.set_rotation(unreal.Rotator(0, 45, 0))
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
