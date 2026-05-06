import unittest
import unreal
from test_framework import PyManimTestCase


class TestGroupAnimation(PyManimTestCase):
    def test_create(self):
        self.assertIsNotNone(unreal.PyManimGroupAnimation())

    def test_add_animation(self):
        group = unreal.PyManimGroupAnimation()
        move = unreal.PyManimMoveAnimation()
        move.set_duration(1.0)
        group.add_animation(move)
        self.assertEqual(group.get_animation_count(), 1)
        rotate = unreal.PyManimRotateAnimation()
        rotate.set_duration(1.0)
        group.add_animation(rotate)
        self.assertEqual(group.get_animation_count(), 2)

    def test_set_play_mode(self):
        group = unreal.PyManimGroupAnimation()
        group.set_play_mode(False)
        group.set_play_mode(True)

    def test_parallel(self):
        scene = self.make_scene()
        sphere = scene.create_sphere(50)
        sphere.set_location(unreal.Vector(0, 0, 0))
        cube = scene.create_cube(50)
        cube.set_location(unreal.Vector(200, 0, 0))

        move = unreal.PyManimMoveAnimation()
        move.set_target_mobject(sphere)
        move.set_target(unreal.Vector(200, 0, 0))
        move.set_duration(1.0)
        move.set_easing("linear")

        rotate = unreal.PyManimRotateAnimation()
        rotate.set_target_mobject(cube)
        rotate.set_rotation_angle(90)
        rotate.set_axis(unreal.Vector(0, 0, 1))
        rotate.set_duration(1.0)

        group = unreal.PyManimGroupAnimation()
        group.add_animation(move)
        group.add_animation(rotate)
        group.set_play_mode(False)

        scene.play(group)
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        self.assertNear(sphere.get_location().x, 200, 2.0)
        self.cleanup_scene(scene)

    def test_sequential(self):
        scene = self.make_scene()
        sphere = scene.create_sphere(50)
        sphere.set_location(unreal.Vector(0, 0, 0))

        move1 = unreal.PyManimMoveAnimation()
        move1.set_target_mobject(sphere)
        move1.set_target(unreal.Vector(100, 0, 0))
        move1.set_duration(1.0)
        move1.set_easing("linear")

        move2 = unreal.PyManimMoveAnimation()
        move2.set_target_mobject(sphere)
        move2.set_start(unreal.Vector(100, 0, 0))
        move2.set_target(unreal.Vector(200, 0, 0))
        move2.set_duration(1.0)
        move2.set_easing("linear")

        group = unreal.PyManimGroupAnimation()
        group.add_animation(move1)
        group.add_animation(move2)
        group.set_play_mode(True)

        scene.play(group)
        self.assertTrue(scene.has_active_animations())
        self.tick_animation(scene, 1.0)
        self.assertTrue(scene.has_active_animations())
        self.tick_animation(scene, 1.5)
        self.assertFalse(scene.has_active_animations())
        self.assertNear(sphere.get_location().x, 200, 2.0)
        self.cleanup_scene(scene)

    def test_base_api(self):
        group = unreal.PyManimGroupAnimation()
        group.set_duration(2.0)
        self.assertNear(group.get_duration(), 2.0, 0.01)
        group.set_easing("linear")
        self.assertEqual(group.get_easing(), "linear")
        group.reset()
        self.assertNear(group.get_progress(), 0.0, 0.01)

    def test_empty_group(self):
        scene = self.make_scene()
        group = unreal.PyManimGroupAnimation()
        scene.play(group)
        self.assertFalse(scene.has_active_animations())
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
