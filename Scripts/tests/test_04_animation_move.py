import unittest
import unreal
from test_framework import PyManimTestCase


class TestMoveAnimation(PyManimTestCase):
    def test_create(self):
        anim = unreal.PyManimMoveAnimation()
        self.assertIsNotNone(anim)

    def test_base_api(self):
        anim = unreal.PyManimMoveAnimation()
        anim.set_duration(2.0)
        self.assertNear(anim.get_duration(), 2.0, 0.01)
        anim.set_easing("ease_in_out")
        self.assertEqual(anim.get_easing(), "ease_in_out")
        self.assertFalse(anim.is_finished())
        self.assertNear(anim.get_progress(), 0.0, 0.01)
        anim.reset()
        self.assertNear(anim.get_progress(), 0.0, 0.01)

    def test_tick_progress(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_location(unreal.Vector(0, 0, 0))
        anim = unreal.PyManimMoveAnimation()
        anim.set_target_mobject(obj)
        anim.set_target(unreal.Vector(200, 0, 0))
        anim.set_duration(2.0)
        anim.set_easing("linear")
        scene.play(anim)
        scene.tick(1.0 / 30.0)
        self.assertTrue(scene.has_active_animations())
        self.assertTrue(obj.get_location().x > 0)
        self.cleanup_scene(scene)

    def test_finish(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_location(unreal.Vector(0, 0, 0))
        anim = unreal.PyManimMoveAnimation()
        anim.set_target_mobject(obj)
        anim.set_target(unreal.Vector(200, 0, 0))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        self.assertNear(obj.get_location().x, 200, 1.0)
        self.cleanup_scene(scene)

    def test_with_start(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        anim = unreal.PyManimMoveAnimation()
        anim.set_target_mobject(obj)
        anim.set_start(unreal.Vector(-100, 0, 0))
        anim.set_target(unreal.Vector(100, 0, 0))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertNear(obj.get_location().x, 100, 1.0)
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
