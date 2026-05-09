import unreal


class MVector:
    __slots__ = ('x', 'y', 'z')

    def __init__(self, x=0.0, y=0.0, z=0.0):
        if isinstance(x, (list, tuple)):
            self.x = float(x[0]) if len(x) > 0 else 0.0
            self.y = float(x[1]) if len(x) > 1 else 0.0
            self.z = float(x[2]) if len(x) > 2 else 0.0
        elif isinstance(x, MVector):
            self.x = x.x
            self.y = x.y
            self.z = x.z
        elif hasattr(x, 'x') and hasattr(x, 'y'):
            self.x = float(x.x)
            self.y = float(x.y)
            self.z = float(getattr(x, 'z', 0))
        else:
            self.x = float(x)
            self.y = float(y)
            self.z = float(z)

    def __mul__(self, scalar):
        if isinstance(scalar, (int, float)):
            return MVector(self.x * scalar, self.y * scalar, self.z * scalar)
        return NotImplemented

    def __rmul__(self, scalar):
        if isinstance(scalar, (int, float)):
            return MVector(self.x * scalar, self.y * scalar, self.z * scalar)
        return NotImplemented

    def __add__(self, other):
        if isinstance(other, (MVector, list, tuple)):
            o = MVector(other)
            return MVector(self.x + o.x, self.y + o.y, self.z + o.z)
        if isinstance(other, (int, float)):
            return MVector(self.x + other, self.y + other, self.z + other)
        return NotImplemented

    def __radd__(self, other):
        if isinstance(other, (int, float)):
            return MVector(self.x + other, self.y + other, self.z + other)
        return NotImplemented

    def __sub__(self, other):
        if isinstance(other, (MVector, list, tuple)):
            o = MVector(other)
            return MVector(self.x - o.x, self.y - o.y, self.z - o.z)
        if isinstance(other, (int, float)):
            return MVector(self.x - other, self.y - other, self.z - other)
        return NotImplemented

    def __neg__(self):
        return MVector(-self.x, -self.y, -self.z)

    def __truediv__(self, scalar):
        if isinstance(scalar, (int, float)):
            return MVector(self.x / scalar, self.y / scalar, self.z / scalar)
        return NotImplemented

    def __abs__(self):
        return (self.x ** 2 + self.y ** 2 + self.z ** 2) ** 0.5

    def __iter__(self):
        yield self.x
        yield self.y
        yield self.z

    def __getitem__(self, idx):
        if idx == 0:
            return self.x
        if idx == 1:
            return self.y
        if idx == 2:
            return self.z
        raise IndexError(f"MVector index out of range: {idx}")

    def __len__(self):
        return 3

    def __eq__(self, other):
        if isinstance(other, MVector):
            return self.x == other.x and self.y == other.y and self.z == other.z
        if isinstance(other, (list, tuple)) and len(other) == 3:
            return self.x == other[0] and self.y == other[1] and self.z == other[2]
        return NotImplemented

    def __hash__(self):
        return hash((self.x, self.y, self.z))

    def __repr__(self):
        return f"MVector({self.x}, {self.y}, {self.z})"

    def __str__(self):
        return f"({self.x}, {self.y}, {self.z})"

    def to_tuple(self):
        return (self.x, self.y, self.z)

    def to_ue_vector(self):
        return unreal.Vector(self.x, self.y, self.z)

    def normalized(self):
        length = abs(self)
        if length < 1e-10:
            return MVector(0, 0, 0)
        return self / length

    def dot(self, other):
        o = MVector(other)
        return self.x * o.x + self.y * o.y + self.z * o.z

    def cross(self, other):
        o = MVector(other)
        return MVector(
            self.y * o.z - self.z * o.y,
            self.z * o.x - self.x * o.z,
            self.x * o.y - self.y * o.x,
        )

    def component_wise(self, other, op):
        o = MVector(other)
        return MVector(op(self.x, o.x), op(self.y, o.y), op(self.z, o.z))


def to_mvector(value):
    if isinstance(value, MVector):
        return value
    if isinstance(value, (list, tuple)):
        return MVector(*value)
    if hasattr(value, 'x') and hasattr(value, 'y'):
        return MVector(value.x, value.y, getattr(value, 'z', 0))
    if isinstance(value, (int, float)):
        return MVector(value, 0, 0)
    return MVector(0, 0, 0)
