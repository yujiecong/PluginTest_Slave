import unreal
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion_utils import frames_to_video

from .mobject import Mobject
from .camera import Camera
from .animation import Animation
from .colors import resolve_color, vec
from .vector import MVector, to_mvector
from .constants import (
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_X_RADIUS, FRAME_Y_RADIUS,
    ORIGIN, UL, UR, DL, DR,
    DEFAULT_PIXEL_WIDTH, DEFAULT_PIXEL_HEIGHT,
    DEFAULT_CAMERA_Z, SCALE_FACTOR,
    DEFAULT_CUBE_SIDE, DEFAULT_SPHERE_RADIUS,
    DEFAULT_DOT_RADIUS,
    DEFAULT_CAMERA_ASPECT_RATIO, DEFAULT_SENSOR_WIDTH,
    PROJECTION_PERSPECTIVE, PROJECTION_ORTHOGRAPHIC,
    motion_to_ue, ue_to_motion, motion_to_ue_vec, ue_to_motion_vec,
)


def _color_to_rgb(linear_color):
    if isinstance(linear_color, unreal.LinearColor):
        return (linear_color.r, linear_color.g, linear_color.b)
    if isinstance(linear_color, (list, tuple)):
        return tuple(linear_color[:3])
    return (1.0, 1.0, 1.0)


