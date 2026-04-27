# The UE build runs on the user's Windows machine, not in this linux container!
# We don't have C:/Program Files/Epic Games/UE_5.7 here.
# But we can infer what happened:
# FUsdInfoCache has private members IsPotentialGeometryCacheRoot and CanXformableSubtreeBeCollapsed.
# In the original USDImporter, FUsdGeomMeshTranslator, FUsdGeomXformableTranslator, etc. were friend classes of FUsdInfoCache.
# Now that we renamed our translators to FMyUsdGeomXformableTranslator, they are NO LONGER friends!
# And we CANNOT edit FUsdInfoCache (it's in the engine).
# 
# How to bypass C++ access control?
# We can declare a struct that inherits from FUsdInfoCache? No, we don't know if it has protected/virtual.
# Wait, if FUsdGeomXformableTranslator is a friend, what if we define a dummy FUsdGeomXformableTranslator class in our plugin just to call the private methods?
# NO, FUsdGeomXformableTranslator is already defined in the Engine's USDImporter module! We would get a redefinition error if we define it.
# Wait, does the engine's USDImporter export FUsdGeomXformableTranslator? If not exported, we could define it. But it might be exported.
#
# Wait, there's a well-known C++ trick to access private members:
# template<typename Tag> struct Result { typedef typename Tag::type type; static type ptr; };
# template<typename Tag> typename Result<Tag>::type Result<Tag>::ptr;
# template<typename Tag, typename Tag::type p> struct Rob { struct Filler { Filler() { Result<Tag>::ptr = p; } }; static Filler filler; };
# template<typename Tag, typename Tag::type p> typename Rob<Tag, p>::Filler Rob<Tag, p>::filler;
#
# But UE has another way: just define a macro before including the header!
# #define private public
# #include "Objects/USDInfoCache.h"
# #undef private
# 
# Let's check if we can do this in the files that use it.
