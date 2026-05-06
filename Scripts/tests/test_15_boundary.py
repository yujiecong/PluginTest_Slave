import unittest
import time
import unreal
from test_framework import UEMotionTestCase


class TestBoundary(UEMotionTestCase):
    def test_negative_radius(self):
        scene = self.make_scene()
        obj = scene.create_sphere(-1)
        self.assertIsNotNone(obj, "CreateSphere(-1) should clamp and return non-None")
        self.cleanup_scene(scene)

    def test_negative_size(self):
        scene = self.make_scene()
        obj = scene.create_cube(-5)
        self.assertIsNotNone(obj)
        self.cleanup_scene(scene)

    def test_negative_cylinder(self):
        scene = self.make_scene()
        obj = scene.create_cylinder(-1, -1)
        self.assertIsNotNone(obj)
        self.cleanup_scene(scene)

    def test_uninitialized_scene(self):
        scene = unreal.UEMotionScene()
        obj = scene.create_sphere(50)
        self.assertIsNone(obj, "Uninitialized scene should return None")
        self.assertNoCrash(lambda: scene.render_frames("D:/nonexistent", 1.0, 30))
        self.assertNoCrash(lambda: scene.tick(0.033))

    def test_zero_duration_animation(self):
        scene = self.make_scene()
        obj = scene.create_sphere(50)
        anim = unreal.UEMotionMoveAnimation()
        anim.set_target_mobject(obj)
        anim.set_target(unreal.Vector(200, 0, 50))
        anim.set_duration(0.0)
        self.assertNoCrash(lambda: scene.play(anim))
        self.cleanup_scene(scene)

    def test_empty_path_render(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.render_frames("", 1.0, 30))
        self.assertNoCrash(lambda: scene.render_single_frame(""))
        self.cleanup_scene(scene)

    def test_many_mobjects_100(self):
        scene = self.make_scene()
        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        count = 0
        for i in range(100):
            x = (i % 10) * 80 - 360
            y = (i // 10) * 80 - 360
            obj = scene.create_sphere(15)
            if obj:
                obj.set_location(unreal.Vector(x, y, 30))
                count += 1
        self.assertEqual(count, 100)
        self.assertGreaterEqual(len(scene.get_all_mobjects()), 100)
        self.cleanup_scene(scene)

    def test_long_animation_60s(self):
        scene = self.make_scene()
        obj = scene.create_sphere(40)
        anim = unreal.UEMotionMoveAnimation()
        anim.set_target_mobject(obj)
        anim.set_target(unreal.Vector(200, 0, 50))
        anim.set_duration(60.0)
        anim.set_easing("linear")
        scene.play(anim)
        self.tick_animation(scene, 61.0)
        self.assertFalse(scene.has_active_animations())
        self.cleanup_scene(scene)

    def test_chinese_path(self):
        scene = self.make_scene()
        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        scene.create_sphere(50)
        self.assertNoCrash(lambda: scene.render_single_frame("D:/输出/测试帧.png"))
        self.cleanup_scene(scene)

    def test_destroy_twice(self):
        scene = self.make_scene()
        scene.destroy()
        self.assertNoCrash(lambda: scene.destroy())

    def test_tick_zero_delta(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.tick(0.0))
        self.cleanup_scene(scene)

    def test_tick_negative_delta(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.tick(-1.0))
        self.cleanup_scene(scene)

    def test_play_none(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.play(None))
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
