import unittest
import unreal
from test_framework import UEMotionTestCase


class TestRotateAnimation(UEMotionTestCase):
    def test_create(self):
        self.assertIsNotNone(unreal.UEMotionRotateAnimation())

    def test_base_api(self):
        anim = unreal.UEMotionRotateAnimation()
        anim.set_duration(2.0)
        self.assertNear(anim.get_duration(), 2.0, 0.01)
        anim.set_easing("ease_out_cubic")
        self.assertEqual(anim.get_easing(), "ease_out_cubic")

    def test_tick(self):
        scene = self.make_scene()
        obj = scene.create_cube(50)
        anim = unreal.UEMotionRotateAnimation()
        anim.set_target_mobject(obj)
        anim.set_rotation_angle(90)
        anim.set_axis(unreal.Vector(0, 0, 1))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        scene.tick(1.0 / 30.0)
        rotation = obj.get_rotation()
        has_rotation = (abs(rotation.pitch) > 0.1 or
                       abs(rotation.yaw) > 0.1 or
                       abs(rotation.roll) > 0.1)
        self.assertTrue(has_rotation, "Object should have some rotation after tick")
        self.cleanup_scene(scene)

    def test_finish(self):
        scene = self.make_scene()
        obj = scene.create_cube(50)
        anim = unreal.UEMotionRotateAnimation()
        anim.set_target_mobject(obj)
        anim.set_rotation_angle(360)
        anim.set_axis(unreal.Vector(0, 0, 1))
        anim.set_duration(1.0)
        anim.set_easing("linear")
        scene.play(anim)
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        final_yaw = abs(obj.get_rotation().yaw)
        is_360_or_0 = (final_yaw < 5.0 or abs(final_yaw - 360) < 5.0)
        self.assertTrue(is_360_or_0, f"Expected ~360 or ~0 (normalized), got {final_yaw}")
        self.cleanup_scene(scene)

    def test_different_axis(self):
        scene = self.make_scene()
        obj = scene.create_cube(50)
        anim = unreal.UEMotionRotateAnimation()
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
