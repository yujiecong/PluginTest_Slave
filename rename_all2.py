import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter"

modules = [
    "USDStageEditorViewModels",
    "USDStageImporter",
    "USDClassesEditor",
    "GeometryCacheUSD",
    "USDStageEditor",
    "USDExporter",
    "USDSchemas",
    "USDStage",
    "USDTests"
]

str_replacements = {
    "USDImporter": "MyUSDImporter",
    "UsdImporter": "MyUsdImporter",
    "usdImporter": "myUsdImporter",
    "USDIMPORTER_API": "MYUSDIMPORTER_API",
    "USDIMPORTER": "MYUSDIMPORTER"
}

for m in modules:
    str_replacements[m] = f"My{m}"
    str_replacements[f"{m.upper()}_API"] = f"MY{m.upper()}_API"

# Sort keys by length descending to avoid partial replacements
sorted_keys = sorted(str_replacements.keys(), key=len, reverse=True)

class_pattern = re.compile(r'\b([UAFESI])(Usd|USD)([A-Za-z0-9_]*)\b')

def rename_content(content):
    # First, rename specific module names and macros
    for old_s in sorted_keys:
        new_s = str_replacements[old_s]
        content = content.replace(old_s, new_s)
        
    # Then rename UE classes
    content = class_pattern.sub(r'\1My\2\3', content)
    return content

# Rename folders inside Source
source_dir = os.path.join(plugin_dir, "Source")
for m in modules:
    old_mod_dir = os.path.join(source_dir, m)
    new_mod_dir = os.path.join(source_dir, f"My{m}")
    if os.path.exists(old_mod_dir):
        os.rename(old_mod_dir, new_mod_dir)

# Rename files
for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.cs', '.cpp', '.h', '.uplugin', '.ini')):
            new_file = file
            for old_s in sorted_keys:
                new_s = str_replacements[old_s]
                if old_s in new_file:
                    new_file = new_file.replace(old_s, new_s)
                    break 
                    
            if file == "USDImporter.uplugin":
                new_file = "MyUSDImporter.uplugin"
                
            if new_file == file and file.endswith(('.h', '.cpp')):
                match = re.match(r'^([SIUAFE]?)(USD|Usd)(.*\.h|.*\.cpp)$', file)
                if match:
                    prefix, usd, rest = match.groups()
                    new_file = f"{prefix}My{usd}{rest}"
                    
            if new_file != file:
                os.rename(os.path.join(root, file), os.path.join(root, new_file))

# Process content
for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.cs', '.cpp', '.h', '.uplugin', '.ini')):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                
            new_content = rename_content(content)
            
            def include_replacer(m):
                path = m.group(1) or ""
                prefix = m.group(2) or ""
                usd = m.group(3)
                rest = m.group(4)
                if "pxr" in path.lower() or "usd/usd" in path.lower():
                    return m.group(0)
                return f'#include "{path}{prefix}My{usd}{rest}"'
                
            new_content = re.sub(r'#include\s+"(.*?/)?([SIUAFE]?)(USD|Usd)(.*?\.h)"', include_replacer, new_content)
            
            if new_content != content:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(new_content)

print("All renaming completed safely.")
