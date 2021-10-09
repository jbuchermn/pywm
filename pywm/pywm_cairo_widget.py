from __future__ import annotations
from typing import TYPE_CHECKING

import cairo
import numpy as np
from abc import abstractmethod

from .pywm_widget import PyWMWidget

if TYPE_CHECKING:
    from .pywm import PyWM, ViewT


class PyWMCairoWidget(PyWMWidget):
    def __init__(self, wm: PyWM[ViewT], width: int, height: int) -> None:
        super().__init__(wm)

        self.width = width
        self.height = height

    def render(self) -> None:
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32,
                                     self.width, self.height)
        self._render(surface)
        buf = surface.get_data()
        data = np.ndarray(shape=(self.width, self.height, 4),
                          dtype=np.uint8,
                          buffer=buf)
        data = data.reshape((self.width * self.height * 4), order='C')
        self.set_pixels(4*self.width,
                        self.width, self.height, data.tobytes())

    @abstractmethod
    def _render(self, surface: cairo.ImageSurface) -> None:
        pass