class Scene:
    def __init__(self, name="default", width=DEFAULT_PIXEL_WIDTH, height=DEFAULT_PIXEL_HEIGHT, mode="2d",
                 aspect_ratio=DEFAULT_CAMERA_ASPECT_RATIO):
        self._ue = unreal.UEMotionScene()
        self._ue.initialize(name, width, height)
        self._name = name
        self._width = width
        self._height = height
        self._mode = mode.lower()
        self._pending_animations = []
        self._camera = Camera(self, self._ue.get_camera())
        self._render_callbacks = []
        self._bind_render_delegate()
        self._frame_width = FRAME_WIDTH
        self._frame_height = FRAME_HEIGHT
        self._scale_factor = SCALE_FACTOR

        if self._mode == "2d":
            self._setup_standard_2d_camera()
        else:
            self._setup_3d_camera()

        self.camera.aspect_ratio = aspect_ratio

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

    @property
    def mode(self):
        return self._mode

    def get_corner(self, corner_name):
        corners = {'UL': UL, 'UR': UR, 'DL': DL, 'DR': DR}
        return corners.get(corner_name.upper(), ORIGIN)

    def get_bounding_box(self):
        return {
            'min': MVector(-FRAME_X_RADIUS, -FRAME_Y_RADIUS, 0),
            'mid': MVector(0, 0, 0),
            'max': MVector(FRAME_X_RADIUS, FRAME_Y_RADIUS, 0),
        }

    def get_bounding_box_point(self, direction):
        bb = self.get_bounding_box()
        d = to_mvector(direction)
        return MVector(
            bb['min'].x * (1 - d.x) / 2 + bb['max'].x * (1 + d.x) / 2,
            bb['min'].y * (1 - d.y) / 2 + bb['max'].y * (1 + d.y) / 2,
            bb['min'].z * (1 - d.z) / 2 + bb['max'].z * (1 + d.z) / 2,
        )

    def _setup_standard_2d_camera(self):
        cam_z = motion_to_ue(DEFAULT_CAMERA_Z)
        self.camera.position = (0, 0, cam_z)
        self.camera.look_at((0, 0, 0))

        ortho_width = self._frame_width * self._scale_factor
        self.camera.set_orthographic(ortho_width=ortho_width)
        self.camera._ue.set_ortho_near_clip_plane(0.0)
        self.camera._ue.set_ortho_far_clip_plane(cam_z * 2.0)
        self._ue.update_camera_key()

    def _setup_3d_camera(self):
        import math
        cam_z = motion_to_ue(DEFAULT_CAMERA_Z)
        self.camera.position = (0, 0, cam_z)
        self.camera.look_at((0, 0, 0))

        desired_half = FRAME_Y_RADIUS * self._scale_factor
        fov = 2.0 * math.degrees(math.atan(desired_half / cam_z))
        self.camera.fov = fov
        self._ue.update_camera_key()

    def dot(self, radius=DEFAULT_DOT_RADIUS, color="white", location=None):
        return self.from_config(mesh_type="sphere", size=radius * SCALE_FACTOR, color=color, location=location)

    def sphere(self, radius=DEFAULT_SPHERE_RADIUS, color="white", location=None):
        return self.from_config(mesh_type="sphere", size=radius * SCALE_FACTOR, color=color, location=location)

    def cube(self, size=DEFAULT_CUBE_SIDE, color="white", location=None):
        return self.from_config(mesh_type="cube", size=size * SCALE_FACTOR, color=color, location=location)

    def cylinder(self, radius=0.5, height=1.0, color="white", location=None):
        return self.from_config(mesh_type="cylinder", size=radius * SCALE_FACTOR, color=color, location=location)

    def cone(self, radius=0.5, height=1.0, color="white", location=None):
        return self.from_config(mesh_type="cone", size=radius * SCALE_FACTOR, color=color, location=location)

    def plane(self, width=1.0, height=1.0, color="white", location=None):
        return self.from_config(mesh_type="plane", size=width * SCALE_FACTOR, color=color, location=location)

    def torus(self, outer_radius=0.5, inner_radius=0.2, color="white", location=None):
        return self.from_config(mesh_type="torus", size=outer_radius * SCALE_FACTOR, color=color, location=location)

    def from_asset(self, blueprint_path, mesh_type="cube", size=DEFAULT_CUBE_SIDE, color="white",
                   metallic=0.0, roughness=0.5, opacity=1.0, custom_mesh_path="",
                   location=None):
        ue_size = size * SCALE_FACTOR
        obj = self._ue.create_mobject_from_asset(
            blueprint_path, mesh_type, ue_size,
            resolve_color(color), metallic, roughness, opacity, custom_mesh_path
        )
        if obj is None:
            return None
        m = Mobject(self, obj)
        if location is not None:
            m.location = location
        return m

    def from_config(self, mesh_type="cube", size=DEFAULT_CUBE_SIDE * SCALE_FACTOR, color="white",
                    metallic=0.0, roughness=0.5, opacity=1.0,
                    custom_mesh_path="", location=None):
        obj = self._ue.create_mobject_from_params(
            mesh_type, size,
            resolve_color(color), metallic, roughness, opacity, custom_mesh_path
        )
        if obj is None:
            return None
        m = Mobject(self, obj)
        if location is not None:
            m.location = location
        return m

    def create_asset(self, mesh_type="cube", asset_name="CustomAsset",
                     size=DEFAULT_CUBE_SIDE, color="white", metallic=0.0, roughness=0.5,
                     opacity=1.0, custom_mesh_path="",
                     output_path="/Game/UEMotion/Assets/Blueprints"):
        ue_size = size * SCALE_FACTOR
        return self._ue.create_and_save_blueprint_asset(
            mesh_type, ue_size, resolve_color(color),
            metallic, roughness, opacity, custom_mesh_path,
            asset_name, output_path
        )

    @staticmethod
    def asset_exists(asset_path):
        return unreal.UEMotionScene.does_asset_exist(asset_path)

    def directional_light(self, direction=(0, -1, -1), color="white", intensity=10):
        d = to_mvector(direction)
        self._ue.add_directional_light(vec(d.x, d.y, d.z), resolve_color(color), intensity)

    def point_light(self, location=(0, 0, 200), color="white", intensity=5000):
        loc = to_mvector(location)
        ue_loc = motion_to_ue_vec(loc)
        self._ue.add_point_light(vec(ue_loc.x, ue_loc.y, ue_loc.z), resolve_color(color), intensity)

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
