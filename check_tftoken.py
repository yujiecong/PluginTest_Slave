import re
f_matx = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/Custom/MaterialXUSDShadeMaterialTranslator.cpp"
with open(f_matx, 'r') as f:
    content = f.read()

# Look for TfToken
print("TfToken usage:")
for line in content.split('\n'):
    if 'TfToken' in line:
        print(line.strip())

f_mesh = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MeshTranslationImpl.cpp"
with open(f_mesh, 'r') as f:
    content = f.read()

print("UsdTimeCode usage:")
for line in content.split('\n'):
    if 'UsdTimeCode' in line:
        print(line.strip())
