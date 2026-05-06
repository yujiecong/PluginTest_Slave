import unittest
import json
import os
import time
import sys

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
REPORT_DIR = os.path.join(TESTS_DIR, "test_report")


class PyManimTestCase(unittest.TestCase):
    def assertNear(self, a, b, tolerance=0.01, msg=""):
        self.assertTrue(abs(a - b) <= tolerance, f"{msg} Expected {b} +/- {tolerance}, got {a}")

    def assertNoCrash(self, func, msg=""):
        try:
            func()
        except Exception as e:
            self.fail(f"{msg} Expected no crash, but got {type(e).__name__}: {e}")

    @staticmethod
    def make_scene():
        import unreal
        scene = unreal.PyManimScene()
        scene.initialize(1920, 1080)
        return scene

    @staticmethod
    def tick_animation(scene, duration, fps=30):
        total_frames = int(duration * fps) + 1
        for _ in range(total_frames):
            scene.tick(1.0 / fps)

    @staticmethod
    def cleanup_scene(scene):
        if scene:
            try:
                scene.destroy()
            except:
                pass


class JsonReportRunner(unittest.TextTestRunner):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._results = []

    def run(self, test):
        start = time.time()
        result = super().run(test)
        duration = round(time.time() - start, 2)

        tests_data = []
        for test_case, traceback_str in result.failures + result.errors:
            tests_data.append({"name": str(test_case), "status": "fail", "error": traceback_str})
        for test_case, traceback_str in result.unexpectedSuccesses if hasattr(result, 'unexpectedSuccesses') else []:
            tests_data.append({"name": str(test_case), "status": "fail", "error": "unexpected success"})
        for test_case, reason in result.skipped:
            tests_data.append({"name": str(test_case), "status": "skip", "error": reason})

        passed_count = result.testsRun - len(result.failures) - len(result.errors) - len(result.skipped)
        for _ in range(passed_count):
            tests_data.append({"name": "", "status": "pass", "error": ""})

        report = {
            "timestamp": time.strftime('%Y-%m-%dT%H:%M:%S'),
            "total": result.testsRun,
            "passed": passed_count,
            "failed": len(result.failures) + len(result.errors),
            "skipped": len(result.skipped),
            "duration_s": duration,
            "tests": tests_data
        }

        if not os.path.exists(REPORT_DIR):
            os.makedirs(REPORT_DIR, exist_ok=True)
        report_path = os.path.join(REPORT_DIR, "report.json")
        with open(report_path, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        print(f"\nJSON report saved: {report_path}")

        return result
