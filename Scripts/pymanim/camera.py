import unreal
from .colors import resolve_color, vec


class Camera:
    def __init__(self, scene, ue_camera):
        self._scene = scene
        self._ue = ue_camera

    @property
    def position(self):
        return self._ue.get_position()

    @position.setter
    def position(self, value):
        if isinstance(value, (list, tuple)):
            self._ue.set_position(value[0], value[1], value[2])
        else:
            self._ue.set_position(value.x, value.y, value.z)

    @property
    def fov(self):
        return self._ue.get_fov()

    @fov.setter
    def fov(self, value):
        self._ue.set_fov(value)

    def look_at(self, target):
        self._ue.look_at(vec(*target) if isinstance(target, (list, tuple)) else target)

    def orbit(self, center, radius, angle_deg, height=0):
        self._ue.orbit_around(
            vec(*center) if isinstance(center, (list, tuple)) else center,
            radius, angle_deg, height
        )
