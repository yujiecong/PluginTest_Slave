import unittest
import unreal
from test_framework import UEMotionTestCase


class TestColorAnimation(UEMotionTestCase):
    def test_create(self):
        self.assertIsNotNone(unreal.UEMotionColorAnimation())

    def test_base_api(self):
        anim = unreal.UEMotionColorAnimation()
        anim.set_duration(1.5)
        self.assertNear(anim.get_duration(), 1.5, 0.01)
        anim.set_easing("ease_in_out_cubic")
        self.assertEqual(anim.get_easing(), "ease_in_out_cubic")

    def test_tick(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_color(unreal.LinearColor(1, 0, 0, 1))
        anim = unreal.UEMotionColorAnimation()
        anim.set_target_mobject(obj)
        anim.set_start_color(unreal.LinearColor(1, 0, 0, 1))
        anim.set_end_color(unreal.LinearColor(0, 0, 1, 1))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        scene.tick(1.0 / 30.0)
        c = obj.get_color()
        self.assertTrue(c.r < 1.0 or c.b > 0.0)
        self.cleanup_scene(scene)

    def test_finish(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_color(unreal.LinearColor(1, 0, 0, 1))
        anim = unreal.UEMotionColorAnimation()
        anim.set_target_mobject(obj)
        anim.set_start_color(unreal.LinearColor(1, 0, 0, 1))
        anim.set_end_color(unreal.LinearColor(0, 0, 1, 1))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        c = obj.get_color()
        self.assertNear(c.r, 0, 0.05)
        self.assertNear(c.b, 1, 0.05)
        self.cleanup_scene(scene)

    def test_auto_start(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_color(unreal.LinearColor(0, 1, 0, 1))
        anim = unreal.UEMotionColorAnimation()
        anim.set_target_mobject(obj)
        anim.set_end_color(unreal.LinearColor(1, 0, 0, 1))
        anim.set_duration(1.0)
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertNear(obj.get_color().r, 1, 0.05)
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
