import unreal
from .colors import resolve_color


class Mobject:
    def __init__(self, scene, ue_mobject):
        self._scene = scene
        self._ue = ue_mobject

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
