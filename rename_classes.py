import os
import re

plugin_dir = "/workspace/Plugins/MyUSDImporter"

# Regex patterns to find UE types and Slate types starting with Usd or USD
patterns = [
    (re.compile(r'\bUUsd([A-Za-z0-9_]*)\b'), r'UMyUsd\1'),
    (re.compile(r'\bAUsd([A-Za-z0-9_]*)\b'), r'AMyUsd\1'),
    (re.compile(r'\bFUsd([A-Za-z0-9_]*)\b'), r'FMyUsd\1'),
    (re.compile(r'\bEUsd([A-Za-z0-9_]*)\b'), r'EMyUsd\1'),
    (re.compile(r'\bSUsd([A-Za-z0-9_]*)\b'), r'SMyUsd\1'),
    (re.compile(r'\bIUsd([A-Za-z0-9_]*)\b'), r'IMyUsd\1'),
    (re.compile(r'\bUUSD([A-Za-z0-9_]*)\b'), r'UMyUSD\1'),
    (re.compile(r'\bAUSD([A-Za-z0-9_]*)\b'), r'AMyUSD\1'),
    (re.compile(r'\bFUSD([A-Za-z0-9_]*)\b'), r'FMyUSD\1'),
    (re.compile(r'\bEUSD([A-Za-z0-9_]*)\b'), r'EMyUSD\1'),
    (re.compile(r'\bSUSD([A-Za-z0-9_]*)\b'), r'SMyUSD\1'),
    (re.compile(r'\bIUSD([A-Za-z0-9_]*)\b'), r'IMyUSD\1'),
    # Also rename the file includes if they got renamed!
]

def process_file(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    new_content = content
    for pattern, repl in patterns:
        new_content = pattern.sub(repl, new_content)
        
    if new_content != content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)

for root, dirs, files in os.walk(plugin_dir):
    for file in files:
        if file.endswith(('.cs', '.cpp', '.h', '.uplugin', '.ini')):
            process_file(os.path.join(root, file))

print("Class renaming completed.")
