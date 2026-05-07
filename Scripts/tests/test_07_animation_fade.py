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
        obj.set_opacity(0.0)
        anim = unreal.UEMotionFadeAnimation()
        anim.set_target_mobject(obj)
        anim.set_fade_in(True)
        anim.set_duration(1.0)
        scene.play(anim)
        scene.tick(1.0 / 30.0)
        self.assertGreater(obj.get_opacity(), 0.0)
        self.cleanup_scene(scene)

    def test_fade_out(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_opacity(1.0)
        anim = unreal.UEMotionFadeAnimation()
        anim.set_target_mobject(obj)
        anim.set_fade_out(True)
        anim.set_duration(1.0)
        scene.play(anim)
        scene.tick(0.5 / 30.0)
        self.assertLess(obj.get_opacity(), 1.0)
        self.tick_animation(scene, 1.5)
        self.assertNear(obj.get_opacity(), 0.0, 0.05)
        self.assertFalse(obj.get_visibility())
        self.cleanup_scene(scene)

    def test_fade_in_completes(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_opacity(0.0)
        anim = unreal.UEMotionFadeAnimation()
        anim.set_target_mobject(obj)
        anim.set_fade_in(True)
        anim.set_duration(1.0)
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertNear(obj.get_opacity(), 1.0, 0.05)
        self.assertTrue(obj.get_visibility())
        self.cleanup_scene(scene)

    def test_set_fade_out_flag(self):
        anim = unreal.UEMotionFadeAnimation()
        anim.set_fade_out(True)
        anim.set_fade_out(False)
        self.assertIsNotNone(anim)

    def test_fade_midpoint(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        obj.set_opacity(1.0)
        anim = unreal.UEMotionFadeAnimation()
        anim.set_target_mobject(obj)
        anim.set_fade_out(True)
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        self.tick_animation(scene, 0.5)
        self.assertNear(obj.get_opacity(), 0.5, 0.1)
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
