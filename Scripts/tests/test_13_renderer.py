import unittest
import unreal
from test_framework import PyManimTestCase


class TestRenderer(PyManimTestCase):
    def _make_renderer(self):
        import unreal as ue
        scene = self.make_scene()
        scene.add_directional_light(ue.Vector(0, -1, -1), ue.LinearColor(1, 1, 1, 1), 10)
        renderer = ue.PyManimRenderer()
        world = ue.EditorLevelLibrary.get_editor_world()
        if not world:
            world = ue.UnrealEditor.get_editor_world()
        renderer.initialize(world, 1920, 1080)
        return scene, renderer

    def test_initialize(self):
        scene = self.make_scene()
        renderer = unreal.PyManimRenderer()
        self.assertNoCrash(lambda: renderer.initialize(
            unreal.EditorLevelLibrary.get_editor_world(), 1920, 1080))
        self.cleanup_scene(scene)

    def test_bind_camera(self):
        scene = self.make_scene()
        renderer = unreal.PyManimRenderer()
        renderer.initialize(unreal.EditorLevelLibrary.get_editor_world(), 1920, 1080)
        self.assertNoCrash(lambda: renderer.bind_camera(None))
        self.cleanup_scene(scene)

    def test_set_aa(self):
        renderer = unreal.PyManimRenderer()
        self.assertNoCrash(lambda: renderer.set_anti_aliasing(0))
        self.assertNoCrash(lambda: renderer.set_anti_aliasing(1))

    def test_set_format(self):
        renderer = unreal.PyManimRenderer()
        self.assertNoCrash(lambda: renderer.set_output_format("png"))
        self.assertNoCrash(lambda: renderer.set_output_format("jpg"))

    def test_is_rendering(self):
        renderer = unreal.PyManimRenderer()
        self.assertFalse(renderer.is_rendering())

    def test_get_progress(self):
        renderer = unreal.PyManimRenderer()
        progress = renderer.get_progress()
        self.assertIsInstance(progress, float)

    def test_render_sequence(self):
        scene = self.make_scene()
        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        scene.create_sphere(50)
        self.assertNoCrash(lambda: scene.render_frames(
            "D:/PyManimTest/test_renderer_seq", 0.5, 30))
        self.cleanup_scene(scene)

    def test_render_single_frame(self):
        scene = self.make_scene()
        scene.add_directional_light(unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10)
        scene.create_sphere(50)
        self.assertNoCrash(lambda: scene.render_single_frame(
            "D:/PyManimTest/test_renderer_frame.png"))
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
