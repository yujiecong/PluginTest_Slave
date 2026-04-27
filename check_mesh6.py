f_mesh = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MeshTranslationImpl.cpp"
with open(f_mesh, 'r') as f:
    content = f.read()

# Replace `pxr::UsdTimeCode(Context.Time)` with `pxr::UsdTimeCode(Context.Time).GetValue()`? No, GetPrimMaterialAssignments probably takes `double`.
# Wait, let's see if we can just pass `Context.Time` instead of `pxr::UsdTimeCode(Context.Time)`.
# Actually, the parameter is likely `const double Time` or `double Time`.
content = content.replace('pxr::UsdTimeCode(Context.Time)', 'Context.Time')

with open(f_mesh, 'w') as f:
    f.write(content)

