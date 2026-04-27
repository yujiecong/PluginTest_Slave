f_mesh = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeomMeshTranslator.cpp"
with open(f_mesh, 'r') as f:
    content = f.read()

lines = content.split('\n')
for i, line in enumerate(lines):
    if 'TryLoadingMultipleLODs' in line:
        for j in range(i, min(i+300, len(lines))):
            if 'UsdTimeCode' in lines[j]:
                print(f"Line {j+1}: {lines[j]}")

