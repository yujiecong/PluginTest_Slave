import unittest
import unreal
from test_framework import PyManimTestCase

EASING_NAMES = [
    "linear", "ease_in_quad", "ease_out_quad", "ease_in_out_quad",
    "ease_in_cubic", "ease_out_cubic", "ease_in_out_cubic",
    "ease_in_quart", "ease_out_quart", "ease_in_out_quart",
    "ease_in_quint", "ease_out_quint", "ease_in_out_quint",
    "ease_in_expo", "ease_out_expo", "ease_in_out_expo",
    "ease_in_circ", "ease_out_circ", "ease_in_out_circ",
    "ease_in_back", "ease_out_back", "ease_in_out_back",
    "ease_in_bounce", "ease_out_bounce", "ease_in_out_bounce",
    "ease_in_elastic", "ease_out_elastic", "ease_in_out_elastic",
]

MONOTONIC_EASINGS = [
    "linear", "ease_in_quad", "ease_out_quad", "ease_in_cubic",
    "ease_out_cubic", "ease_in_quart", "ease_out_quart",
    "ease_in_quint", "ease_out_quint", "ease_in_expo", "ease_out_expo",
    "ease_in_circ", "ease_out_circ",
]


class TestEasing(PyManimTestCase):
    def test_endpoints(self):
        lib = unreal.PyManimBlueprintLibrary
        for name in EASING_NAMES:
            v0 = lib.apply_easing_by_name(name, 0.0)
            v1 = lib.apply_easing_by_name(name, 1.0)
            self.assertNear(v0, 0.0, 0.01, f"{name}(0)={v0}")
            self.assertNear(v1, 1.0, 0.01, f"{name}(1)={v1}")

    def test_monotonic(self):
        lib = unreal.PyManimBlueprintLibrary
        for name in MONOTONIC_EASINGS:
            prev = 0.0
            for i in range(1, 21):
                t = i / 20.0
                v = lib.apply_easing_by_name(name, t)
                self.assertTrue(v >= prev - 0.001, f"{name} not monotonic at t={t}: {v} < {prev}")
                prev = v

    def test_range(self):
        lib = unreal.PyManimBlueprintLibrary
        for name in EASING_NAMES:
            for i in range(21):
                t = i / 20.0
                v = lib.apply_easing_by_name(name, t)
                self.assertTrue(-0.5 <= v <= 1.5, f"{name}({t})={v} out of range")

    def test_known_values(self):
        lib = unreal.PyManimBlueprintLibrary
        self.assertNear(lib.ease_in_quad(0.5), 0.25, 0.001)
        self.assertNear(lib.ease_out_quad(0.5), 0.75, 0.001)
        self.assertNear(lib.ease_in_cubic(0.5), 0.125, 0.001)
        self.assertNear(lib.ease_out_cubic(0.5), 0.875, 0.001)
        self.assertNear(lib.ease_linear(0.3), 0.3, 0.001)

    def test_clamp(self):
        lib = unreal.PyManimBlueprintLibrary
        self.assertNear(lib.ease_linear(-1.0), 0.0, 0.001)
        self.assertNear(lib.ease_linear(2.0), 1.0, 0.001)
        self.assertNear(lib.apply_easing_by_name("ease_in_quad", -0.5), 0.0, 0.001)
        self.assertNear(lib.apply_easing_by_name("ease_in_quad", 1.5), 1.0, 0.001)

    def test_apply_easing_by_name(self):
        lib = unreal.PyManimBlueprintLibrary
        for t in [0.0, 0.25, 0.5, 0.75, 1.0]:
            self.assertNear(lib.apply_easing_by_name("linear", t), t, 0.001)
        self.assertNear(lib.apply_easing_by_name("ease_in_quad", 0.5), 0.25, 0.001)

    def test_get_all_easing_names(self):
        lib = unreal.PyManimBlueprintLibrary
        names = lib.get_all_easing_names()
        self.assertGreaterEqual(len(names), 28)
        for name in EASING_NAMES:
            self.assertIn(name, names)

    def test_back_overshoot(self):
        lib = unreal.PyManimBlueprintLibrary
        has_below_zero = lib.ease_in_back(0.1) < 0.0
        has_above_one = lib.ease_out_back(0.9) > 1.0
        self.assertTrue(has_below_zero or has_above_one, "Back easing should overshoot [0,1]")

    def test_elastic_overshoot(self):
        lib = unreal.PyManimBlueprintLibrary
        self.assertTrue(lib.ease_out_elastic(0.5) > 1.0, "Elastic should overshoot above 1")

    def test_bounce_bounces(self):
        lib = unreal.PyManimBlueprintLibrary
        vals = [lib.ease_out_bounce(i / 100.0) for i in range(101)]
        has_decrease = False
        for i in range(1, len(vals)):
            if vals[i] < vals[i - 1]:
                has_decrease = True
                break
        self.assertTrue(has_decrease, "Bounce should have decreasing segments")


if __name__ == "__main__":
    unittest.main()
