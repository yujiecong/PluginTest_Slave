import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter/Source"

# We need to revert #include "MyUSD..." back to #include "Usd..." or "USD..." 
# IF the included file is NOT one of our own plugin's files.
# Let's first build a set of all header files actually present in our plugin's Source folder.
plugin_headers = set()
for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith('.h'):
            plugin_headers.add(file)

print(f"Found {len(plugin_headers)} headers in MyUSDImporter.")

# Regular expression to match #include "MyUSD..." or #include "MyUsd..."
include_pattern = re.compile(r'#include\s+["<](.*?/)?(MyUSD|MyUsd)(.*?\.h)[">]')

changed_files = 0

for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.cpp', '.h', '.cs')):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                
            def replacer(match):
                path = match.group(1) or ""
                prefix = match.group(2) # "MyUSD" or "MyUsd"
                rest = match.group(3)
                
                # Check if the fully constructed header name exists in our plugin
                header_name = f"{prefix}{rest}"
                if header_name in plugin_headers:
                    # It's one of our own headers, keep the "My" prefix
                    return match.group(0)
                else:
                    # It's an external header (like USDCore, USDUtilities, UnrealUSDWrapper, etc.)
                    # We should remove the "My" prefix.
                    original_prefix = prefix[2:] # Remove "My" -> "USD" or "Usd"
                    
                    # Special fix: MyUSDAssetCache3.h -> USDAssetCache3.h 
                    return f'#include "{path}{original_prefix}{rest}"'
            
            new_content = include_pattern.sub(replacer, content)
            
            if new_content != content:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                changed_files += 1

print(f"Fixed includes in {changed_files} files.")
