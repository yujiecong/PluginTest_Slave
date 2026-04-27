import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter/Source"

changed_files = 0

for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.h', '.cpp', '.cs')):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()

            # Fix specific includes
            new_content = content.replace('#include "Objects/MyUSDSchemaTranslator.h"', '#include "Objects/USDSchemaTranslator.h"')
            new_content = new_content.replace('#include "Objects/MyUSDInfoCache.h"', '#include "Objects/USDInfoCache.h"')
            new_content = new_content.replace('UE_INLINE_GENERATED_CPP_BY_NAME(USDAssetCacheFactory)', 'UE_INLINE_GENERATED_CPP_BY_NAME(MyUSDAssetCacheFactory)')

            if new_content != content:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                changed_files += 1
                print(f"Fixed {file}")

print(f"Fixed {changed_files} files.")
