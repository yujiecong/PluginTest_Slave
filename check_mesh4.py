f_mesh = "/workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MeshTranslationImpl.cpp"
with open(f_mesh, 'r') as f:
    content = f.read()

# Instead of pxr::UsdTimeCode(Context.Time), we should probably use the Unreal USD wrapper or just UsdTimeCode::Default() if Context.Time is default.
# Actually, the error says: pxrInternal_v0_25_8__pxrReserved__::UsdTimeCode::_IssueGetValueOnDefaultError(void)const
# This happens when you try to get a value from a default timecode and it errors out, or the inline constructor of UsdTimeCode uses it.
# To avoid the linker error, we can use the Unreal wrapper:
# double Time = Context.Time;
# Or maybe UE has a wrapper for UsdTimeCode? Yes, but usually they just use `Context.Time`.
# Wait, `Context.Time` is a float or double. The constructor of `pxr::UsdTimeCode(double)` is inline but might call something that is not exported?
# What if we change `pxr::UsdTimeCode(Context.Time)` to `pxr::UsdTimeCode((double)Context.Time)`?
# Let's see what is on lines 802 and 864.
lines = content.split('\n')
for i in range(800, 810):
    print(f"Line {i}: {lines[i]}")
print("---")
for i in range(860, 870):
    print(f"Line {i}: {lines[i]}")
