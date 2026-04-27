import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter"

def get_new_name(filename):
    if filename.startswith("USD"):
        return "My" + filename
    elif filename.startswith("Usd"):
        return "My" + filename
    elif filename.startswith("SUSD"):
        return "SMy" + filename[1:]
    elif filename.startswith("SUsd"):
        return "SMy" + filename[1:]
    elif filename.startswith("IUSD"):
        return "IMy" + filename[1:]
    elif filename.startswith("IUsd"):
        return "IMy" + filename[1:]
    elif filename.startswith("UUSD"):
        return "UMy" + filename[1:]
    elif filename.startswith("UUsd"):
        return "UMy" + filename[1:]
    elif filename.startswith("FUSD"):
        return "FMy" + filename[1:]
    elif filename.startswith("FUsd"):
        return "FMy" + filename[1:]
    return filename

file_map = {}

# Pass 1: collect renames
for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.h', '.cpp', '.cs')):
            new_file = get_new_name(file)
            if new_file != file:
                old_path = os.path.join(root, file)
                new_path = os.path.join(root, new_file)
                file_map[file] = new_file
                os.rename(old_path, new_path)

# Pass 2: replace includes
def replace_includes(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
        
    new_content = content
    for old_file, new_file in file_map.items():
        new_content = new_content.replace(f'"{old_file}"', f'"{new_file}"')
        new_content = new_content.replace(f'/{old_file}"', f'/{new_file}"')
        
    if new_content != content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)

for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.cs', '.cpp', '.h')):
            replace_includes(os.path.join(root, file))

print(f"Renamed {len(file_map)} files.")
