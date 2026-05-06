import unittest
import sys
import os

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TESTS_DIR)
sys.path.insert(0, os.path.join(TESTS_DIR, '..'))

from test_framework import JsonReportRunner


def main():
    filter_pattern = "test_"
    for arg in sys.argv[1:]:
        if arg.startswith("--filter="):
            filter_pattern = arg.split("=", 1)[1]

    loader = unittest.TestLoader()
    loader.testNamePattern = filter_pattern if filter_pattern != "test_" else "test*"
    suite = loader.discover(TESTS_DIR, pattern="test_*.py")

    print("=" * 64)
    print("  PyManim Automated Test Suite")
    print("=" * 64)

    runner = JsonReportRunner(verbosity=2)
    result = runner.run(suite)

    print("\n" + "=" * 64)
    if result.wasSuccessful():
        print("  Result: ALL TESTS PASSED")
    else:
        print(f"  Result: {len(result.failures)} FAILURES, {len(result.errors)} ERRORS")
    print("=" * 64)

    sys.exit(0 if result.wasSuccessful() else 1)


if __name__ == "__main__":
    main()
