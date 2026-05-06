import unittest
import unreal
from test_framework import PyManimTestCase


class TestRotateAnimation(PyManimTestCase):
    def test_create(self):
        self.assertIsNotNone(unreal.PyManimRotateAnimation())

    def test_base_api(self):
        anim = unreal.PyManimRotateAnimation()
        anim.set_duration(2.0)
        self.assertNear(anim.get_duration(), 2.0, 0.01)
        anim.set_easing("ease_out_cubic")
        self.assertEqual(anim.get_easing(), "ease_out_cubic")

    def test_tick(self):
        scene = self.make_scene()
        obj = scene.create_cube(50)
        anim = unreal.PyManimRotateAnimation()
        anim.set_target_mobject(obj)
        anim.set_rotation_angle(90)
        anim.set_axis(unreal.Vector(0, 0, 1))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        scene.tick(1.0 / 30.0)
        self.assertTrue(abs(obj.get_rotation().yaw) > 0)
        self.cleanup_scene(scene)

    def test_finish(self):
        scene = self.make_scene()
        obj = scene.create_cube(50)
        anim = unreal.PyManimRotateAnimation()
        anim.set_target_mobject(obj)
        anim.set_rotation_angle(360)
        anim.set_axis(unreal.Vector(0, 0, 1))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        self.assertNear(abs(obj.get_rotation().yaw), 360, 5.0)
        self.cleanup_scene(scene)

    def test_different_axis(self):
        scene = self.make_scene()
        obj = scene.create_cube(50)
        anim = unreal.PyManimRotateAnimation()
        anim.set_target_mobject(obj)
        anim.set_rotation_angle(90)
        anim.set_axis(unreal.Vector(1, 0, 0))
        anim.set_duration(1.0)
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
