import unreal
import json
import os
from typing import Optional, Dict, Any, Tuple

ASSET_ROOT = "/Game/UEMotion/Assets"
MESHES_PATH = f"{ASSET_ROOT}/Meshes"
MATERIALS_PATH = f"{ASSET_ROOT}/Materials"
BLUEPRINTS_PATH = f"{ASSET_ROOT}/Blueprints"
TEMPLATES_PATH = f"{ASSET_ROOT}/Templates"


class AssetConfig:
    def __init__(
        self,
        mesh_type: str = "cube",
        size: float = 50.0,
        base_color: Tuple[float, float, float] = (1.0, 1.0, 1.0),
        metallic: float = 0.0,
        roughness: float = 0.5,
        opacity: float = 1.0,
        normal_map_path: str = "",
        emissive_color_hex: str = "",
        simulate_physics: bool = False,
        custom_mesh_path: str = "",
    ):
        self.mesh_type = mesh_type.lower()
        self.size = size
        self.base_color = base_color if len(base_color) == 3 else base_color[:3]
        self.metallic = metallic
        self.roughness = roughness
        self.opacity = opacity
        self.normal_map_path = normal_map_path
        self.emissive_color_hex = emissive_color_hex
        self.simulate_physics = simulate_physics
        self.custom_mesh_path = custom_mesh_path

    def to_ue_config(self):
        config = unreal.FMotionAssetConfig()
        config.mesh_type = self.mesh_type
        config.size = self.size
        config.base_color = unreal.LinearColor(*self.base_color, 1.0)
        config.metallic = self.metallic
        config.roughness = self.roughness
        config.opacity = self.opacity
        config.normal_map_path = self.normal_map_path
        config.emissive_color_hex = self.emissive_color_hex
        config.simulate_physics = self.simulate_physics
        config.custom_mesh_path = self.custom_mesh_path
        return config

    def to_dict(self) -> Dict[str, Any]:
        return {
            "mesh_type": self.mesh_type,
            "size": self.size,
            "base_color": list(self.base_color),
            "metallic": self.metallic,
            "roughness": self.roughness,
            "opacity": self.opacity,
            "normal_map_path": self.normal_map_path,
            "emissive_color_hex": self.emissive_color_hex,
            "simulate_physics": self.simulate_physics,
            "custom_mesh_path": self.custom_mesh_path,
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'AssetConfig':
        return cls(
            mesh_type=data.get("mesh_type", "cube"),
            size=data.get("size", 50.0),
            base_color=tuple(data.get("base_color", (1.0, 1.0, 1.0))),
            metallic=data.get("metallic", 0.0),
            roughness=data.get("roughness", 0.5),
            opacity=data.get("opacity", 1.0),
            normal_map_path=data.get("normal_map_path", ""),
            emissive_color_hex=data.get("emissive_color_hex", ""),
            simulate_physics=data.get("simulate_physics", False),
            custom_mesh_path=data.get("custom_mesh_path", ""),
        )


class AssetLibrary:
    _instance: Optional['AssetLibrary'] = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._cache = {}
            cls._instance._templates = {}
        return cls._instance

    @classmethod
    def reset(cls):
        cls._instance = None

    def create_blueprint_asset(
        self,
        config: AssetConfig,
        asset_name: str,
        output_path: str = BLUEPRINTS_PATH,
    ) -> str:
        ue_config = config.to_ue_config()
        scene = unreal.UEMotionScene()
        full_path = scene.create_and_save_blueprint_asset(ue_config, asset_name, output_path)
        if full_path:
            self._cache[full_path] = {"type": "blueprint", "config": config}
            unreal.log(f"[AssetLibrary] Created blueprint asset: {full_path}")
        return full_path

    def create_material_asset(
        self,
        config: AssetConfig,
        asset_name: str,
        output_path: str = MATERIALS_PATH,
    ) -> str:
        factory = unreal.UEMotionAssetFactory()
        ue_config = config.to_ue_config()
        mic = factory.create_and_save_material_asset(ue_config, asset_name, output_path)
        full_path = f"{output_path}/{asset_name}"
        if mic:
            self._cache[full_path] = {"type": "material", "config": config}
            unreal.log(f"[AssetLibrary] Created material asset: {full_path}")
            return full_path
        return ""

    def does_asset_exist(self, asset_path: str) -> bool:
        return unreal.UEMotionScene.does_asset_exist(asset_path)

    def get_or_create_asset(
        self,
        config: AssetConfig,
        asset_name: str,
        output_path: str = BLUEPRINTS_PATH,
        force_recreate: bool = False,
    ) -> str:
        full_path = f"{output_path}/{asset_name}"
        if not force_recreate and self.does_asset_exist(full_path):
            unreal.log(f"[AssetLibrary] Using cached asset: {full_path}")
            return full_path
        return self.create_blueprint_asset(config, asset_name, output_path)

    def register_template(self, template_path: str) -> bool:
        if not os.path.exists(template_path):
            unreal.log_warning(f"[AssetLibrary] Template file not found: {template_path}")
            return False
        with open(template_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        template_name = data.get("name", os.path.splitext(os.path.basename(template_path))[0])
        config = AssetConfig.from_dict(data.get("config", {}))
        self._templates[template_name] = config
        unreal.log(f"[AssetLibrary] Registered template: {template_name}")
        return True

    def use_template(self, template_name: str, overrides: Dict = None) -> AssetConfig:
        if template_name not in self._templates:
            available = list(self._templates.keys())
            raise ValueError(f"Template '{template_name}' not found. Available: {available}")
        base = self._templates[template_name]
        data = base.to_dict()
        if overrides:
            data.update(overrides)
        return AssetConfig.from_dict(data)

    def list_cached_assets(self) -> Dict:
        return self._cache.copy()

    def clear_cache(self):
        self._cache.clear()
        unreal.log("[AssetLibrary] Cache cleared")


def get_asset_library() -> AssetLibrary:
    return AssetLibrary()
