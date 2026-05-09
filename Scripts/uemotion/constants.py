from .vector import MVector

FRAME_HEIGHT = 8.0
ASPECT_RATIO = 1.0
FRAME_WIDTH = FRAME_HEIGHT * ASPECT_RATIO

FRAME_Y_RADIUS = FRAME_HEIGHT / 2
FRAME_X_RADIUS = FRAME_WIDTH / 2

ORIGIN = MVector(0.0, 0.0, 0.0)
UP = MVector(0.0, 1.0, 0.0)
DOWN = MVector(0.0, -1.0, 0.0)
LEFT = MVector(-1.0, 0.0, 0.0)
RIGHT = MVector(1.0, 0.0, 0.0)
IN = MVector(0.0, 0.0, -1.0)
OUT = MVector(0.0, 0.0, 1.0)

TOP = MVector(0.0, FRAME_Y_RADIUS, 0.0)
BOTTOM = MVector(0.0, -FRAME_Y_RADIUS, 0.0)
LEFT_SIDE = MVector(-FRAME_X_RADIUS, 0.0, 0.0)
RIGHT_SIDE = MVector(FRAME_X_RADIUS, 0.0, 0.0)

UL = MVector(-FRAME_X_RADIUS, FRAME_Y_RADIUS, 0.0)
UR = MVector(FRAME_X_RADIUS, FRAME_Y_RADIUS, 0.0)
DL = MVector(-FRAME_X_RADIUS, -FRAME_Y_RADIUS, 0.0)
DR = MVector(FRAME_X_RADIUS, -FRAME_Y_RADIUS, 0.0)

SMALL_BUFF = 0.1
MED_SMALL_BUFF = 0.25
DEFAULT_MOBJECT_TO_EDGE_BUFF = 0.5
DEFAULT_MOBJECT_TO_MOBJECT_BUFF = 0.25

DEFAULT_PIXEL_WIDTH = 1080
DEFAULT_PIXEL_HEIGHT = 1080
DEFAULT_FPS = 30
DEFAULT_CAMERA_Z = 10.0
SCALE_FACTOR = 50.0

DEFAULT_DOT_RADIUS = 0.05
DEFAULT_CUBE_SIDE = 0.1
DEFAULT_SPHERE_RADIUS = 0.05
DEFAULT_SQUARE_SIDE = 1.0
DEFAULT_CIRCLE_RADIUS = 1.0

DEFAULT_CAMERA_ASPECT_RATIO = 1.0
DEFAULT_SENSOR_WIDTH = 24.0

PROJECTION_PERSPECTIVE = 0
PROJECTION_ORTHOGRAPHIC = 1


def ue_to_motion(x):
    return x / SCALE_FACTOR


def ue_to_motion_vec(v):
    return MVector(ue_to_motion(v.x), ue_to_motion(v.y), ue_to_motion(v.z))


def motion_to_ue(x):
    return x * SCALE_FACTOR


def motion_to_ue_vec(v):
    from .vector import to_mvector
    mv = to_mvector(v)
    return MVector(motion_to_ue(mv.x), motion_to_ue(mv.y), motion_to_ue(mv.z))


__all__ = [
    "FRAME_HEIGHT",
    "ASPECT_RATIO",
    "FRAME_WIDTH",
    "FRAME_Y_RADIUS",
    "FRAME_X_RADIUS",
    "ORIGIN",
    "UP",
    "DOWN",
    "LEFT",
    "RIGHT",
    "IN",
    "OUT",
    "TOP",
    "BOTTOM",
    "LEFT_SIDE",
    "RIGHT_SIDE",
    "UL",
    "UR",
    "DL",
    "DR",
    "SMALL_BUFF",
    "MED_SMALL_BUFF",
    "DEFAULT_MOBJECT_TO_EDGE_BUFF",
    "DEFAULT_MOBJECT_TO_MOBJECT_BUFF",
    "DEFAULT_PIXEL_WIDTH",
    "DEFAULT_PIXEL_HEIGHT",
    "DEFAULT_FPS",
    "DEFAULT_CAMERA_Z",
    "SCALE_FACTOR",
    "DEFAULT_DOT_RADIUS",
    "DEFAULT_CUBE_SIDE",
    "DEFAULT_SPHERE_RADIUS",
    "DEFAULT_SQUARE_SIDE",
    "DEFAULT_CIRCLE_RADIUS",
    "DEFAULT_CAMERA_ASPECT_RATIO",
    "DEFAULT_SENSOR_WIDTH",
    "PROJECTION_PERSPECTIVE",
    "PROJECTION_ORTHOGRAPHIC",
    "ue_to_motion",
    "ue_to_motion_vec",
    "motion_to_ue",
    "motion_to_ue_vec",
]
