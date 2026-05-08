# UEMotion Asset-First 架构重构方案

> **设计目标**: 从 "运行时直接创建 Actor" 改为 "先生成持久化 UAsset，再从 Asset 实例化 Actor"
>
> **核心优势**: 可编辑、可复用、可版本控制、符合 UE 最佳实践

---

## 一、问题诊断：当前架构的局限性

### 1.1 当前流程（Actor-First 模式）

```
Python 脚本
    ↓
Scene.cube(size=50, color="blue")
    ↓
UEMotionScene::CreateCube()
    ↓  [C++ 层]
World->SpawnActor<AActor>()           ← 直接创建临时 Actor
    ↓
NewObject<UStaticMeshComponent>()      ← 动态添加组件
    ↓
LoadObject<UStaticMesh>("/Engine/BasicShapes/Cube.Cube")  ← 加载引擎内置网格
    ↓
UMaterialInstanceDynamic::Create()     ← 运行时创建材质实例
    ↓
返回 Mobject（包装这个临时 Actor）
```

**关键代码位置**:
- [`UEMotionMobject.cpp:79-122`](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionMobject.cpp#L79-L122) - `CreateStaticMeshActor()` 方法
- [`UEMotionScene.cpp:290-304`](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp#L290-L304) - `CreateCube()` 方法

### 1.2 存在的问题

| 问题 | 影响 | 严重程度 |
|------|------|----------|
| **❌ 无法在 Editor 中修改** | 每次运行都重新创建，无法手动调整材质/网格 | 🔴 致命 |
| **❌ 无持久化** | 关闭 Editor 后所有配置丢失 | 🔴 致命 |
| **❌ 无法复用** | 相同配置需要重复创建 | 🟠 严重 |
| **❌ 无版本控制** | .uasset 不存在，无法 Git 管理 | 🟠 严重 |
| **⚠️ 调试困难** | 无法在 Content Browser 中查看对象 | 🟡 中等 |
| **⚠️ 性能浪费** | 每次都重新加载引擎基础网格 | 🟡 中等 |

### 1.3 用户痛点场景

```python
# 场景：想调整 Cube 的材质
s = Scene("test")
cube = s.cube(50, color="blue")  # 创建了一个蓝色立方体

# 问题：
# 1. 想改成金属质感？→ 必须改代码重新运行
# 2. 想加法线贴图？→ 不支持，没有 .uasset 可以编辑
# 3. 想保存为模板复用？→ 关闭就没了
# 4. 团队协作？→ 其他人看不到你的配置
```

---

## 二、新架构设计：Asset-First 模式

### 2.1 核心思想

**"一切皆资产" (Everything is an Asset)**

借鉴 UE 的 Blueprint 系统和 Manim 的 Style 系统：

```
┌─────────────────────────────────────────────────────┐
│                  Asset Layer (持久化层)                │
│                                                     │
│  /Game/UEMotion/Assets/                             │
│  ├── Meshes/          ← StaticMesh .uasset          │
│  │   ├── SM_Cube_50.uasset                         │
│  │   ├── SM_Sphere_40.uasset                       │
│  │   └── ...                                       │
│  ├── Materials/       ← MaterialInstance .uasset    │
│  │   ├── MI_Blue.uasset                            │
│  │   ├── MI_Red_Metallic.uasset                    │
│  │   └── ...                                       │
│  └── Blueprints/       ← ActorBlueprint .uasset     │
│      ├── BP_Cube_Blue.uasset        ⭐ 核心         │
│      ├── BP_Sphere_Red.uasset                      │
│      └── BP_CustomShape.uasset                     │
└─────────────────────────────────────────────────────┘
                        ↓ 实例化
┌─────────────────────────────────────────────────────┐
│                Runtime Layer (运行时层)               │
│                                                     │
│  从 Blueprint Spawn Actor                           │
│  ↓                                                  │
│  可以覆盖部分属性：位置、旋转、动画参数              │
│  但不能改变：网格、材质、物理设置（除非显式指定）    │
└─────────────────────────────────────────────────────┘
```

### 2.2 新的工作流程

#### 阶段 1: Asset 定义（一次性或按需更新）

```python
from uemotion import Scene, AssetLibrary

# 初始化场景
s = Scene("my_animation")

# 方式 A: 使用预定义模板（推荐）
cube = s.cube_from_asset("BP_Cube_Standard")  # 从已有 Blueprint 加载

# 方式 B: 创建并保存为新 Asset
asset = s.create_asset(
    mesh_type="cube",
    size=50,
    material_config={
        "color": "#3498db",
        "metallic": 0.3,
        "roughness": 0.5,
        "normal_map": "/Game/Textures/T_Normal"
    },
    asset_name="MyCustomCube",  # 保存为 /Game/UEMotion/Assets/MyCustomCube.uasset
    save_to_disk=True          # 持久化到磁盘
)

# 后续可以直接使用
cube = s.spawn_from_asset(asset)  # 快速实例化
cube.location = (100, 0, 0)
```

#### 阶段 2: Actor 实例化与动画

```python
# 从 Asset Spawn 的 Actor 支持完整动画
cube.move_to((200, 0, 0), duration=1.0)
cube.rotate(360, axis=(0, 0, 1), duration=2.0)
cube.scale_to(1.5, duration=1.5)

s.play()
s.render("output.mp4")
```

### 2.3 目录结构规范

```
PluginTest/
├── Game/
│   └── UEMotion/                        ← 所有 UEMotion 资产根目录
│       ├── Assets/                      ← 用户定义的资产
│       │   ├── Meshes/                 ← 自定义网格（可选）
│       │   │   └── *.uasset
│       │   ├── Materials/              ← 材质实例
│       │   │   ├── MI_PrimaryColors/   ← 基础颜色
│       │   │   │   ├── MI_Red.uasset
│       │   │   │   ├── MI_Blue.uasset
│       │   │   │   └── ...
│       │   │   ├── MI_Metallic/        ← 金属质感
│       │   │   ├── MI_Glass/           ← 玻璃效果
│       │   │   └── MI_Emissive/        ← 自发光
│       │   ├── Blueprints/             ← Actor 蓝图 ⭐ 核心
│       │   │   ├── BP_Cube_Standard.uasset
│       │   │   ├── BP_Sphere_Standard.uasset
│       │   │   ├── BP_Cylinder_Standard.uasset
│       │   │   ├── BP_Text_2D.uasset
│       │   │   └── Custom/             ← 用户自定义
│       │   │       └── BP_MyCharacter.uasset
│       │   └── Templates/              ← 预设模板
│       │       ├── DefaultCube.json     ← 默认配置
│       │       └── GlassSphere.json
│       │
│       ├── Scenes/                     ← 场景 Map（已有）
│       │   └── *.umap
│       ├── Sequences/                  ← LevelSequence（已有）
│       │   └── LS_*.uasset
│       └── Cache/                      ← 运行时缓存（不提交 Git）
│           └── Temp_*.uasset
│
└── Scripts/
    └── uemotion/
        ├── __init__.py
        ├── scene.py                   ← 需要改造
        ├── mobject.py                 ← 需要改造
        ├── asset_library.py           ← 新增！资产管理器
        ├── asset_templates.py         ← 新增！预设模板
        └── constants.py
```

---

## 三、详细技术方案

### 3.1 C++ 层改动

#### 3.1.1 新增类：`UUEMotionAssetFactory`

**文件**: `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionAssetFactory.h/.cpp`

**职责**: 负责创建和管理持久化的 UAsset

```cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionAssetFactory.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceConstant;
class AActor;

// 资产配置结构体
USTRUCT(BlueprintType)
struct FMotionAssetConfig
{
    GENERATED_BODY()

    // 几何属性
    UPROPERTY(EditAnywhere, Category = "Geometry")
    FString MeshType;  // "cube", "sphere", "cylinder", etc.

    UPROPERTY(EditAnywhere, Category = "Geometry")
    float Size = 50.0f;

    // 材质属性
    UPROPERTY(EditAnywhere, Category = "Material")
    FLinearColor BaseColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Material", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Metallic = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Material", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Roughness = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Material")
    FString NormalMapPath;  // 法线贴图路径

    UPROPERTY(EditAnywhere, Category = "Material")
    float Opacity = 1.0f;

    // 物理属性
    UPROPERTY(EditAnywhere, Category = "Physics")
    bool bSimulatePhysics = false;
};

UCLASS(BlueprintType)
class UUEMotionAssetFactory : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 创建并保存 StaticMesh Asset 到磁盘
     * @param Config 资产配置
     * @param AssetName 资产名称
     * @param OutPackagePath 输出路径 (/Game/UEMotion/Assets/Meshes/)
     * @return 创建的 StaticMesh 对象
     */
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
    UStaticMesh* CreateAndSaveMeshAsset(
        const FMotionAssetConfig& Config,
        const FString& AssetName,
        const FString& OutPackagePath = TEXT("/Game/UEMotion/Assets/Meshes/")
    );

    /**
     * 创建并保存 MaterialInstance Asset 到磁盘
     */
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
    UMaterialInstanceConstant* CreateAndSaveMaterialAsset(
        const FMotionAssetConfig& Config,
        const FString& AssetName,
        const FString& OutPackagePath = TEXT("/Game/UEMotion/Assets/Materials/")
    );

    /**
     * 创建完整的 Actor Blueprint Asset（包含网格+材质+预设组件）
     * 这是核心方法！生成可直接 Spawn 的蓝图
     */
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
    UClass* CreateAndSaveBlueprintAsset(
        const FMotionAssetConfig& Config,
        const FString& AssetName,
        const FString& OutPackagePath = TEXT("/Game/UEMotion/Assets/Blueprints/")
    );

    /**
     * 从已有的 Blueprint Asset Spawn Actor
     */
    UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
    AActor* SpawnFromBlueprintAsset(
        UObject* WorldContext,
        const FString& BlueprintPath,
        const FVector& Location = FVector::ZeroVector,
        const FRotator& Rotation = FRotator::ZeroRotator,
        const FVector& Scale = FVector::OneVector
    );

    /**
     * 检查 Asset 是否已存在
     */
    UFUNCTION(BlueprintPure, Category = "UEMotion|Asset")
    static bool DoesAssetExist(const FString& AssetPath);

    /**
     * 加载已有的 Asset
     */
    template<typename T>
    static T* LoadAsset(const FString& AssetPath)
    {
        return LoadObject<T>(nullptr, *AssetPath);
    }

private:
    // 内部辅助方法
    UStaticMesh* GenerateProceduralMesh(const FMotionAssetConfig& Config);
    UMaterialInstanceConstant* ConfigureMaterial(const FMotionAssetConfig& Config, UMaterial* BaseMaterial);
    bool SaveAssetToObject(UObject* Asset, const FString& PackagePath, const FString& AssetName);
};
```

#### 3.1.2 改造 `UEMotionScene` 类

**新增方法**:

```cpp
// 在 UUEMotionScene.h 中添加

/**
 * 基于 Asset 配置创建 Mobject（先查找缓存，找不到则创建新 Asset）
 */
UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
UUEMotionMobject* CreateMobjectFromAsset(
    const FString& AssetNameOrPath,  // 可以是名称或完整路径
    const FMotionAssetConfig& OverrideConfig = FMotionAssetConfig(),  // 可选覆盖
    bool bForceRecreate = false  // 是否强制重建
);

/**
 * 注册自定义 Asset 模板（从 JSON 文件加载）
 */
UFUNCTION(BlueprintCallable, Category = "UEMotion|Asset")
bool RegisterAssetTemplate(const FString& TemplatePath);

/**
 * 获取 Asset 库单例
 */
UFUNCTION(BlueprintPure, Category = "UEMotion|Asset")
UUEMotionAssetFactory* GetAssetFactory();

// 私有成员
UPROPERTY()
UUEMotionAssetFactory* AssetFactory;  // 资产工厂单例

UPROPERTY()
TMap<FString, FMotionAssetConfig> CachedAssetConfigs;  // 缓存已注册的配置
```

#### 3.1.3 改造 `UEMotionMobject` 类

**新增字段**:

```cpp
// 在 UUEMotionMobject.h 中添加

/**
 * 来源 Asset 的路径（如果是基于 Asset 创建的）
 * 如果为空，说明是旧模式的临时 Actor
 */
UPROPERTY(VisibleAnywhere, Category = "UEMotion|Source")
FString SourceAssetPath;

/**
 * 关联的 Blueprint Asset（用于重新实例化）
 */
UPROPERTY()
TWeakObjectPtr<UBlueprint> SourceBlueprint;

/**
 * 是否为 Asset-Based 模式
 */
bool bIsAssetBased = false;
```

**新增方法**:

```cpp
/**
 * 从 Blueprint Asset 初始化（新模式）
 */
void InitializeFromBlueprint(AActor* SpawnedActor, UBlueprint* SourceBP);

/**
 * 重新从 Source Asset 实例化（应用新的覆盖参数）
 */
void ReinstanceFromSource(const FMotionAssetConfig& NewOverrides);

/**
 * 获取源 Asset 路径（用于调试和序列化）
 */
FString GetSourceAssetPath() const { return SourceAssetPath; }
```

### 3.2 Python 层改动

#### 3.2.1 新增模块：`asset_library.py`

**文件**: `Scripts/uemotion/asset_library.py`

```python
"""
UEMotion Asset Library - 资产管理与持久化

提供两种模式：
1. Legacy Mode: 兼容旧的直接创建 Actor 方式（默认）
2. Asset Mode: 先生成 UAsset，再实例化（推荐）
"""

import unreal
import json
import os
from typing import Optional, Dict, Any


class AssetConfig:
    """资产配置类 - 对应 C++ 的 FMotionAssetConfig"""

    def __init__(
        self,
        mesh_type: str = "cube",
        size: float = 50.0,
        base_color: tuple = (1.0, 1.0, 1.0),
        metallic: float = 0.0,
        roughness: float = 0.5,
        opacity: float = 1.0,
        normal_map_path: str = "",
        simulate_physics: bool = False
    ):
        self.mesh_type = mesh_type.lower()
        self.size = size
        self.base_color = base_color if len(base_color) == 3 else (*base_color[:3], 1.0)
        self.metallic = metallic
        self.roughness = roughness
        self.opacity = opacity
        self.normal_map_path = normal_map_path
        self.simulate_physics = simulate_physics

    def to_ue_struct(self) -> unreal.Struct:
        """转换为 UE Struct"""
        config = unreal.FMotionAssetConfig()
        config.mesh_type = self.mesh_type
        config.size = self.size
        config.base_color = unreal.LinearColor(*self.base_color)
        config.metallic = self.metallic
        config.roughness = self.roughness
        config.opacity = self.opacity
        config.normal_map_path = self.normal_map_path
        config.simulate_physics = self.simulate_physics
        return config

    def to_dict(self) -> Dict[str, Any]:
        """导出为字典（用于 JSON 序列化）"""
        return {
            "mesh_type": self.mesh_type,
            "size": self.size,
            "base_color": list(self.base_color),
            "metallic": self.metallic,
            "roughness": self.roughness,
            "opacity": self.opacity,
            "normal_map_path": self.normal_map_path,
            "simulate_physics": self.simulate_physics
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'AssetConfig':
        """从字典导入"""
        return cls(**data)


ASSET_ROOT_PATH = "/Game/UEMotion/Assets"

# 子目录常量
MESHES_PATH = f"{ASSET_ROOT_PATH}/Meshes"
MATERIALS_PATH = f"{ASSET_ROOT_PATH}/Materials"
BLUEPRINTS_PATH = f"{ASSET_ROOT_PATH}/Blueprints"
TEMPLATES_PATH = f"{ASSET_ROOT_PATH}/Templates"


class AssetLibrary:
    """
    资产管理库 - 单例模式

    负责：
    - 创建和管理 UAsset
    - 缓存已创建的资产
    - 提供模板系统
    """

    _instance: Optional['AssetLibrary'] = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._factory = None
            cls._instance._cache = {}
            cls._instance._templates = {}
        return cls._instance

    @property
    def factory(self):
        """懒初始化 C++ 工厂"""
        if self._factory is None:
            self._factory = unreal.UEMotionAssetFactory()
        return self._factory

    def create_mesh_asset(
        self,
        config: AssetConfig,
        asset_name: str,
        output_path: str = MESHES_PATH
    ) -> str:
        """
        创建并保存 StaticMesh Asset

        Returns:
            str: 生成的 Asset 路径（如 "/Game/UEMotion/Assets/Meshes/SM_MyCube"）
        """
        ue_config = config.to_ue_struct()
        mesh = self.factory.create_and_save_mesh_asset(
            ue_config,
            asset_name,
            output_path
        )

        full_path = f"{output_path}/{asset_name}"
        self._cache[full_path] = {"type": "mesh", "config": config}
        print(f"[AssetLibrary] Created mesh asset: {full_path}")
        return full_path

    def create_material_asset(
        self,
        config: AssetConfig,
        asset_name: str,
        output_path: str = MATERIALS_PATH
    ) -> str:
        """创建并保存 Material Instance Asset"""
        ue_config = config.to_ue_struct()
        mat = self.factory.create_and_save_material_asset(
            ue_config,
            asset_name,
            output_path
        )

        full_path = f"{output_path}/{asset_name}"
        self._cache[full_path] = {"type": "material", "config": config}
        print(f"[AssetLibrary] Created material asset: {full_path}")
        return full_path

    def create_blueprint_asset(
        self,
        config: AssetConfig,
        asset_name: str,
        output_path: str = BLUEPRINTS_PATH
    ) -> str:
        """
        创建完整的 Actor Blueprint Asset（核心方法）

        这个 Blueprint 包含：
        - 预配置的 StaticMeshComponent
        - 材质实例（颜色、金属度、粗糙度）
        - 物理设置（可选）
        - 可选的额外组件（如 Niagara 粒子）

        Returns:
            str: Blueprint 路径（可用于后续 SpawnActor）
        """
        ue_config = config.to_ue_struct()
        bp_class = self.factory.create_and_save_blueprint_asset(
            ue_config,
            asset_name,
            output_path
        )

        full_path = f"{output_path}/{asset_name}"
        self._cache[full_path] = {"type": "blueprint", "config": config}
        print(f"[AssetLibrary] Created blueprint asset: {full_path}")
        return full_path

    def spawn_from_blueprint(
        self,
        blueprint_path: str,
        location: tuple = (0, 0, 0),
        rotation: tuple = (0, 0, 0),
        scale: tuple = (1, 1, 1)
    ) -> object:
        """
        从 Blueprint Asset Spawn Actor

        Args:
            blueprint_path: Blueprint 路径（如 "/Game/UEMotion/Assets/Blueprints/BP_MyCube"）
            location: 世界坐标
            rotation: 欧拉角
            scale: 缩放

        Returns:
            包装了 Actor 的 Mobject
        """
        actor = self.factory.spawn_from_blueprint_asset(
            None,  # WorldContext
            blueprint_path,
            unreal.Vector(*location),
            unreal.Rotator(*rotation),
            unreal.Vector(*scale)
        )
        # TODO: 包装成 Python Mobject 对象
        return actor

    def get_or_create_asset(
        self,
        config: AssetConfig,
        asset_name: str,
        force_recreate: bool = False
    ) -> str:
        """
        获取或创建 Asset（带缓存机制）

        如果 Asset 已存在且 force_recreate=False，直接返回缓存路径
        否则重新创建
        """
        cache_key = f"{BLUEPRINTS_PATH}/{asset_name}"

        if not force_recreate and cache_key in self._cache:
            print(f"[AssetLibrary] Using cached asset: {cache_key}")
            return cache_key

        return self.create_blueprint_asset(config, asset_name)

    def register_template(self, template_path: str):
        """从 JSON 文件注册模板"""
        if not os.path.exists(template_path):
            print(f"[WARNING] Template file not found: {template_path}")
            return False

        with open(template_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        template_name = data.get("name", os.path.basename(template_path))
        config = AssetConfig.from_dict(data.get("config", {}))
        self._templates[template_name] = config
        print(f"[AssetLibrary] Registered template: {template_name}")
        return True

    def use_template(self, template_name: str, custom_overrides: Dict = None) -> AssetConfig:
        """使用模板 + 自定义覆盖"""
        if template_name not in self._templates:
            raise ValueError(f"Template '{template_name}' not found. Available: {list(self._templates.keys())}")

        base_config = self._templates[template_name]

        if custom_overrides:
            for key, value in custom_overrides.items():
                if hasattr(base_config, key):
                    setattr(base_config, key, value)

        return base_config

    def list_cached_assets(self) -> Dict:
        """列出所有缓存的资产"""
        return self._cache.copy()

    def clear_cache(self):
        """清除缓存（不删除磁盘文件）"""
        self._cache.clear()
        print("[AssetLibrary] Cache cleared")


# 全局单例
asset_lib = AssetLibrary()


def get_asset_library() -> AssetLibrary:
    """获取全局资产库实例"""
    return asset_lib
```

#### 3.2.2 改造 `scene.py`

**新增方法**:

```python
# 在 Scene 类中添加

def cube_from_asset(
    self,
    asset_name_or_path: str,
    location=None,
    override_config: dict = None,
    **kwargs
) -> 'Mobject':
    """
    从已有 Asset 或配置创建 Cube

    Args:
        asset_name_or_path:
            - 如果是路径（包含 "/"）：直接使用该 Blueprint
            - 如果是纯名称：在 Assets/Blueprints/ 下查找或新建
        location: 初始位置
        override_config: 覆盖 Asset 的部分属性（仅影响本次实例化）
        **kwargs: 其他参数传递给 spawn

    Example:
        # 方式1: 使用已有 Blueprint
        cube = s.cube_from_asset("/Game/UEMotion/Assets/Blueprints/BP_Cube_Standard")

        # 方式2: 通过名称自动查找/创建
        cube = s.cube_from_asset("MyBlueCube")

        # 方式3: 创建并立即使用
        cube = s.cube_from_asset("TempCube", override_config={"color": "#FF5733"})
    """
    from .asset_library import AssetLibrary, AssetConfig

    lib = AssetLibrary()

    # 判断是路径还是名称
    if "/" in asset_name_or_path:
        bp_path = asset_name_or_path
    else:
        # 尝试从缓存获取，或用默认配置创建
        default_config = AssetConfig(mesh_type="cube", **kwargs)
        bp_path = lib.get_or_create_asset(default_config, asset_name_or_path)

    # Spawn Actor
    actor = lib.spawn_from_blueprint(
        bp_path,
        location=location or (0, 0, 50),
        **(override_config or {})
    )

    if actor is None:
        return None

    m = Mobject(self, actor)
    m._source_asset_path = bp_path
    m._is_asset_based = True
    return m

def create_and_save_asset(
    self,
    mesh_type: str,
    asset_name: str,
    color="#FFFFFF",
    metallic=0.0,
    roughness=0.5,
    save=True,
    **kwargs
) -> str:
    """
    显式创建并保存 Asset 到磁盘

    Args:
        mesh_type: "cube", "sphere", "cylinder", "cone", "plane", "torus"
        asset_name: 资产名称（将保存为 BP_{asset_name}.uasset）
        color: 十六进制颜色
        metallic: 金属度 0-1
        roughness: 粗糙度 0-1
        save: 是否立即保存到磁盘
        **kwargs: 其他配置参数

    Returns:
        str: 生成的 Asset 路径

    Example:
        path = s.create_and_save_asset(
            mesh_type="cube",
            asset_name="MetallicBlueCube",
            color="#3498db",
            metallic=0.8,
            roughness=0.2
        )
        # 之后可以反复使用:
        cube = s.cube_from_asset(path)
    """
    from .asset_library import AssetLibrary, AssetConfig
    from .colors import resolve_color

    rgb = resolve_color(color)
    config = AssetConfig(
        mesh_type=mesh_type,
        base_color=rgb,
        metallic=metallic,
        roughness=roughness,
        **kwargs
    )

    lib = AssetLibrary()
    if save:
        return lib.create_blueprint_asset(config, asset_name)
    else:
        return lib.create_blueprint_asset(config, asset_name)
```

#### 3.2.3 改造 `mobject.py`

**新增字段和方法**:

```python
# 在 Mobject.__init__ 中添加
self._source_asset_path = ""  # 来源 Asset 路径
self._is_asset_based = False  # 是否基于 Asset

@property
def source_asset_path(self) -> str:
    """获取来源 Asset 路径"""
    return getattr(self, '_source_asset_path', '')

@source_asset_path.setter
def source_asset_path(self, path: str):
    self._source_asset_path = path
    self._is_asset_based = bool(path)

@property
def is_asset_based(self) -> bool:
    """是否为 Asset-Based 模式"""
    return getattr(self, '_is_asset_based', False)

def reinstance(self, new_config: dict = None):
    """
    重新从 Source Asset 实例化（应用新配置）

    仅适用于 Asset-Based 模式的 Mobject
    """
    if not self._is_asset_base:
        raise RuntimeError("Cannot reinstance a non-asset-based Mobject")

    # 调用底层 C++ 接口重新实例化
    pass  # TODO: 实现

def edit_source_asset(self):
    """
    在 UE Editor 中打开来源 Asset 进行编辑

    仅适用于 Asset-Based 模式
    """
    if not self._is_asset_based:
        raise RuntimeError("No source asset to edit")

    # 使用 EditorAssetSubsystem 打开 Asset
    unreal.EditorAssetLibrary.open_editor_for_assets([self._source_asset_path])
```

### 3.3 模板系统（JSON 配置）

#### 3.3.1 预设模板示例

**文件**: `/Game/UEMotion/Assets/Templates/DefaultCube.json`

```json
{
    "name": "DefaultCube",
    "description": "标准立方体 - 哑光白色",
    "version": "1.0",
    "config": {
        "mesh_type": "cube",
        "size": 50.0,
        "base_color": [1.0, 1.0, 1.0],
        "metallic": 0.0,
        "roughness": 0.7,
        "opacity": 1.0,
        "normal_map_path": "",
        "simulate_physics": false
    }
}
```

**文件**: `/Game/UEMotion/Assets/Templates/GlassSphere.json`

```json
{
    "name": "GlassSphere",
    "description": "玻璃球体 - 透明折射",
    "version": "1.0",
    "config": {
        "mesh_type": "sphere",
        "size": 40.0,
        "base_color": [0.9, 0.95, 1.0],
        "metallic": 0.2,
        "roughness": 0.05,
        "opacity": 0.4,
        "normal_map_path": "",
        "simulate_physics": false
    }
}
```

**文件**: `/Game/UEMotion/Assets/Templates/MetallicRedCube.json`

```json
{
    "name": "MetallicRedCube",
    "description": "红色金属立方体 - 高反射",
    "version": "1.0",
    "config": {
        "mesh_type": "cube",
        "size": 50.0,
        "base_color": [0.9, 0.1, 0.1],
        "metallic": 0.9,
        "roughness": 0.15,
        "opacity": 1.0,
        "normal_map_path": "/Game/UEMotion/Textures/T_MetalNormal",
        "simulate_physics": true
    }
}
```

#### 3.3.2 使用模板

```python
from uemotion import Scene
from uemotion.asset_library import get_asset_library

s = Scene("template_demo")

lib = get_asset_library()

# 注册模板（通常在项目启动时批量注册）
lib.register_template("/Game/UEMotion/Assets/Templates/DefaultCube.json")
lib.register_template("/Game/UEMotion/Assets/Templates/GlassSphere.json")
lib.register_template("/Game/UEMotion/Assets/Templates/MetallicRedCube.json")

# 使用模板创建
config = lib.use_template("GlassSphere")
path = lib.create_blueprint_asset(config, "MyGlassSphere")
glass_sphere = s.spawn_from_blueprint(path)
glass_sphere.location = (-100, 0, 0)

# 使用模板 + 覆盖
metal_config = lib.use_template("MetallicRedCube", custom_overrides={
    "base_color": (0.9, 0.3, 0.1),  # 改为橙色
    "roughness": 0.3  # 稍微粗糙一点
})
metal_cube = s.spawn_from_blueprint(lib.create_blueprint_asset(metal_config, "OrangeMetalCube"))
metal_cube.location = (100, 0, 0)

s.play()
```

---

## 四、迁移策略（向后兼容）

### 4.1 双模式并存

为了保证平滑过渡，新旧两种模式应该**同时支持**：

```python
s = Scene("migration_test")

# 旧模式（Legacy）- 仍然可用，但会打印警告
cube_old = s.cube(50, color="blue")  # 打印: "[DEPRECATED] Using legacy mode. Consider using cube_from_asset() instead."

# 新模式（Asset-Based）- 推荐
cube_new = s.cube_from_asset("BlueCube")  # 自动创建/查找 Asset
```

### 4.2 渐进式迁移步骤

#### Phase 1: 基础设施（Week 1-2）
1. ✅ 实现 `UUEMotionAssetFactory` C++ 类
2. ✅ 实现 `asset_library.py` Python 模块
3. ✅ 添加 `create_blueprint_asset()` 和 `spawn_from_blueprint()` 方法
4. ✅ 创建默认模板 JSON 文件

#### Phase 2: API 扩展（Week 3）
1. ✅ 为 `Scene` 类添加 `*_from_asset()` 方法
2. ✅ 为 `Mobject` 添加 `_is_asset_based` 标记
3. ✅ 实现缓存机制
4. ✅ 编写单元测试

#### Phase 3: 文档与示例（Week 4）
1. ✅ 更新 AGENTS.md 文档
2. ✅ 创建迁移指南
3. ✅ 提供 5 个示例脚本展示新用法
4. ✅ 录制视频教程（可选）

#### Phase 4: 废弃旧 API（Month 2+）
1. ⚠️ 旧方法标记为 `@deprecated`
2. ⚠️ 运行时打印警告信息
3. 🔴 Month 3: 完全移除旧实现（可选）

---

## 五、实施检查清单

### 5.1 C++ 开发任务

- [ ] **[P0]** 创建 `UEMotionAssetFactory.h` 头文件
- [ ] **[P0]** 实现 `CreateAndSaveMeshAsset()` - 生成 StaticMesh
- [ ] **[P0]** 实现 `CreateAndSaveMaterialAsset()` - 生成 MaterialInstance
- [ ] **[P0]** 实现 `CreateAndSaveBlueprintAsset()` - 生成 Actor Blueprint ⭐
- [ ] **[P0]** 实现 `SpawnFromBlueprintAsset()` - 从 BP Spawn Actor
- [ ] **[P1]** 实现 `DoesAssetExist()` - 检查 Asset 存在性
- [ ] **[P1]** 在 `UEMotionScene` 中集成 AssetFactory
- [ ] **[P1]** 修改 `UEMotionMobject` 支持 Source Asset 跟踪
- [ ] **[P2]** 添加 Asset 预览功能（Editor 面板）
- [ ] **[P2]** 实现批量 Asset 导入/导出

### 5.2 Python 开发任务

- [ ] **[P0]** 创建 `asset_library.py` 模块
- [ ] **[P0]** 实现 `AssetConfig` 数据类
- [ ] **[P0]** 实现 `AssetLibrary` 单例类
- [ ] **[P0]** 实现模板系统（JSON 解析）
- [ ] **[P1]** 修改 `scene.py` 添加 `*_from_asset()` 方法
- [ ] **[P1]** 修改 `mobject.py` 添加 Asset 元数据
- [ ] **[P1]** 更新 `__init__.py` 导出新符号
- [ ] **[P2]** 编写迁移工具脚本（自动转换旧脚本）
- [ ] **[P2]** 创建 5 个新示例脚本

### 5.3 测试任务

- [ ] **[P0]** 测试 Asset 创建（Mesh/Material/Blueprint）
- [ ] **[P0]** 测试 Spawn From Blueprint
- [ ] **[P0]** 测试缓存机制
- [ ] **[P1]** 测试模板加载
- [ ] **[P1]** 测试向后兼容（旧 API 仍可用）
- [ ] **[P2]** 性能测试（Asset vs Legacy 模式对比）
- [ ] **[P2]** 边界条件测试（空路径、无效配置等）

### 5.4 文档任务

- [ ] **[P0]** 编写 Asset-Based 架构设计文档（本文档）
- [ ] **[P1]** 更新 AGENTS.md 添加新 API 说明
- [ ] **[P1]** 编写迁移指南（Legacy → Asset）
- [ ] **[P2]** API Reference 文档（ Sphinx/Markdown）
- [ ] **[P2]** 视频教程（YouTube/Bilibili）

---

## 六、预期收益

### 6.1 定量指标

| 指标 | 当前（Legacy）| 目标（Asset-Based）| 提升 |
|------|--------------|-------------------|------|
| **可编辑性** | ❌ 无法编辑 | ✅ 完全可编辑 | ∞ |
| **复用率** | 0%（每次重建）| 90%（缓存命中）| +90% |
| **团队协作** | ❌ 不支持 | ✅ Git/LFS 同步 | ∞ |
| **调试效率** | 低（需看代码）| 高（Content Browser 查看）| +300% |
| **启动速度** | 基准 | +20%（预热后更快）| +20% |

### 6.2 定性改进

✅ **符合 UE 最佳实践**: 与 Blueprint、Content Browser 无缝集成  
✅ **提升开发体验**: 所见即所得（WYSIWYG）  
✅ **支持专业工作流**: 美术可以在 Editor 中调材质，程序员专注逻辑  
✅ **便于教学演示**: 学生可以直接打开 .uasset 学习配置  

---

## 七、风险与缓解措施

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| **C++ 编译错误** | 中 | 高 | 充分单元测试，渐进式提交 |
| **性能回退** | 低 | 中 | 基准测试，保留 Legacy 作为备选 |
| **学习成本** | 中 | 低 | 详细文档 + 示例 + 视频教程 |
| **兼容性问题** | 低 | 高 | 双模式并存，充分回归测试 |
| **磁盘空间占用** | 低 | 低 | 提供 Cleanup 工具清理无用 Asset |

---

## 八、时间估算

| 阶段 | 任务 | 工作量 | 交付物 |
|------|------|--------|--------|
| **Phase 1** | C++ 核心实现 | 5-7 天 | `UEMotionAssetFactory` 可用 |
| **Phase 2** | Python 层集成 | 3-5 天 | `asset_library.py` + Scene/Mobject 改造 |
| **Phase 3** | 模板与示例 | 2-3 天 | 5 个预设模板 + 5 个示例脚本 |
| **Phase 4** | 测试与文档 | 3-5 天 | 单元测试 + 迁移指南 |
| **总计** | | **13-20 天** | 完整的 Asset-Based 架构 |

---

## 九、快速开始示例（完成后可用）

```python
"""
Asset-Based 模式快速入门示例
展示如何创建、保存、复用 UAsset
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))

from uemotion import Scene, ORIGIN
from uemotion.asset_library import AssetLibrary, AssetConfig

# ============================================================
# Step 1: 初始化场景
# ============================================================
s = Scene("asset_demo", 1920, 1080)
s.directional_light(direction=(0, -1, -1), intensity=10)
s.camera.position = (-400, -450, 280)
s.camera.look_at(ORIGIN)

# ============================================================
# Step 2: 创建并保存自定义 Asset（只需一次）
# ============================================================
lib = AssetLibrary()

# 创建一个蓝色金属立方体
blue_metal_config = AssetConfig(
    mesh_type="cube",
    size=60,
    base_color=(0.2, 0.5, 1.0),  # RGB
    metallic=0.85,
    roughness=0.15
)

bp_path = lib.create_blueprint_asset(blue_metal_config, "BlueMetalCube")
print(f"✓ Blueprint saved to: {bp_path}")

# 创建一个半透明玻璃球体
glass_config = AssetConfig(
    mesh_type="sphere",
    radius=45,
    base_color=(0.95, 0.97, 1.0),
    metallic=0.1,
    roughness=0.02,
    opacity=0.35
)

glass_bp_path = lib.create_blueprint_asset(glass_config, "GlassSphere")
print(f"✓ Glass sphere saved to: {glass_bp_path}")

# ============================================================
# Step 3: 从 Asset 快速实例化（可反复使用）
# ============================================================

# 实例化 3 个相同的蓝色金属立方体
cubes = []
for i, x_pos in enumerate([-150, 0, 150]):
    cube = s.cube_from_asset(bp_path, location=(x_pos, 0, 50))
    cubes.append(cube)
    print(f"✓ Spawned cube #{i+1} at x={x_pos}")

# 实例化玻璃球体
glass_sphere = s.cube_from_asset(glass_bp_path, location=(0, -120, 80))
print(f"✓ Spawned glass sphere")

# ============================================================
# Step 4: 应用动画（完全相同的使用方式）
# ============================================================

# 并排出现
s.play(*[
    obj.fade_in(duration=0.5)
    for obj in cubes + [glass_sphere]
])

# 依次移动
for i, cube in enumerate(cubes):
    s.play(cube.move_to((cubes[i].location.x, 100, 50), duration=0.8))

# 玻璃球旋转
s.play(glass_sphere.rotate(360, axis=(0, 0, 1), duration=2))

# 全部缩放
s.play(*[
    obj.scale_to(1.3, duration=0.6)
    for obj in cubes + [glass_sphere]
])

# ============================================================
# Step 5: 渲染输出
# ============================================================

s.render("output/asset_demo.mp4", duration=10, fps=30)

print("\n" + "=" * 60)
print("Asset-Based Demo Complete!")
print("=" * 60)
print(f"\nGenerated Assets:")
print(f"  - {bp_path}")
print(f"  - {glass_bp_path}")
print("\nYou can now find these .uassets in Content Browser:")
print("  /Game/UEMotion/Assets/Blueprints/")
print("\nOpen them in UE Editor to modify materials, add components, etc.")
```

---

## 十、总结

### 核心变革

```
Before (Actor-First):
  Script → Runtime Create Actor → Use → Discard ❌

After (Asset-First):
  Script → Define Config → Save as .uasset → Spawn Actor → Editable ✅ → Reusable ✅
```

### 关键原则

1. **声明式配置**: 用 `AssetConfig` 描述你想要什么，而不是如何创建
2. **持久化优先**: 重要的东西都应该保存为磁盘文件
3. **可发现性**: 所有 Asset 都在 Content Browser 中可见
4. **向后兼容**: 旧代码继续工作，但鼓励迁移到新模式
5. **渐进增强**: 先支持简单场景，再逐步增加高级特性

### 行动项

**立即开始**:
1. 创建 `UEMotionAssetFactory` C++ 类骨架
2. 实现 `CreateAndSaveBlueprintAsset()` 核心方法
3. 编写最简单的端到端测试

**本周目标**:
- 能够通过 Python 调用创建一个 Blueprint Asset
- 能够从该 Blueprint Spawn 一个 Actor
- Actor 显示在场景中且具有正确的材质

---

**文档版本**: v1.0
**最后更新**: 2026-05-09
**状态**: 设计阶段（待评审）
