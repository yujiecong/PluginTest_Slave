import unreal
import os
import math

COLOR_MAP = {
    "red": unreal.LinearColor(1, 0.2, 0.2, 1),
    "green": unreal.LinearColor(0.2, 0.8, 0.3, 1),
    "blue": unreal.LinearColor(0.2, 0.4, 1, 1),
    "yellow": unreal.LinearColor(1, 0.95, 0.2, 1),
    "orange": unreal.LinearColor(1, 0.6, 0.1, 1),
    "purple": unreal.LinearColor(0.6, 0.2, 0.9, 1),
    "cyan": unreal.LinearColor(0.1, 0.9, 0.9, 1),
    "magenta": unreal.LinearColor(0.9, 0.1, 0.9, 1),
    "white": unreal.LinearColor(1, 1, 1, 1),
    "black": unreal.LinearColor(0.05, 0.05, 0.05, 1),
    "gray": unreal.LinearColor(0.5, 0.5, 0.5, 1),
    "grey": unreal.LinearColor(0.5, 0.5, 0.5, 1),
    "dark_gray": unreal.LinearColor(0.2, 0.2, 0.2, 1),
    "pink": unreal.LinearColor(1, 0.4, 0.7, 1),
    "gold": unreal.LinearColor(1, 0.84, 0, 1),
}


def resolve_color(color):
    if isinstance(color, str):
        name = color.lower().strip()
        if name in COLOR_MAP:
            return COLOR_MAP[name]
        if name.startswith("#") and len(name) == 7:
            r = int(name[1:3], 16) / 255.0
            g = int(name[3:5], 16) / 255.0
            b = int(name[5:7], 16) / 255.0
            return unreal.LinearColor(r, g, b, 1)
        if name.startswith("#") and len(name) == 9:
            r = int(name[1:3], 16) / 255.0
            g = int(name[3:5], 16) / 255.0
            b = int(name[5:7], 16) / 255.0
            a = int(name[7:9], 16) / 255.0
            return unreal.LinearColor(r, g, b, a)
        if name.startswith("rgb(") and name.endswith(")"):
            vals = name[4:-1].split(",")
            r, g, b = [float(v.strip()) / 255.0 for v in vals[:3]]
            return unreal.LinearColor(r, g, b, 1)
        return unreal.LinearColor(1, 1, 1, 1)
    if isinstance(color, unreal.LinearColor):
        return color
    if isinstance(color, (list, tuple)) and len(color) >= 3:
        a = color[3] if len(color) > 3 else 1.0
        return unreal.LinearColor(color[0], color[1], color[2], a)
    return unreal.LinearColor(1, 1, 1, 1)


def vec(x=0, y=0, z=0):
    if isinstance(x, (list, tuple)):
        return unreal.Vector(x[0], x[1], x[2]) if len(x) >= 3 else unreal.Vector(x[0], x[1], 0)
    return unreal.Vector(x, y, z)


def rot(pitch=0, yaw=0, roll=0):
    return unreal.Rotator(pitch, yaw, roll)
