import unreal
from .colors import resolve_color
from .constants import (
    UP, DOWN, LEFT, RIGHT,
    TOP, BOTTOM, LEFT_SIDE, RIGHT_SIDE,
    UL, UR, DL, DR,
    DEFAULT_MOBJECT_TO_EDGE_BUFF,
    DEFAULT_MOBJECT_TO_MOBJECT_BUFF,
)


class Mobject:
    def __init__(self, scene, ue_mobject):
        self._scene = scene
        self._ue = ue_mobject
        self._source_asset_path = ""
        self._is_asset_based = False

    @property
    def source_asset_path(self):
        if hasattr(self._ue, 'get_source_asset_path'):
            return self._ue.get_source_asset_path()
        return self._source_asset_path

    @source_asset_path.setter
    def source_asset_path(self, path):
        self._source_asset_path = path
        self._is_asset_based = bool(path)

    @property
    def is_asset_based(self):
        if hasattr(self._ue, 'is_asset_based'):
            return self._ue.is_asset_based()
        return self._is_asset_based

    @property
    def location(self):
        return self._ue.get_location()

    @location.setter
    def location(self, value):
        from .colors import vec
        self._ue.set_location(vec(*value) if isinstance(value, (list, tuple)) else value)

    @property
    def color(self):
        return self._ue.get_color()

    @color.setter
    def color(self, value):
        self._ue.set_color(resolve_color(value))

    @property
    def scale(self):
        return self._ue.get_scale()

    @scale.setter
    def scale(self, value):
        if isinstance(value, (int, float)):
            self._ue.set_scale(unreal.Vector(value, value, value))
        else:
            from .colors import vec
            self._ue.set_scale(vec(*value) if isinstance(value, (list, tuple)) else value)

    @property
    def rotation(self):
        return self._ue.get_rotation()

    @rotation.setter
    def rotation(self, value):
        from .colors import rot
        if isinstance(value, (list, tuple)):
            self._ue.set_rotation(rot(*value))
        elif isinstance(value, (int, float)):
            self._ue.set_rotation(rot(0, value, 0))
        else:
            self._ue.set_rotation(value)

    @property
    def visible(self):
        return self._ue.get_visibility()

    @visible.setter
    def visible(self, value):
        self._ue.set_visibility(bool(value))

    def move_to(self, target, duration=1.0, easing="ease_in_out"):
        anim = unreal.UEMotionMoveAnimation()
        anim.set_target_mobject(self._ue)
        from .colors import vec
        anim.set_target(vec(*target) if isinstance(target, (list, tuple)) else target)
        anim.set_duration(duration)
        anim.set_easing(easing)
        self._scene._pending_animations.append(anim)
        return self

    def shift(self, vector, duration=1.0, easing="linear"):
        current = self.location
        target = (
            current.x + vector[0],
            current.y + vector[1],
            current.z + vector[2] if len(vector) > 2 else current.z
        )
        return self.move_to(target, duration, easing)

    def to_edge(self, edge, buff=DEFAULT_MOBJECT_TO_EDGE_BUFF):
        edges = {
            'UP': TOP,
            'DOWN': BOTTOM,
            'LEFT': LEFT_SIDE,
            'RIGHT': RIGHT_SIDE,
            'UL': UL,
            'UR': UR,
            'DL': DL,
            'DR': DR,
        }
        target = edges.get(edge.upper(), (0, 0, 0))
        if edge.upper() in ['UP', 'DOWN']:
            target = (self.location.x, target[1], 0)
        elif edge.upper() in ['LEFT', 'RIGHT']:
            target = (target[0], self.location.y, 0)
        if edge.upper() in ['UP', 'RIGHT', 'UR']:
            target = (target[0] - buff, target[1] - buff, 0) if len(target) > 2 else (target[0] - buff, target[1] - buff)
        elif edge.lower() in ['down', 'left', 'dl']:
            target = (target[0] + buff, target[1] + buff, 0) if len(target) > 2 else (target[0] + buff, target[1] + buff)
        return self.move_to(target)

    def next_to(self, other, direction=RIGHT, buff=DEFAULT_MOBJECT_TO_MOBJECT_BUFF):
        other_pos = other.location
        dir_vec = {
            'UP': UP, 'DOWN': DOWN,
            'LEFT': LEFT, 'RIGHT': RIGHT
        }.get(direction.upper(), RIGHT)
        target = (
            other_pos.x + dir_vec[0] * buff,
            other_pos.y + dir_vec[1] * buff,
            0
        )
        return self.move_to(target)

    def align_to(self, other_or_edge, direction=UP):
        pass

    def rotate(self, angle=360, axis=(0, 0, 1), duration=1.0, easing="ease_in_out"):
        anim = unreal.UEMotionRotateAnimation()
        anim.set_target_mobject(self._ue)
        anim.set_rotation_angle(angle)
        from .colors import vec
        anim.set_axis(vec(*axis) if isinstance(axis, (list, tuple)) else axis)
        anim.set_duration(duration)
        anim.set_easing(easing)
        self._scene._pending_animations.append(anim)
        return self

    def scale_to(self, target_scale, duration=1.0, easing="ease_in_out"):
        anim = unreal.UEMotionScaleAnimation()
        anim.set_target_mobject(self._ue)
        if isinstance(target_scale, (int, float)):
            anim.set_start_scale(self._ue.get_scale())
            anim.set_end_scale(unreal.Vector(target_scale, target_scale, target_scale))
        else:
            from .colors import vec
            anim.set_start_scale(self._ue.get_scale())
            anim.set_end_scale(vec(*target_scale) if isinstance(target_scale, (list, tuple)) else target_scale)
        anim.set_duration(duration)
        anim.set_easing(easing)
        self._scene._pending_animations.append(anim)
        return self

    def fade_in(self, duration=1.0, easing="ease_in_out"):
        anim = unreal.UEMotionFadeAnimation()
        anim.set_target_mobject(self._ue)
        anim.set_fade_out(False)
        anim.set_duration(duration)
        anim.set_easing(easing)
        self._ue.set_visibility(False)
        self._scene._pending_animations.append(anim)
        return self

    def fade_out(self, duration=1.0, easing="ease_in_out"):
        anim = unreal.UEMotionFadeAnimation()
        anim.set_target_mobject(self._ue)
        anim.set_fade_out(True)
        anim.set_duration(duration)
        anim.set_easing(easing)
        self._scene._pending_animations.append(anim)
        return self

    def change_color(self, target_color, duration=1.0, easing="ease_in_out"):
        anim = unreal.UEMotionColorAnimation()
        anim.set_target_mobject(self._ue)
        anim.set_start_color(self._ue.get_color())
        anim.set_end_color(resolve_color(target_color))
        anim.set_duration(duration)
        anim.set_easing(easing)
        self._scene._pending_animations.append(anim)
        return self

    def destroy(self):
        self._ue.destroy()
