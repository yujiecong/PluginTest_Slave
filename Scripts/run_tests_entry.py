import unreal
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'tests'))
from tests.run_all_tests import main

try:
    exit_code = main()
except Exception as e:
    unreal.log(f"[ERROR] Test runner crashed: {e}")
    exit_code = 1

from tests.run_all_tests import cleanup
cleanup()
