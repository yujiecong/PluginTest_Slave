import unittest
import sys
import os

try:
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
    from uemotion import Scene, vec, rot, resolve_color, COLOR_MAP, MVector, ORIGIN, RIGHT, UP
    from uemotion.constants import motion_to_ue
    HAS_UEMOTION = True
except ImportError:
    HAS_UEMOTION = False


@unittest.skipUnless(HAS_UEMOTION, "uemotion Python wrapper module not available (Phase 4)")
class TestPythonWrapper(unittest.TestCase):
    def test_scene_create(self):
        s = Scene("test_create", 1920, 1080, mode="2d")
        self.assertIsNotNone(s)
        self.assertEqual(s.mode, "2d")

    def test_sphere(self):
        s = Scene("test_sphere", 1920, 1080, mode="2d")
        s.directional_light()
        ball = s.sphere(1.2, color="red", location=ORIGIN)
        self.assertIsNotNone(ball)
        ball.color = "blue"
        s.destroy()

    def test_cube(self):
        s = Scene("test_cube", 1920, 1080, mode="2d")
        s.directional_light()
        box = s.cube(1.0, color="blue", location=RIGHT * 2)
        self.assertIsNotNone(box)
        s.destroy()

    def test_all_shapes(self):
        s = Scene("test_shapes", 1920, 1080, mode="2d")
        s.directional_light()
        self.assertIsNotNone(s.sphere(0.8, color="red"))
        self.assertIsNotNone(s.cube(0.8, color="blue"))
        self.assertIsNotNone(s.cylinder(0.6, 1.6, color="green"))
        self.assertIsNotNone(s.cone(0.6, 1.6, color="yellow"))
        self.assertIsNotNone(s.plane(4.0, 4.0, color="gray"))
        self.assertIsNotNone(s.torus(0.8, 0.24, color="purple"))
        s.destroy()

    def test_move_to(self):
        s = Scene("test_move", 1920, 1080, mode="2d")
        s.directional_light()
        ball = s.sphere(0.8, color="red", location=ORIGIN)
        ball.move_to(RIGHT * 4, duration=1.0, easing="ease_in_out")
        s.play()
        s.destroy()

    def test_rotate(self):
        s = Scene("test_rotate", 1920, 1080, mode="2d")
        s.directional_light()
        box = s.cube(1.0, color="blue", location=ORIGIN)
        box.rotate(360, axis=(0, 0, 1), duration=1.0)
        s.play()
        s.destroy()

    def test_scale_to(self):
        s = Scene("test_scale", 1920, 1080, mode="2d")
        s.directional_light()
        ball = s.sphere(0.8, color="red", location=ORIGIN)
        ball.scale_to(2.0, duration=1.0)
        s.play()
        s.destroy()

    def test_fade_in(self):
        s = Scene("test_fade_in", 1920, 1080, mode="2d")
        s.directional_light()
        ball = s.sphere(0.8, color="red", location=ORIGIN)
        ball.fade_in(duration=1.0)
        s.play()
        s.destroy()

    def test_fade_out(self):
        s = Scene("test_fade_out", 1920, 1080, mode="2d")
        s.directional_light()
        ball = s.sphere(0.8, color="red", location=ORIGIN)
        ball.fade_out(duration=1.0)
        s.play()
        s.destroy()

    def test_change_color(self):
        s = Scene("test_color", 1920, 1080, mode="2d")
        s.directional_light()
        ball = s.sphere(0.8, color="red", location=ORIGIN)
        ball.change_color("blue", duration=1.0)
        s.play()
        s.destroy()

    def test_camera(self):
        s = Scene("test_camera", 1920, 1080, mode="2d")
        s.directional_light()
        from uemotion.constants import PROJECTION_ORTHOGRAPHIC
        self.assertEqual(s.camera.projection_mode, PROJECTION_ORTHOGRAPHIC)
        s.camera.fov = 60
        self.assertLess(abs(s.camera.fov - 60), 1.0)
        s.destroy()

    def test_wait(self):
        s = Scene("test_wait", 1920, 1080, mode="2d")
        s.directional_light()
        s.wait(0.5)
        s.destroy()

    def test_color_resolution(self):
        c1 = resolve_color("red")
        self.assertIsNotNone(c1)
        c2 = resolve_color("#FF8800")
        self.assertIsNotNone(c2)
        c3 = resolve_color([0.5, 0.3, 0.8])
        self.assertIsNotNone(c3)
        c4 = resolve_color("nonexistent")
        self.assertIsNotNone(c4)

    def test_vec(self):
        v = vec(1, 2, 3)
        self.assertIsNotNone(v)
        v2 = vec([1, 2, 3])
        self.assertIsNotNone(v2)

    def test_mvector(self):
        v = MVector(1, 2, 3)
        self.assertEqual(v.x, 1.0)
        self.assertEqual(v[1], 2.0)
        v2 = v * 2
        self.assertEqual(v2.x, 2.0)
        v3 = UP * 3 + RIGHT * 2
        self.assertAlmostEqual(v3.x, 2.0)
        self.assertAlmostEqual(v3.y, 3.0)

    def test_coordinate_conversion(self):
        loc = MVector(4, 4, 0)
        ue_loc = motion_to_ue(loc.x)
        self.assertAlmostEqual(ue_loc, 200.0)

    def test_chain(self):
        s = Scene("test_chain", 1920, 1080, mode="2d")
        s.directional_light()
        ball = s.sphere(0.8, color="red", location=ORIGIN)
        ball.move_to(RIGHT * 4, duration=1.0).rotate(360, duration=1.0)
        s.play()
        s.destroy()


if __name__ == "__main__":
    unittest.main()
