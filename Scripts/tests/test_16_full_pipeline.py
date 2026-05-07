import unittest
import unreal
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from test_framework import UEMotionTestCase

OUTPUT_BASE = "D:/UEMotionTest/full_pipeline"


class TestFullPipeline(UEMotionTestCase):

    def test_cube_move_y_equals_x(self):
        num_steps = 5
        step_size = 10
        initial_z = 50.0

        scene = self.make_scene()
        self.assertTrue(scene.is_initialized())

        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        scene.add_point_light(unreal.Vector(0, 0, 400), unreal.LinearColor(1, 1, 1, 1), 3000)

        cube = scene.create_cube(30)
        self.assertIsNotNone(cube)
        cube.set_location(unreal.Vector(0, 0, initial_z))

        camera = scene.get_camera()
        camera.set_position(-300, -500, 400)
        camera.look_at(unreal.Vector(num_steps * step_size / 2.0, num_steps * step_size / 2.0, initial_z))

        for i in range(1, num_steps + 1):
            target_x = float(i * step_size)
            target_y = float(i * step_size)

            anim = unreal.UEMotionMoveAnimation()
            anim.set_target_mobject(cube)
            anim.set_target(unreal.Vector(target_x, target_y, initial_z))
            anim.set_duration(1.0)
            anim.set_easing("linear")
            scene.play(anim)
            self.tick_animation(scene, 1.5)

            loc = cube.get_location()
            self.assertNear(loc.x, target_x, 2.0, f"Step {i} X position")
            self.assertNear(loc.y, target_y, 2.0, f"Step {i} Y position")
            self.assertNear(loc.z, initial_z, 2.0, f"Step {i} Z position")

        self.assertFalse(scene.has_active_animations())
        self.cleanup_scene(scene)

    def test_cube_y_equals_x_render_steps(self):
        num_steps = 5
        step_size = 10
        initial_z = 50.0

        scene = self.make_scene()
        self.assertTrue(scene.is_initialized())

        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        scene.add_point_light(unreal.Vector(0, 0, 400), unreal.LinearColor(1, 1, 1, 1), 3000)

        cube = scene.create_cube(30)
        self.assertIsNotNone(cube)

        camera = scene.get_camera()
        camera.set_position(-300, -500, 400)
        camera.look_at(unreal.Vector(num_steps * step_size / 2.0, num_steps * step_size / 2.0, initial_z))

        frames_dir = os.path.join(OUTPUT_BASE, "y_equals_x_steps")
        if not os.path.exists(frames_dir):
            os.makedirs(frames_dir, exist_ok=True)

        for i in range(num_steps + 1):
            pos_x = float(i * step_size)
            pos_y = float(i * step_size)
            cube.set_location(unreal.Vector(pos_x, pos_y, initial_z))

            step_dir = os.path.join(frames_dir, f"step_{i:04d}")
            if not os.path.exists(step_dir):
                os.makedirs(step_dir, exist_ok=True)

            self.assertNoCrash(lambda d=step_dir: scene.render_single_frame(d + "/frame.png"))

        self.cleanup_scene(scene)

    def test_cube_y_equals_x_render_sequence(self):
        num_steps = 5
        step_size = 10
        initial_z = 50.0

        scene = self.make_scene()
        self.assertTrue(scene.is_initialized())

        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        scene.add_point_light(unreal.Vector(0, 0, 400), unreal.LinearColor(1, 1, 1, 1), 3000)

        cube = scene.create_cube(30)
        self.assertIsNotNone(cube)
        cube.set_location(unreal.Vector(0, 0, initial_z))

        camera = scene.get_camera()
        camera.set_position(-300, -500, 400)
        camera.look_at(unreal.Vector(num_steps * step_size / 2.0, num_steps * step_size / 2.0, initial_z))

        for i in range(1, num_steps + 1):
            target_x = float(i * step_size)
            target_y = float(i * step_size)
            anim = unreal.UEMotionMoveAnimation()
            anim.set_target_mobject(cube)
            anim.set_target(unreal.Vector(target_x, target_y, initial_z))
            anim.set_duration(1.0)
            anim.set_easing("linear")
            scene.play(anim)
            self.tick_animation(scene, 1.5)

        seq_dir = os.path.join(OUTPUT_BASE, "y_equals_x_sequence")
        self.assertNoCrash(lambda: scene.render_frames(seq_dir, 0.5, 30))

        self.cleanup_scene(scene)

    def test_cube_y_equals_x_pymanim(self):
        try:
            sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
            from pymanim import Scene, vec
        except ImportError:
            self.skipTest("pymanim module not available")

        num_steps = 5
        step_size = 10
        initial_z = 50.0

        s = Scene(1920, 1080)
        s.directional_light(direction=(0, -1, -1), color="white", intensity=10)
        s.point_light(location=(0, 0, 400), color="white", intensity=3000)
        s.camera.position = (-300, -500, 400)
        s.camera.look_at((num_steps * step_size / 2.0, num_steps * step_size / 2.0, initial_z))

        box = s.cube(30, color="cyan", location=(0, 0, initial_z))
        self.assertIsNotNone(box)

        for i in range(1, num_steps + 1):
            target_x = i * step_size
            target_y = i * step_size
            box.move_to((target_x, target_y, initial_z), duration=1.0, easing="linear")
            s.play()

        loc = box.location
        self.assertTrue(abs(loc.x - num_steps * step_size) < 3.0,
                        f"Final X: expected {num_steps * step_size}, got {loc.x}")
        self.assertTrue(abs(loc.y - num_steps * step_size) < 3.0,
                        f"Final Y: expected {num_steps * step_size}, got {loc.y}")

        frames_dir = os.path.join(OUTPUT_BASE, "pymanim_y_equals_x")
        self.assertNoCrash(lambda: s._ue.render_frames(frames_dir, 0.5, 30))

        s.destroy()

    def test_cube_y_equals_x_position_trace(self):
        num_steps = 5
        step_size = 10
        initial_z = 50.0

        scene = self.make_scene()
        self.assertTrue(scene.is_initialized())

        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)

        cube = scene.create_cube(30)
        self.assertIsNotNone(cube)
        cube.set_location(unreal.Vector(0, 0, initial_z))

        camera = scene.get_camera()
        camera.set_position(-300, -500, 400)
        camera.look_at(unreal.Vector(num_steps * step_size / 2.0, num_steps * step_size / 2.0, initial_z))

        positions = []
        positions.append((0.0, 0.0, initial_z))

        for i in range(1, num_steps + 1):
            target_x = float(i * step_size)
            target_y = float(i * step_size)

            anim = unreal.UEMotionMoveAnimation()
            anim.set_target_mobject(cube)
            anim.set_target(unreal.Vector(target_x, target_y, initial_z))
            anim.set_duration(1.0)
            anim.set_easing("linear")
            scene.play(anim)
            self.tick_animation(scene, 1.5)

            loc = cube.get_location()
            positions.append((loc.x, loc.y, loc.z))

        for i, (px, py, pz) in enumerate(positions):
            expected_x = float(i * step_size)
            expected_y = float(i * step_size)
            self.assertNear(px, expected_x, 2.0, f"Position trace step {i} X")
            self.assertNear(py, expected_y, 2.0, f"Position trace step {i} Y")
            self.assertNear(pz, initial_z, 2.0, f"Position trace step {i} Z")

        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
