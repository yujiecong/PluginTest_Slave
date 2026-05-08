import unreal
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion_utils import frames_to_video

from .mobject import Mobject
from .camera import Camera
from .animation import Animation
from .colors import resolve_color, vec
from .constants import (
    FRAME_WIDTH, FRAME_HEIGHT,
    ORIGIN, UL, UR, DL, DR,
    DEFAULT_PIXEL_WIDTH, DEFAULT_PIXEL_HEIGHT,
    DEFAULT_CAMERA_Z, SCALE_FACTOR,
)


def _color_to_rgb(linear_color):
    if isinstance(linear_color, unreal.LinearColor):
        return (linear_color.r, linear_color.g, linear_color.b)
    if isinstance(linear_color, (list, tuple)):
        return tuple(linear_color[:3])
    return (1.0, 1.0, 1.0)


class Scene:
    def __init__(self, name="default", width=DEFAULT_PIXEL_WIDTH, height=DEFAULT_PIXEL_HEIGHT):
        self._ue = unreal.UEMotionScene()
        self._ue.initialize(name, width, height)
        self._name = name
        self._width = width
        self._height = height
        self._pending_animations = []
        self._camera = Camera(self, self._ue.get_camera())
        self._render_callbacks = []
        self._bind_render_delegate()
        self._frame_width = FRAME_WIDTH
        self._frame_height = FRAME_HEIGHT
        self._scale_factor = SCALE_FACTOR
        self._setup_standard_2d_camera()

    def _bind_render_delegate(self):
        delegate = None
        for attr_name in ['on_render_finished_delegate', 'on_render_finished']:
            if hasattr(self._ue, attr_name):
                delegate = getattr(self._ue, attr_name)
                break
        if delegate is not None:
            delegate.add_callable_unique(self._on_render_finished)
        else:
            unreal.log_warning("UEMotion: OnRenderFinished delegate not found on UEMotionScene. Callbacks will not work until editor is fully restarted.")

    def _on_render_finished(self, scene, success):
        for cb in self._render_callbacks:
            try:
                cb(success)
            except Exception as e:
                unreal.log_warning(f"UEMotion: render callback error: {e}")
        self._render_callbacks.clear()

    @property
    def name(self):
        return self._name

    @property
    def camera(self):
        return self._camera

    @property
    def level_sequence(self):
        return self._ue.get_level_sequence()

    @property
    def scene_world(self):
        return self._ue.get_scene_world()

    @property
    def frame_width(self):
        return self._frame_width

    @property
    def frame_height(self):
        return self._frame_height

    @property
    def center(self):
        return ORIGIN

    def get_corner(self, corner_name):
        corners = {'UL': UL, 'UR': UR, 'DL': DL, 'DR': DR}
        return corners.get(corner_name.upper(), ORIGIN)

    def _setup_standard_2d_camera(self):
        cam_z = DEFAULT_CAMERA_Z * self._scale_factor
        self.camera.position = (0, 0, cam_z)
        self.camera.look_at((0, 0, 0))

    def sphere(self, radius=50, color="white", location=None):
        obj = self._ue.create_sphere(radius)
        if obj is None:
            return None
        m = Mobject(self, obj)
        if color:
            m.color = color
        if location:
            m.location = location
        return m

    def cube(self, size=50, color="white", location=None):
        obj = self._ue.create_cube(size)
        if obj is None:
            return None
        m = Mobject(self, obj)
        if color:
            m.color = color
        if location:
            m.location = location
        return m

    def cylinder(self, radius=50, height=100, color="white", location=None):
        obj = self._ue.create_cylinder(radius, height)
        if obj is None:
            return None
        m = Mobject(self, obj)
        if color:
            m.color = color
        if location:
            m.location = location
        return m

    def cone(self, radius=50, height=100, color="white", location=None):
        obj = self._ue.create_cone(radius, height)
        if obj is None:
            return None
        m = Mobject(self, obj)
        if color:
            m.color = color
        if location:
            m.location = location
        return m

    def plane(self, width=500, height=500, color="white", location=None):
        obj = self._ue.create_plane(width, height)
        if obj is None:
            return None
        m = Mobject(self, obj)
        if color:
            m.color = color
        if location:
            m.location = location
        return m

    def torus(self, outer_radius=80, inner_radius=25, color="white", location=None):
        obj = self._ue.create_torus(outer_radius, inner_radius)
        if obj is None:
            return None
        m = Mobject(self, obj)
        if color:
            m.color = color
        if location:
            m.location = location
        return m

    def from_asset(self, blueprint_path, mesh_type="cube", size=50, color="white",
                   metallic=0.0, roughness=0.5, opacity=1.0, custom_mesh_path="",
                   location=None):
        obj = self._ue.create_mobject_from_asset(
            blueprint_path, mesh_type, size,
            resolve_color(color), metallic, roughness, opacity, custom_mesh_path
        )
        if obj is None:
            return None
        m = Mobject(self, obj)
        if location:
            m.location = location
        return m

    def from_config(self, mesh_type="cube", size=50, color="white",
                    metallic=0.0, roughness=0.5, opacity=1.0,
                    custom_mesh_path="", location=None):
        from .asset_library import AssetConfig
        config = AssetConfig(
            mesh_type=mesh_type, size=size,
            base_color=_color_to_rgb(resolve_color(color)),
            metallic=metallic, roughness=roughness, opacity=opacity,
            custom_mesh_path=custom_mesh_path,
        )
        obj = self._ue.create_mobject_from_config(config.to_ue_config())
        if obj is None:
            return None
        m = Mobject(self, obj)
        if location:
            m.location = location
        return m

    def create_asset(self, mesh_type="cube", asset_name="CustomAsset",
                     size=50, color="white", metallic=0.0, roughness=0.5,
                     opacity=1.0, custom_mesh_path="",
                     output_path="/Game/UEMotion/Assets/Blueprints"):
        from .asset_library import AssetConfig
        config = AssetConfig(
            mesh_type=mesh_type, size=size,
            base_color=_color_to_rgb(resolve_color(color)),
            metallic=metallic, roughness=roughness, opacity=opacity,
            custom_mesh_path=custom_mesh_path,
        )
        return self._ue.create_and_save_blueprint_asset(
            config.to_ue_config(), asset_name, output_path
        )

    @staticmethod
    def asset_exists(asset_path):
        return unreal.UEMotionScene.does_asset_exist(asset_path)

    def directional_light(self, direction=(0, -1, -1), color="white", intensity=10):
        self._ue.add_directional_light(vec(*direction), resolve_color(color), intensity)

    def point_light(self, location=(0, 0, 200), color="white", intensity=5000):
        self._ue.add_point_light(vec(*location), resolve_color(color), intensity)

    def play(self, *animations):
        all_ue_anims = list(self._pending_animations)
        self._pending_animations.clear()

        for item in animations:
            if isinstance(item, Animation):
                built = item.build()
                if built:
                    all_ue_anims.append(built)
            elif isinstance(item, unreal.UEMotionAnimation):
                all_ue_anims.append(item)

        if not all_ue_anims:
            return

        if len(all_ue_anims) == 1:
            self._ue.play(all_ue_anims[0])
        else:
            group = unreal.UEMotionGroupAnimation()
            for anim in all_ue_anims:
                group.add_animation(anim)
            group.set_play_mode(False)
            self._ue.play(group)

    def wait(self, duration=1.0):
        self._ue.wait(duration)

    def render(self, output_path, duration=5.0, fps=30, on_finished=None):
        output_dir = os.path.dirname(output_path)
        if output_dir and not os.path.exists(output_dir):
            os.makedirs(output_dir, exist_ok=True)

        frames_dir = output_path + "_frames"
        if not os.path.exists(frames_dir):
            os.makedirs(frames_dir, exist_ok=True)

        if on_finished:
            self._render_callbacks.append(on_finished)

        self._ue.render_frames(frames_dir, duration, fps)

    def on_render_finished(self, callback):
        self._render_callbacks.append(callback)
        return self

    def render_frame(self, file_path):
        self._ue.render_single_frame(file_path)

    def save_assets(self):
        self._ue.save_assets()

    def set_auto_cleanup(self, cleanup):
        self._ue.set_auto_cleanup(cleanup)

    def clear(self):
        self._ue.clear_scene()

    def destroy(self):
        self._ue.destroy()
