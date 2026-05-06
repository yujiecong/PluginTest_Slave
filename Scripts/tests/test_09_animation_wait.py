import unittest
import unreal
from test_framework import PyManimTestCase


class TestWaitAnimation(PyManimTestCase):
    def test_create(self):
        self.assertIsNotNone(unreal.PyManimWaitAnimation())

    def test_duration_no_move(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        start_loc = obj.get_location()
        anim = unreal.PyManimWaitAnimation()
        anim.set_duration(2.0)
        scene.play(anim)
        self.tick_animation(scene, 2.5)
        self.assertFalse(scene.has_active_animations())
        end_loc = obj.get_location()
        self.assertNear(end_loc.x, start_loc.x, 0.1)
        self.assertNear(end_loc.y, start_loc.y, 0.1)
        self.assertNear(end_loc.z, start_loc.z, 0.1)
        self.cleanup_scene(scene)

    def test_base_api(self):
        anim = unreal.PyManimWaitAnimation()
        anim.set_duration(3.0)
        self.assertNear(anim.get_duration(), 3.0, 0.01)
        anim.set_easing("linear")
        self.assertEqual(anim.get_easing(), "linear")
        self.assertFalse(anim.is_finished())
        anim.reset()
        self.assertNear(anim.get_progress(), 0.0, 0.01)


if __name__ == "__main__":
    unittest.main()
