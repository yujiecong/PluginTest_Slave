import os

filepath = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Public/MyUSDPrimLinkCache.h"

with open(filepath, 'r') as f:
    content = f.read()

content = content.replace('#include "Objects/MyUSDPrimLinkCache.h"', '#include "Objects/USDPrimLinkCache.h"')

with open(filepath, 'w') as f:
    f.write(content)

# Also check other files for `#include "Objects/MyUSDPrimLinkCache.h"`
plugin_dir = "/workspace/Plugins/MyUSDImporter/Source"
for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.cpp', '.h', '.cs')):
            fp = os.path.join(root, file)
            with open(fp, 'r', encoding='utf-8', errors='ignore') as f:
                c = f.read()
            if '#include "Objects/MyUSDPrimLinkCache.h"' in c:
                c = c.replace('#include "Objects/MyUSDPrimLinkCache.h"', '#include "Objects/USDPrimLinkCache.h"')
                with open(fp, 'w', encoding='utf-8') as f:
                    f.write(c)
                print(f"Fixed {file}")
