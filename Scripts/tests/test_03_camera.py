import unittest
import math
import unreal
from test_framework import UEMotionTestCase


class TestCamera(UEMotionTestCase):
    def test_set_get_position(self):
        scene = self.make_scene()
        cam = scene.get_camera()
        cam.set_position(100, 200, 300)
        pos = cam.get_position()
        self.assertNear(pos.x, 100, 0.1)
        self.assertNear(pos.y, 200, 0.1)
        self.assertNear(pos.z, 300, 0.1)
        self.cleanup_scene(scene)

    def test_set_get_rotation(self):
        scene = self.make_scene()
        cam = scene.get_camera()
        cam.set_rotation(30, 60, 0)
        rot = cam.get_rotation()
        self.assertNear(rot.pitch, 30, 0.1)
        self.assertNear(rot.yaw, 60, 0.1)
        self.cleanup_scene(scene)

    def test_set_get_fov(self):
        scene = self.make_scene()
        cam = scene.get_camera()
        cam.set_fov(60)
        self.assertNear(cam.get_fov(), 60, 0.1)
        cam.set_fov(90)
        self.assertNear(cam.get_fov(), 90, 0.1)
        self.cleanup_scene(scene)

    def test_look_at(self):
        scene = self.make_scene()
        cam = scene.get_camera()
        cam.set_position(0, -500, 0)
        cam.look_at(unreal.Vector(0, 0, 0))
        rot = cam.get_rotation()
        self.assertTrue(abs(rot.yaw) > 0 or abs(rot.pitch) > 0)
        self.cleanup_scene(scene)

    def test_orbit(self):
        scene = self.make_scene()
        cam = scene.get_camera()
        cam.orbit_around(unreal.Vector(0, 0, 0), 500, 90, 100)
        pos = cam.get_position()
        self.assertNear(pos.z, 100, 0.1)
        dist_xy = math.sqrt(pos.x ** 2 + pos.y ** 2)
        self.assertNear(dist_xy, 500, 1.0)
        self.cleanup_scene(scene)

    def test_default_fov(self):
        scene = self.make_scene()
        self.assertNear(scene.get_camera().get_fov(), 90, 0.1)
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
