import unreal
from .colors import resolve_color, vec
from .vector import MVector, to_mvector
from .constants import (
    UP, DOWN, LEFT, RIGHT,
    TOP, BOTTOM, LEFT_SIDE, RIGHT_SIDE,
    UL, UR, DL, DR, ORIGIN,
    DEFAULT_MOBJECT_TO_EDGE_BUFF,
    DEFAULT_MOBJECT_TO_MOBJECT_BUFF,
    SCALE_FACTOR,
    ue_to_motion, ue_to_motion_vec,
    motion_to_ue, motion_to_ue_vec,
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
        ue_loc = self._ue.get_location()
        return ue_to_motion_vec(ue_loc)

    @location.setter
    def location(self, value):
        v = to_mvector(value)
        ue_v = motion_to_ue_vec(v)
        self._ue.set_location(vec(ue_v.x, ue_v.y, ue_v.z))

    @property
    def raw_location(self):
        return self._ue.get_location()

    @raw_location.setter
    def raw_location(self, value):
        if isinstance(value, (list, tuple)):
            self._ue.set_location(vec(*value))
        else:
            self._ue.set_location(value)

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
            v = to_mvector(value)
            self._ue.set_scale(vec(v.x, v.y, v.z))

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

    def get_bounding_box(self):
        origin, extent = self._ue.get_bounds()
        center = ue_to_motion_vec(origin)
        half = ue_to_motion_vec(extent)
        return {
            'min': center - half,
            'mid': center,
            'max': center + half,
        }

    def get_bounding_box_point(self, direction):
        bb = self.get_bounding_box()
        d = to_mvector(direction)
        return MVector(
            bb['min'].x * (1 - d.x) / 2 + bb['max'].x * (1 + d.x) / 2,
            bb['min'].y * (1 - d.y) / 2 + bb['max'].y * (1 + d.y) / 2,
            bb['min'].z * (1 - d.z) / 2 + bb['max'].z * (1 + d.z) / 2,
        )

    def get_center(self):
        return self.get_bounding_box_point(ORIGIN)

    def get_top(self):
        return self.get_bounding_box_point(UP)

    def get_bottom(self):
        return self.get_bounding_box_point(DOWN)

    def get_left(self):
        return self.get_bounding_box_point(LEFT)

    def get_right(self):
        return self.get_bounding_box_point(RIGHT)

    def get_corner(self, direction):
        return self.get_bounding_box_point(direction)

    @property
    def width(self):
        bb = self.get_bounding_box()
        return bb['max'].x - bb['min'].x

    @property
    def height(self):
        bb = self.get_bounding_box()
        return bb['max'].y - bb['min'].y

    def move_to(self, target, aligned_edge=None, duration=1.0, easing="ease_in_out"):
        if aligned_edge is None:
            aligned_edge = ORIGIN

        if isinstance(target, Mobject):
            target_point = target.get_bounding_box_point(aligned_edge)
        else:
            target_point = to_mvector(target)

        current_point = self.get_bounding_box_point(aligned_edge)
        shift_vec = target_point - current_point
        return self.shift(shift_vec, duration=duration, easing=easing)

    def shift(self, vector, duration=1.0, easing="linear"):
        v = to_mvector(vector)
        current = self.location
        target = current + v
        ue_target = motion_to_ue_vec(target)
        anim = unreal.UEMotionMoveAnimation()
        anim.set_target_mobject(self._ue)
        anim.set_target(vec(ue_target.x, ue_target.y, ue_target.z))
        anim.set_duration(duration)
        anim.set_easing(easing)
        self._scene._pending_animations.append(anim)
        return self

    def to_edge(self, edge, buff=DEFAULT_MOBJECT_TO_EDGE_BUFF):
        d = to_mvector(edge)
        frame_edge_point = self._scene.get_bounding_box_point(d)
        self_anchor = self.get_bounding_box_point(d)
        neg_d = -d
        shift_vec = frame_edge_point - self_anchor + neg_d * buff
        return self.shift(shift_vec)

    def next_to(self, other, direction=RIGHT, buff=DEFAULT_MOBJECT_TO_MOBJECT_BUFF, aligned_edge=None):
        d = to_mvector(direction)
        if isinstance(other, Mobject):
            target_anchor = other.get_bounding_box_point(d)
        else:
            target_anchor = to_mvector(other)

        neg_d = -d
        self_anchor = self.get_bounding_box_point(neg_d)
        shift_vec = target_anchor - self_anchor + d * buff
        return self.shift(shift_vec)

    def align_to(self, other_or_edge, direction=UP):
        d = to_mvector(direction)
        if isinstance(other_or_edge, Mobject):
            target = other_or_edge.get_bounding_box_point(d)
        elif isinstance(other_or_edge, str):
            corner = self._scene.get_corner(other_or_edge)
            target = to_mvector(corner)
        else:
            target = to_mvector(other_or_edge)

        current = self.get_bounding_box_point(d)
        shift_vec = target - current
        mask = MVector(abs(d.x), abs(d.y), abs(d.z))
        shift_vec = MVector(
            shift_vec.x * mask.x if mask.x > 0.01 else 0,
            shift_vec.y * mask.y if mask.y > 0.01 else 0,
            shift_vec.z * mask.z if mask.z > 0.01 else 0,
        )
        return self.shift(shift_vec)

    def rotate(self, angle=360, axis=(0, 0, 1), duration=1.0, easing="ease_in_out"):
        anim = unreal.UEMotionRotateAnimation()
        anim.set_target_mobject(self._ue)
        anim.set_rotation_angle(angle)
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
            v = to_mvector(target_scale)
            anim.set_start_scale(self._ue.get_scale())
            anim.set_end_scale(vec(v.x, v.y, v.z))
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
