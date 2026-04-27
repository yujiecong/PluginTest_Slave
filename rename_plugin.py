import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter"
modules = [
    "GeometryCacheUSD",
    "USDClassesEditor",
    "USDExporter",
    "USDSchemas",
    "USDStage",
    "USDStageEditor",
    "USDStageEditorViewModels",
    "USDStageImporter",
    "USDTests"
]

# Rename .uplugin
uplugin_old = os.path.join(plugin_dir, "USDImporter.uplugin")
uplugin_new = os.path.join(plugin_dir, "MyUSDImporter.uplugin")
if os.path.exists(uplugin_old):
    os.rename(uplugin_old, uplugin_new)

# Generate replacement mappings
replacements = {
    "USDImporter": "MyUSDImporter",
    "USDIMPORTER_API": "MYUSDIMPORTER_API",
    "USDImporterModule": "MyUSDImporterModule",
    "UsdImporter": "MyUsdImporter",
    "usdImporter": "myUsdImporter",
}

for mod in modules:
    new_mod = f"My{mod}"
    replacements[mod] = new_mod
    replacements[f"{mod.upper()}_API"] = f"{new_mod.upper()}_API"

def process_file(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    new_content = content
    for old_str, new_str in replacements.items():
        new_content = new_content.replace(old_str, new_str)
        
    if new_content != content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False

# Rename folders and files
source_dir = os.path.join(plugin_dir, "Source")
if os.path.exists(source_dir):
    for mod in modules:
        old_mod_dir = os.path.join(source_dir, mod)
        new_mod_dir = os.path.join(source_dir, f"My{mod}")
        if os.path.exists(old_mod_dir):
            os.rename(old_mod_dir, new_mod_dir)

# Walk through all files and rename files, then replace content
for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        old_filepath = os.path.join(root, file)
        
        # Rename file if needed
        new_file = file
        for old_str, new_str in replacements.items():
            if old_str in new_file:
                new_file = new_file.replace(old_str, new_str)
                break # Only replace once per file name to avoid nested MyMy
        
        new_filepath = os.path.join(root, new_file)
        if old_filepath != new_filepath:
            os.rename(old_filepath, new_filepath)
            filepath = new_filepath
        else:
            filepath = old_filepath
            
        # Process content for source files and config
        if filepath.endswith(('.cs', '.cpp', '.h', '.uplugin', '.ini')):
            process_file(filepath)

print("Renaming completed.")
