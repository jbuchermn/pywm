from __future__ import annotations
from typing import TYPE_CHECKING, TypeVar, Optional, Generic

from abc import abstractmethod

# Python imports are great
if TYPE_CHECKING:
    from .pywm import PyWM, ViewT
    PyWMT = TypeVar('PyWMT', bound=PyWM)
else:
    PyWMT = TypeVar('PyWMT')

PYWM_FORMATS: dict[str, int] = dict()

with open('/usr/include/wayland-server-protocol.h', 'r') as header:
    started = False
    for r in header:
        data = r.replace(" ", "").replace("\t", "").replace(",", "").split("=")
        if data[0].startswith('WL_SHM_FORMAT_'):
            name = data[0][14:]
            code = int(data[1], 0)

            PYWM_FORMATS[name] = code


class PyWMWidgetDownstreamState:
    def __init__(self, z_index: int=0, box: tuple[float, float, float, float]=(0, 0, 0, 0), opacity: float=1., lock_enabled: bool=True) -> None:
        self.z_index = int(z_index)
        self.box = (float(box[0]), float(box[1]), float(box[2]), float(box[3]))
        self.opacity = opacity
        self.lock_enabled=lock_enabled

    def copy(self) -> PyWMWidgetDownstreamState:
        return PyWMWidgetDownstreamState(self.z_index, self.box)

    def get(self, root: PyWM[ViewT]) -> tuple[bool, tuple[float, float, float, float], float, int]:
        return (
            self.lock_enabled,
            root.round(*self.box),
            self.opacity,
            self.z_index
        )

class PyWMWidget(Generic[PyWMT]):
    def __init__(self, wm: PyWMT) -> None:
        self._handle = -1

        self.wm = wm

        self._down_state = PyWMWidgetDownstreamState(0, (0, 0, 0, 0))
        self._damaged = True

        """
        (fmt, stride, width, height, data)
        """
        self._pending_pixels: Optional[tuple[int, int, int, int, bytes]] = None

    def _update(self) -> tuple[bool, tuple[float, float, float, float], float, int]:
        if self._damaged:
            self._damaged = False
            self._down_state = self.process()
        return self._down_state.get(self.wm)

    def _update_pixels(self) -> Optional[tuple[int, int, int, int, bytes]]:
        if self._pending_pixels is not None:
            res = self._pending_pixels
            self._pending_pixels = None
            return res

        return None

    def damage(self) -> None:
        self._damaged = True

    def destroy(self) -> None:
        self.wm.widget_destroy(self)

    def set_pixels(self, fmt: int, stride: int, width: int, height: int, data: bytes) -> None:
        self._pending_pixels = (fmt, stride, width, height, data)

    @abstractmethod
    def process(self) -> PyWMWidgetDownstreamState:
        """
        return next down_state based on whatever state the implementation uses
        """
        pass
