f_matx = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/Custom/MaterialXUSDShadeMaterialTranslator.cpp"
with open(f_matx, 'r') as f:
    content = f.read()

# Replace static const pxr::TfToken RenderContextToken = ... with just using it inline or omitting `static const`?
# Wait, `UnrealToUsd::ConvertToken` returns an optional. `.Get()` returns TfToken.
# But `TfToken` has a default constructor that calls `_GetEmptyString()`.
# If `ConvertToken` fails, `.Get()` might throw, or if it's default constructed.
# Wait, `const static pxr::TfToken` might trigger default construction during static initialization?
# Let's change it to a macro or function.
content = content.replace('const static pxr::TfToken RenderContextToken', 'const pxr::TfToken RenderContextToken')

with open(f_matx, 'w') as f:
    f.write(content)


f_mesh = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeomMeshTranslator.cpp"
with open(f_mesh, 'r') as f:
    content = f.read()

# "UsdTimeCode::_IssueGetValueOnDefaultError"
# This happens when `pxr::UsdTimeCode::Default()` is passed or similar?
# Wait, the error is in `MyUSDGeomMeshTranslator.cpp` inside `TryLoadingMultipleLODs` lambda 2.
# Let's check `TryLoadingMultipleLODs`
import re
lines = content.split('\n')
for i, line in enumerate(lines):
    if 'TryLoadingMultipleLODs' in line:
        for j in range(i, min(i+50, len(lines))):
            if 'UsdTimeCode' in lines[j]:
                print(f"Line {j+1}: {lines[j]}")

