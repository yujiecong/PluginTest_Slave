import unreal
from .colors import vec
from .constants import PROJECTION_PERSPECTIVE, PROJECTION_ORTHOGRAPHIC


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
        elif hasattr(value, 'x'):
            self._ue.set_position(value.x, value.y, value.z)
        self._scene._ue.update_camera_key()

    @property
    def fov(self):
        return self._ue.get_fov()

    @fov.setter
    def fov(self, value):
        self._ue.set_fov(value)
        self._scene._ue.update_camera_key()

    @property
    def projection_mode(self):
        return self._ue.get_projection_mode()

    @projection_mode.setter
    def projection_mode(self, value):
        if isinstance(value, str):
            mode_map = {
                'perspective': PROJECTION_PERSPECTIVE,
                'ortho': PROJECTION_ORTHOGRAPHIC,
                'orthographic': PROJECTION_ORTHOGRAPHIC,
            }
            value = mode_map.get(value.lower(), PROJECTION_PERSPECTIVE)
        self._ue.set_projection_mode(int(value))
        self._scene._ue.update_camera_key()

    @property
    def ortho_width(self):
        return self._ue.get_ortho_width()

    @ortho_width.setter
    def ortho_width(self, value):
        self._ue.set_ortho_width(float(value))
        self._scene._ue.update_camera_key()

    def look_at(self, target):
        self._ue.look_at(vec(*target) if isinstance(target, (list, tuple)) else target)
        self._scene._ue.update_camera_key()

    def orbit(self, center, radius, angle_deg, height=0):
        self._ue.orbit_around(
            vec(*center) if isinstance(center, (list, tuple)) else center,
            radius, angle_deg, height
        )
        self._scene._ue.update_camera_key()

    def set_orthographic(self, ortho_width=None):
        self.projection_mode = PROJECTION_ORTHOGRAPHIC
        if ortho_width is not None:
            self.ortho_width = ortho_width
        self._scene._ue.update_camera_key()

    def set_perspective(self, fov=90.0):
        self.projection_mode = PROJECTION_PERSPECTIVE
        self.fov = fov
        self._scene._ue.update_camera_key()
