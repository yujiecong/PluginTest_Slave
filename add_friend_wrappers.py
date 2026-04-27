import os

# We need to add wrappers in the specific .cpp files where the linker errors occurred:
# 1. MyUSDGeometryCacheTranslator.cpp (calls IsPotentialGeometryCacheRoot)
# 2. MyUSDGeomXformableTranslator.cpp (calls CanXformableSubtreeBeCollapsed)
# 3. MyUSDSchemasModule.cpp (calls ResetExternalTranslatorCount)
# 4. MyUSDGeomMeshTranslator.cpp or MeshTranslationImpl.cpp ? The error mentioned TryLoadingMultipleLODs...
# Wait, let's look at the exact error:

# MyUSDGeometryCacheTranslator.cpp
f_geom_cache = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeometryCacheTranslator.cpp"
with open(f_geom_cache, 'r') as f:
    content = f.read()

# Define the proxy class
proxy_geom_cache = """
#include "Objects/USDInfoCache.h"

class FUsdGeometryCacheTranslator
{
public:
    static bool CallIsPotentialGeometryCacheRoot(const FUsdInfoCache* Cache, const UE::FUsdPrim& Prim)
    {
        return Cache->IsPotentialGeometryCacheRoot(Prim);
    }
};
"""
# Insert after includes
content = content.replace('#include "MyUSDGeometryCacheTranslator.h"', '#include "MyUSDGeometryCacheTranslator.h"\n' + proxy_geom_cache)
# Replace the call
content = content.replace('Context->UsdInfoCache->IsPotentialGeometryCacheRoot(', 'FUsdGeometryCacheTranslator::CallIsPotentialGeometryCacheRoot(Context->UsdInfoCache.Get(), ')

with open(f_geom_cache, 'w') as f:
    f.write(content)


# MyUSDGeomXformableTranslator.cpp
f_xform = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeomXformableTranslator.cpp"
with open(f_xform, 'r') as f:
    content = f.read()

proxy_xform = """
#include "Objects/USDInfoCache.h"

class FUsdGeomXformableTranslator
{
public:
    static TOptional<bool> CallCanXformableSubtreeBeCollapsed(const FUsdInfoCache* Cache, const UE::FSdfPath& RootPath, FUsdSchemaTranslationContext& Context)
    {
        return Cache->CanXformableSubtreeBeCollapsed(RootPath, Context);
    }
};
"""
content = content.replace('#include "MyUSDGeomXformableTranslator.h"', '#include "MyUSDGeomXformableTranslator.h"\n' + proxy_xform)
content = content.replace('Context->UsdInfoCache->CanXformableSubtreeBeCollapsed(', 'FUsdGeomXformableTranslator::CallCanXformableSubtreeBeCollapsed(Context->UsdInfoCache.Get(), ')

with open(f_xform, 'w') as f:
    f.write(content)


# MyUSDSchemasModule.cpp
f_schemas = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDSchemasModule.cpp"
with open(f_schemas, 'r') as f:
    content = f.read()

proxy_schemas = """
#include "Objects/USDSchemaTranslator.h"

class FUsdSchemasModule
{
public:
    static void CallResetExternalTranslatorCount(FUsdSchemaTranslatorRegistry& Registry)
    {
        Registry.ResetExternalTranslatorCount();
    }
};
"""
content = content.replace('#include "MyUSDSchemasModule.h"', '#include "MyUSDSchemasModule.h"\n' + proxy_schemas)
content = content.replace('Registry.ResetExternalTranslatorCount();', 'FUsdSchemasModule::CallResetExternalTranslatorCount(Registry);')

with open(f_schemas, 'w') as f:
    f.write(content)


# Now for pxrInternal_v0_25_8__pxrReserved__::TfToken::_GetEmptyString
# This usually happens because TfToken() is not exported properly on Windows when used inline, or similar USD C++ library issue.
# Wait! In UE, we use `UsdUtils::GetEmptyToken()` or something?
# No, `TfToken()` constructor should be used? Wait, in UE's `pxr` wrapper, they disable inline for some methods.
# The error is in `FMaterialXUsdShadeMaterialTranslator::CreateAssets(void)`
# Let's check MaterialXUSDShadeMaterialTranslator.cpp

f_matx = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/Custom/MaterialXUSDShadeMaterialTranslator.cpp"
with open(f_matx, 'r') as f:
    content = f.read()

# We can replace TfToken() or similar empty token usages.
# Where does it call `_GetEmptyString`?
# Usually it's `pxr::TfToken()` or `pxr::TfToken("")`.
# Let's look for `TfToken()` or `TfToken("")`
print(content.count('TfToken()'))
