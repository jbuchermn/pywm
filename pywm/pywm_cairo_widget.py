from __future__ import annotations
from typing import TYPE_CHECKING, Any

import cairo
import numpy as np
from abc import abstractmethod

from .pywm_widget import PyWMWidget

if TYPE_CHECKING:
    from .pywm import PyWM, PyWMOutput, ViewT


class PyWMCairoWidget(PyWMWidget):
    def __init__(self, wm: PyWM[ViewT], output: PyWMOutput, width: int, height: int, *args: Any, **kwargs: Any) -> None:
        super().__init__(wm, output, *args, **kwargs)

        self.width = max(1, width)
        self.height = max(1, height)

    def render(self) -> None:
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32,
                                     self.width, self.height)
        self._render(surface)
        buf = surface.get_data()
        data: np.ndarray = np.ndarray(shape=(self.width, self.height, 4),
                                      dtype=np.uint8,
                                      buffer=buf)
        data = data.reshape((self.width * self.height * 4), order='C')
        self.set_pixels(4*self.width,
                        self.width, self.height, data.tobytes())

    @abstractmethod
    def _render(self, surface: cairo.ImageSurface) -> None:
        pass
