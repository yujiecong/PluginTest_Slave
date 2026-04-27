import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter/Source"

changed_files = 0

# Replace UE_INLINE_GENERATED_CPP_BY_NAME(USD...) with UE_INLINE_GENERATED_CPP_BY_NAME(MyUSD...)
# We also have some UE_INLINE_GENERATED_CPP_BY_NAME(SUSDIntegrationsPanel) -> SMyUSDIntegrationsPanel?
# Let's just match UE_INLINE_GENERATED_CPP_BY_NAME\((.*?)\)

for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith('.cpp'):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
                
            file_changed = False
            for i, line in enumerate(lines):
                if 'UE_INLINE_GENERATED_CPP_BY_NAME(' in line:
                    # Extract the class name inside the macro
                    match = re.search(r'UE_INLINE_GENERATED_CPP_BY_NAME\((.*?)\)', line)
                    if match:
                        name = match.group(1)
                        
                        # We need to make sure the name inside the macro matches the file name (without extension)
                        expected_name = os.path.splitext(file)[0]
                        
                        if name != expected_name:
                            lines[i] = line.replace(f'({name})', f'({expected_name})')
                            file_changed = True
                            
            if file_changed:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.writelines(lines)
                print(f"Fixed {file}")
                changed_files += 1

print(f"Total files updated: {changed_files}")
