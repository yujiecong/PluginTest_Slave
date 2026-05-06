import unreal
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion_utils import frames_to_video

from .mobject import Mobject
from .camera import Camera
from .animation import Animation
from .colors import resolve_color, vec


class Scene:
    def __init__(self, width=1920, height=1080):
        self._ue = unreal.UEMotionScene()
        self._ue.initialize(width, height)
        self._width = width
        self._height = height
        self._pending_animations = []
        self._camera = Camera(self, self._ue.get_camera())

    @property
    def camera(self):
        return self._camera

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

        max_duration = 0
        for anim in all_ue_anims:
            d = anim.get_duration() if hasattr(anim, 'get_duration') else 1.0
            if d > max_duration:
                max_duration = d
        fps = 30
        total_frames = int(max_duration * fps) + 1
        for _ in range(total_frames):
            self._ue.tick(1.0 / fps)

    def wait(self, duration=1.0):
        anim = unreal.UEMotionWaitAnimation()
        anim.set_duration(duration)
        self._ue.play(anim)
        fps = 30
        total_frames = int(duration * fps) + 1
        for _ in range(total_frames):
            self._ue.tick(1.0 / fps)

    def render(self, output_path, duration=5.0, fps=30):
        output_dir = os.path.dirname(output_path)
        if output_dir and not os.path.exists(output_dir):
            os.makedirs(output_dir, exist_ok=True)

        frames_dir = output_path + "_frames"
        if not os.path.exists(frames_dir):
            os.makedirs(frames_dir, exist_ok=True)

        self._ue.render_frames(frames_dir, duration, fps)

        success = frames_to_video(frames_dir, output_path, fps)
        if success:
            print(f"[UEMotion] Video saved: {output_path}")
        else:
            print(f"[UEMotion] Frames saved to: {frames_dir} (ffmpeg failed)")
        return success

    def render_frame(self, file_path):
        self._ue.render_single_frame(file_path)

    def clear(self):
        self._ue.clear_scene()

    def destroy(self):
        self._ue.destroy()
