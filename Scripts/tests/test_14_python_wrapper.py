import unittest
import sys
import os

try:
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
    from uemotion import Scene, vec, rot, resolve_color, COLOR_MAP
    HAS_UEMOTION = True
except ImportError:
    HAS_UEMOTION = False


@unittest.skipUnless(HAS_UEMOTION, "uemotion Python wrapper module not available (Phase 4)")
class TestPythonWrapper(unittest.TestCase):
    def test_scene_create(self):
        s = Scene(1920, 1080)
        self.assertIsNotNone(s)

    def test_sphere(self):
        s = Scene(1920, 1080)
        s.directional_light()
        ball = s.sphere(60, color="red", location=(0, 0, 50))
        self.assertIsNotNone(ball)
        ball.color = "blue"
        s.destroy()

    def test_cube(self):
        s = Scene(1920, 1080)
        s.directional_light()
        box = s.cube(50, color="blue", location=(100, 0, 50))
        self.assertIsNotNone(box)
        s.destroy()

    def test_all_shapes(self):
        s = Scene(1920, 1080)
        s.directional_light()
        self.assertIsNotNone(s.sphere(40, color="red"))
        self.assertIsNotNone(s.cube(40, color="blue"))
        self.assertIsNotNone(s.cylinder(30, 80, color="green"))
        self.assertIsNotNone(s.cone(30, 80, color="yellow"))
        self.assertIsNotNone(s.plane(200, 200, color="gray"))
        self.assertIsNotNone(s.torus(40, 12, color="purple"))
        s.destroy()

    def test_move_to(self):
        s = Scene(1920, 1080)
        s.directional_light()
        ball = s.sphere(40, color="red", location=(0, 0, 50))
        ball.move_to((200, 0, 50), duration=1.0, easing="ease_in_out")
        s.play()
        s.destroy()

    def test_rotate(self):
        s = Scene(1920, 1080)
        s.directional_light()
        box = s.cube(50, color="blue", location=(0, 0, 50))
        box.rotate(360, axis=(0, 0, 1), duration=1.0)
        s.play()
        s.destroy()

    def test_scale_to(self):
        s = Scene(1920, 1080)
        s.directional_light()
        ball = s.sphere(40, color="red", location=(0, 0, 50))
        ball.scale_to(2.0, duration=1.0)
        s.play()
        s.destroy()

    def test_fade_in(self):
        s = Scene(1920, 1080)
        s.directional_light()
        ball = s.sphere(40, color="red", location=(0, 0, 50))
        ball.fade_in(duration=1.0)
        s.play()
        s.destroy()

    def test_fade_out(self):
        s = Scene(1920, 1080)
        s.directional_light()
        ball = s.sphere(40, color="red", location=(0, 0, 50))
        ball.fade_out(duration=1.0)
        s.play()
        s.destroy()

    def test_change_color(self):
        s = Scene(1920, 1080)
        s.directional_light()
        ball = s.sphere(40, color="red", location=(0, 0, 50))
        ball.change_color("blue", duration=1.0)
        s.play()
        s.destroy()

    def test_camera(self):
        s = Scene(1920, 1080)
        s.directional_light()
        s.camera.position = (-350, -450, 250)
        s.camera.look_at((0, 0, 0))
        s.camera.fov = 60
        self.assertLess(abs(s.camera.fov - 60), 1.0)
        s.destroy()

    def test_wait(self):
        s = Scene(1920, 1080)
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

    def test_chain(self):
        s = Scene(1920, 1080)
        s.directional_light()
        ball = s.sphere(40, color="red", location=(0, 0, 50))
        ball.move_to((200, 0, 50), duration=1.0).rotate(360, duration=1.0)
        s.play()
        s.destroy()


if __name__ == "__main__":
    unittest.main()
