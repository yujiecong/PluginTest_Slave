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
