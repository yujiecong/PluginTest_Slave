import unreal
from .colors import resolve_color


class Animation:
    def __init__(self, scene):
        self._scene = scene
        self._ue_anims = []
        self._sequential = False

    def _add(self, ue_anim):
        self._ue_anims.append(ue_anim)
        return self

    def sequential(self):
        self._sequential = True
        return self

    def parallel(self):
        self._sequential = False
        return self

    @staticmethod
    def create_transform(source_mobject, target_mobject, duration=1.0, easing="linear", resolution=64):
        transform_anim = unreal.UEMotionTransformAnimation()
        transform_anim.set_duration(duration)
        transform_anim.set_easing(easing)
        transform_anim.set_source_mobject(source_mobject._ue)
        transform_anim.set_target_mobject(target_mobject._ue)
        transform_anim.set_morph_resolution(resolution)
        return transform_anim

    def build(self):
        if len(self._ue_anims) == 0:
            return None
        if len(self._ue_anims) == 1:
            return self._ue_anims[0]
        group = unreal.UEMotionGroupAnimation()
        for anim in self._ue_anims:
            group.add_animation(anim)
        group.set_play_mode(self._sequential)
        return group
