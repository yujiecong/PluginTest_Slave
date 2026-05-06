import unittest
import unreal
from test_framework import UEMotionTestCase


class TestFadeAnimation(UEMotionTestCase):
    def test_create(self):
        self.assertIsNotNone(unreal.UEMotionFadeAnimation())

    def test_base_api(self):
        anim = unreal.UEMotionFadeAnimation()
        anim.set_duration(1.0)
        self.assertTrue(anim.get_duration() > 0)
        anim.set_easing("ease_in_out")
        self.assertEqual(anim.get_easing(), "ease_in_out")

    def test_fade_in(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_visibility(False)
        anim = unreal.UEMotionFadeAnimation()
        anim.set_target_mobject(obj)
        anim.set_fade_out(False)
        anim.set_duration(1.0)
        scene.play(anim)
        scene.tick(1.0 / 30.0)
        self.assertTrue(obj.get_visibility())
        self.cleanup_scene(scene)

    def test_fade_out(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_visibility(True)
        anim = unreal.UEMotionFadeAnimation()
        anim.set_target_mobject(obj)
        anim.set_fade_out(True)
        anim.set_duration(1.0)
        scene.play(anim)
        scene.tick(0.5 / 30.0)
        self.assertTrue(obj.get_visibility())
        self.tick_animation(scene, 1.5)
        self.assertFalse(obj.get_visibility())
        self.cleanup_scene(scene)

    def test_set_fade_out_flag(self):
        anim = unreal.UEMotionFadeAnimation()
        anim.set_fade_out(True)
        anim.set_fade_out(False)
        self.assertIsNotNone(anim)


if __name__ == "__main__":
    unittest.main()
