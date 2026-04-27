import os

plugin_dir = "/workspace/Plugins/MyUSDImporter/Source"

changed_files = 0

# First, revert the #define private public hacks
for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.cpp', '.h', '.cs')):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()

            new_content = content
            
            hack1 = """#define private public
#include "Objects/USDInfoCache.h"
#undef private"""
            new_content = new_content.replace(hack1, '#include "Objects/USDInfoCache.h"')
            
            hack2 = """#define private public
#include "Objects/USDSchemaTranslator.h"
#undef private"""
            new_content = new_content.replace(hack2, '#include "Objects/USDSchemaTranslator.h"')

            if new_content != content:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                changed_files += 1
                print(f"Reverted hack in {file}")

print(f"Reverted hacks in {changed_files} files.")
