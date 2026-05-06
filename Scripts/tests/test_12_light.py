import unittest
import unreal
from test_framework import UEMotionTestCase


class TestLight(UEMotionTestCase):
    def test_directional_light_create(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.add_directional_light(
            unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 10))
        self.cleanup_scene(scene)

    def test_point_light_create(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.add_point_light(
            unreal.Vector(0, 0, 200), unreal.LinearColor(1, 1, 1, 1), 5000))
        self.cleanup_scene(scene)

    def test_multiple_lights(self):
        scene = self.make_scene()
        for i in range(5):
            scene.add_directional_light(
                unreal.Vector(i * 10, -1, -1), unreal.LinearColor(1, 1, 1, 1), 5)
            scene.add_point_light(
                unreal.Vector(i * 100, 0, 200), unreal.LinearColor(1, 1, 1, 1), 3000)
        self.cleanup_scene(scene)

    def test_zero_intensity(self):
        scene = self.make_scene()
        self.assertNoCrash(lambda: scene.add_directional_light(
            unreal.Vector(0, -1, -1), unreal.LinearColor(1, 1, 1, 1), 0))
        self.assertNoCrash(lambda: scene.add_point_light(
            unreal.Vector(0, 0, 200), unreal.LinearColor(1, 1, 1, 1), 0))
        self.cleanup_scene(scene)


if __name__ == "__main__":
    unittest.main()
