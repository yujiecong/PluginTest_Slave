import unittest
import unreal
from test_framework import PyManimTestCase


class TestScene(PyManimTestCase):
    def test_scene_create(self):
        scene = unreal.PyManimScene()
        self.assertIsNotNone(scene)

    def test_scene_initialize(self):
        scene = self.make_scene()
        self.assertTrue(scene.is_initialized())
        self.cleanup_scene(scene)

    def test_scene_initialize_custom_res(self):
        scene = unreal.PyManimScene()
        scene.initialize(3840, 2160)
        self.assertTrue(scene.is_initialized())
        self.cleanup_scene(scene)

    def test_scene_double_initialize(self):
        scene = unreal.PyManimScene()
        scene.initialize(1920, 1080)
        self.assertNoCrash(lambda: scene.initialize(1920, 1080))
        self.assertTrue(scene.is_initialized())
        self.cleanup_scene(scene)

    def test_scene_get_camera(self):
        scene = self.make_scene()
        cam = scene.get_camera()
        self.assertIsNotNone(cam)
        self.cleanup_scene(scene)

    def test_scene_create_all_shapes(self):
        scene = self.make_scene()
        s = scene.create_sphere(50)
        c = scene.create_cube(50)
        cy = scene.create_cylinder(50, 100)
        co = scene.create_cone(50, 100)
        p = scene.create_plane(500, 500)
        t = scene.create_torus(80, 25)
        self.assertIsNotNone(s)
        self.assertIsNotNone(c)
        self.assertIsNotNone(cy)
        self.assertIsNotNone(co)
        self.assertIsNotNone(p)
        self.assertIsNotNone(t)
        self.assertEqual(len(scene.get_all_mobjects()), 6)
        self.cleanup_scene(scene)

    def test_scene_play_tick(self):
        scene = self.make_scene()
        sphere = scene.create_sphere(50)
        anim = unreal.PyManimMoveAnimation()
        anim.set_target_mobject(sphere)
        anim.set_target(unreal.Vector(200, 0, 0))
        anim.set_duration(1.0)
        scene.play(anim)
        self.assertTrue(scene.has_active_animations())
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        self.cleanup_scene(scene)

    def test_scene_stop_all(self):
        scene = self.make_scene()
        sphere = scene.create_sphere(50)
        anim = unreal.PyManimMoveAnimation()
        anim.set_target_mobject(sphere)
        anim.set_target(unreal.Vector(200, 0, 0))
        anim.set_duration(5.0)
        scene.play(anim)
        self.assertTrue(scene.has_active_animations())
        scene.stop_all()
        self.assertFalse(scene.has_active_animations())
        self.cleanup_scene(scene)

    def test_scene_clear(self):
        scene = self.make_scene()
        scene.create_sphere(50)
        scene.create_cube(50)
        self.assertEqual(len(scene.get_all_mobjects()), 2)
        scene.clear_scene()
        self.assertEqual(len(scene.get_all_mobjects()), 0)
        self.cleanup_scene(scene)

    def test_scene_destroy(self):
        scene = self.make_scene()
        scene.destroy()
        self.assertFalse(scene.is_initialized())

    def test_scene_add_directional_light(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.add_directional_light(
            unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10))
        self.cleanup_scene(scene)

    def test_scene_add_point_light(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.add_point_light(
            unreal.Vector(0, 0, 200), unreal.LinearColor(1, 1, 1, 1), 5000))
        self.cleanup_scene(scene)

    def test_scene_render_frames(self):
        scene = self.make_scene()
        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        scene.create_sphere(50)
        self.assertNoCrash(lambda: scene.render_frames(
            "D:/PyManimTest/test_scene_frames", 0.5, 30))
        self.cleanup_scene(scene)

    def test_scene_render_single_frame(self):
        scene = self.make_scene()
        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        scene.create_sphere(50)
        self.assertNoCrash(lambda: scene.render_single_frame(
            "D:/PyManimTest/test_single_frame.png"))
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
