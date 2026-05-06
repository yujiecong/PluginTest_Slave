import unittest
import unreal
from test_framework import PyManimTestCase


class TestScaleAnimation(PyManimTestCase):
    def test_create(self):
        self.assertIsNotNone(unreal.PyManimScaleAnimation())

    def test_base_api(self):
        anim = unreal.PyManimScaleAnimation()
        anim.set_duration(1.5)
        self.assertNear(anim.get_duration(), 1.5, 0.01)
        anim.set_easing("ease_in_out")
        self.assertEqual(anim.get_easing(), "ease_in_out")

    def test_tick(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        anim = unreal.PyManimScaleAnimation()
        anim.set_target_mobject(obj)
        anim.set_start_scale(unreal.Vector(1, 1, 1))
        anim.set_end_scale(unreal.Vector(2, 2, 2))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        scene.tick(1.0 / 30.0)
        self.assertTrue(obj.get_scale().x > 1.0)
        self.cleanup_scene(scene)

    def test_finish(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        anim = unreal.PyManimScaleAnimation()
        anim.set_target_mobject(obj)
        anim.set_start_scale(unreal.Vector(1, 1, 1))
        anim.set_end_scale(unreal.Vector(3, 3, 3))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        s = obj.get_scale()
        self.assertNear(s.x, 3, 0.1)
        self.assertNear(s.y, 3, 0.1)
        self.assertNear(s.z, 3, 0.1)
        self.cleanup_scene(scene)

    def test_scale_down(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        anim = unreal.PyManimScaleAnimation()
        anim.set_target_mobject(obj)
        anim.set_start_scale(unreal.Vector(1, 1, 1))
        anim.set_end_scale(unreal.Vector(0.1, 0.1, 0.1))
        anim.set_duration(1.0)
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertNear(obj.get_scale().x, 0.1, 0.05)
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
