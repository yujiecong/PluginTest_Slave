import re
f_matx = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/Custom/MaterialXUSDShadeMaterialTranslator.cpp"
with open(f_matx, 'r') as f:
    content = f.read()

# Replace TfToken usages to avoid `_GetEmptyString`.
# UE usually provides `UnrealToUsd::ConvertToken` which doesn't trigger it if we don't default construct.
# Wait, `const static pxr::TfToken` is the problem?
# Let's change it to not be static.
content = content.replace('const static pxr::TfToken', 'const pxr::TfToken')
with open(f_matx, 'w') as f:
    f.write(content)


f_mesh = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MeshTranslationImpl.cpp"
with open(f_mesh, 'r') as f:
    content = f.read()

# In MeshTranslationImpl.cpp, `Context.Time` is passed to `pxr::UsdTimeCode(Context.Time)`.
# Actually, the error says: "UsdTimeCode::_IssueGetValueOnDefaultError(void)const"
# This means something is calling `.GetValue()` on a default UsdTimeCode, or it's inline in the header.
# Wait! In UE 5, `Context.Time` is `double` or `float`?
# Maybe we can use `UsdUtils::GetTimeCode(Context.Time)` if it exists? Or maybe `Context.Time` is `float` but TimeCode needs `double`?
# What if we just use `pxr::UsdTimeCode((double)Context.Time)`?
# Actually, UE provides `UE::FUsdTimeCode`.
# `pxr::UsdTimeCode` constructor from double might be inline and call something.
# Let's see if we can use `Context.Time` directly? No, `GetPrimMaterialAssignments` expects `pxr::UsdTimeCode`?
# No, `UsdUtils::GetPrimMaterialAssignments` is a UE wrapper, it usually takes `double`!
# Let's check `UsdUtils::GetPrimMaterialAssignments` signature in USDUtilities.
