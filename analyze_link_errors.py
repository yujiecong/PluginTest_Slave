# The error shows 5 unresolved external symbols:
# 1. FUsdInfoCache::CanXformableSubtreeBeCollapsed
# 2. FUsdInfoCache::IsPotentialGeometryCacheRoot
# 3. pxrInternal_v0_25_8__pxrReserved__::TfToken::_GetEmptyString
# 4. pxrInternal_v0_25_8__pxrReserved__::UsdTimeCode::_IssueGetValueOnDefaultError
# 5. FUsdSchemaTranslatorRegistry::ResetExternalTranslatorCount
# 
# Wait, why are they unresolved?
# Because the Unreal USDUtilities module does NOT export them with USDUTILITIES_API, OR it does export them but because they are private, the linker sees them differently?
# Actually, the error says:
# __declspec(dllimport) public: struct TOptional<bool> __cdecl FUsdInfoCache::CanXformableSubtreeBeCollapsed
# Wait! In the error message it says "public: struct TOptional<bool> ...".
# But originally they were declared private!
# Because we used `#define private public`, the compiler saw them as `public` in our plugin, so it generated a symbol reference with the `public` mangled name.
# BUT in the engine DLL, they were compiled as `private`, so their mangled name in the engine DLL has the `private` access specifier!
# MSVC name mangling includes access specifiers (public/protected/private)!
# That's why `#define private public` causes LNK2019 in MSVC!

# How to fix this?
# We CANNOT use `#define private public` in MSVC for classes that have methods we want to link against, because MSVC mangles the access specifier into the function name.
# 
# So we MUST undo `#define private public` for USDInfoCache.h and USDSchemaTranslator.h.
# But then how do we access those private methods?
# 
# Answer: We can just define a class with the ORIGINAL name (e.g. FUsdGeomXformableTranslator) in a namespace or something? No, friend declarations don't care about namespaces if the original was in the global namespace.
# Wait! In C++, `friend class FUsdGeomXformableTranslator;` refers to the class `FUsdGeomXformableTranslator`.
# If we define a dummy class `FUsdGeomXformableTranslator` in our plugin, we can make IT call the private methods!
# Let's write a small wrapper struct in our plugin:
# struct FUsdGeomXformableTranslator {
#     static TOptional<bool> CallCanXformableSubtreeBeCollapsed(const FUsdInfoCache* Cache, const UE::FSdfPath& RootPath, FUsdSchemaTranslationContext& Context) {
#         return Cache->CanXformableSubtreeBeCollapsed(RootPath, Context);
#     }
# };
# 
# Wait! Is `FUsdGeomXformableTranslator` already defined by the Engine's USDImporter module?
# The Engine's USDImporter module is NOT included or linked by our plugin (we are replacing it, so we don't depend on it in Build.cs).
# Our plugin depends on `USDCore`, `USDUtilities`, etc.
# Does `USDUtilities` define `FUsdGeomXformableTranslator`? NO! It only declares it as a friend! `class FUsdGeomXformableTranslator;`
# So WE can define `FUsdGeomXformableTranslator` in our code just to act as a proxy!
# Let's verify this!
