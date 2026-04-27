import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter/Source"

changed_files = 0

for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.cpp', '.h', '.cs')):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()

            new_content = content
            
            # For USDInfoCache.h
            if '#include "Objects/USDInfoCache.h"' in new_content:
                hack = """#define private public
#include "Objects/USDInfoCache.h"
#undef private"""
                new_content = new_content.replace('#include "Objects/USDInfoCache.h"', hack)
                
            # For USDSchemaTranslator.h
            if '#include "Objects/USDSchemaTranslator.h"' in new_content:
                hack = """#define private public
#include "Objects/USDSchemaTranslator.h"
#undef private"""
                new_content = new_content.replace('#include "Objects/USDSchemaTranslator.h"', hack)

            # Let's also check if they use `#include "Objects/MyUSDInfoCache.h"` which shouldn't exist anymore, but just in case
            if '#include "Objects/MyUSDInfoCache.h"' in new_content:
                hack = """#define private public
#include "Objects/USDInfoCache.h"
#undef private"""
                new_content = new_content.replace('#include "Objects/MyUSDInfoCache.h"', hack)

            if new_content != content:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                changed_files += 1
                print(f"Hacked {file}")

print(f"Hacked {changed_files} files.")
