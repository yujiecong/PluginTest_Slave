import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter/Source"

changed_files = 0

for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith('.h'):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            generated_idx = -1
            last_include_idx = -1
            
            for i, line in enumerate(lines):
                if ".generated.h" in line and line.strip().startswith("#include"):
                    generated_idx = i
                elif line.strip().startswith("#include"):
                    last_include_idx = i
                    
            if generated_idx != -1 and last_include_idx > generated_idx:
                # Need to move the generated_idx line to just after last_include_idx
                gen_line = lines.pop(generated_idx)
                
                # The index of the last include might have shifted by 1 if generated_idx < last_include_idx
                insert_idx = last_include_idx if last_include_idx > generated_idx else last_include_idx + 1
                
                # We want to insert AFTER the last include
                lines.insert(insert_idx, gen_line)
                
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.writelines(lines)
                
                print(f"Fixed {file}")
                changed_files += 1

print(f"Fixed .generated.h position in {changed_files} files.")
