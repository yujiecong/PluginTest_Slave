import os

plugin_dir = "/workspace/Plugins/MyUSDImporter/Source"

changed_files = 0

for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith('.h'):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()

            # We need to find the correct expected generated file name for THIS file.
            # Usually it is FileBaseName.generated.h
            # But here the file was renamed from USD... to MyUSD...
            # The generated header name inside MUST match the current filename base.
            
            base_name = os.path.splitext(file)[0]
            expected_generated = f"{base_name}.generated.h"
            
            new_lines = []
            file_changed = False
            for line in lines:
                if ".generated.h" in line and line.strip().startswith("#include"):
                    # Check if the generated.h name matches expected
                    current_generated = line.split('"')[1] if '"' in line else ""
                    if current_generated and current_generated != expected_generated:
                        line = f'#include "{expected_generated}"\n'
                        file_changed = True
                new_lines.append(line)

            if file_changed:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.writelines(new_lines)
                print(f"Updated generated.h include in {file}")
                changed_files += 1

print(f"Total files updated: {changed_files}")
