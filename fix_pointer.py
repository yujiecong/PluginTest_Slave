import os

f_geom_cache = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeometryCacheTranslator.cpp"
with open(f_geom_cache, 'r') as f:
    content = f.read()

content = content.replace('Context->UsdInfoCache.Get()', 'Context->UsdInfoCache')

with open(f_geom_cache, 'w') as f:
    f.write(content)


f_xform = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeomXformableTranslator.cpp"
with open(f_xform, 'r') as f:
    content = f.read()

content = content.replace('Context->UsdInfoCache.Get()', 'Context->UsdInfoCache')

with open(f_xform, 'w') as f:
    f.write(content)

print("Fixed pointers.")
