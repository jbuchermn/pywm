from __future__ import annotations
from typing import TYPE_CHECKING, TypeVar, Optional, Generic

from abc import abstractmethod

# Python imports are great
if TYPE_CHECKING:
    from .pywm import PyWM, PyWMOutput, ViewT
    PyWMT = TypeVar('PyWMT', bound=PyWM)
else:
    PyWMT = TypeVar('PyWMT')


class PyWMWidgetDownstreamState:
    def __init__(self, z_index: int=0, box: tuple[float, float, float, float]=(0, 0, 0, 0), mask: tuple[float, float, float, float]=(-1, -1, -1, -1), opacity: float=1., lock_enabled: bool=True) -> None:
        self.z_index = int(z_index)
        self.box = (float(box[0]), float(box[1]), float(box[2]), float(box[3]))
        self.mask = (float(mask[0]), float(mask[1]), float(mask[2]), float(mask[3]))
        self.opacity = opacity
        self.lock_enabled=lock_enabled

    def copy(self) -> PyWMWidgetDownstreamState:
        return PyWMWidgetDownstreamState(self.z_index, self.box, self.mask)

    def get(self, root: PyWM[ViewT], output: Optional[PyWMOutput]) -> tuple[bool, tuple[float, float, float, float], tuple[float, float, float, float], str, float, int]:
        return (
            self.lock_enabled,
            root.round(*self.box),
            self.mask,
            output.name if output is not None else "",
            self.opacity,
            self.z_index
        )

class PyWMWidget(Generic[PyWMT]):
    def __init__(self, wm: PyWMT, output: Optional[PyWMOutput]) -> None:
        self._handle = -1

        self.wm = wm
        self.output = output

        self._down_state = PyWMWidgetDownstreamState(0, (0, 0, 0, 0))
        self._damaged = True

        """
        (stride, width, height, data)
        """
        self._pending_pixels: Optional[tuple[int, int, int, bytes]] = None

    def _update(self) -> tuple[bool, tuple[float, float, float, float], tuple[float, float, float, float], str, float, int]:
        if self._damaged:
            self._damaged = False
            self._down_state = self.process()
        return self._down_state.get(self.wm, self.output)

    def _update_pixels(self) -> Optional[tuple[int, int, int, bytes]]:
        if self._pending_pixels is not None:
            res = self._pending_pixels
            self._pending_pixels = None
            return res

        return None

    def damage(self) -> None:
        self._damaged = True

    def destroy(self) -> None:
        self.wm.widget_destroy(self)

    def set_pixels(self, stride: int, width: int, height: int, data: bytes) -> None:
        self._pending_pixels = (stride, width, height, data)

    @abstractmethod
    def process(self) -> PyWMWidgetDownstreamState:
        """
        return next down_state based on whatever state the implementation uses
        """
        pass
