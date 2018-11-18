import cairo
import numpy as np
from abc import abstractmethod

from .pywm_widget import PyWMWidget, PYWM_FORMATS


class PyWMCairoWidget(PyWMWidget):
    def __init__(self, wm, width, height):
        super().__init__(wm)

        self.width = width
        self.height = height

    def render(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32,
                                     self.width, self.height)
        self._render(surface)
        buf = surface.get_data()
        data = np.ndarray(shape=(self.width, self.height, 4),
                          dtype=np.uint8,
                          buffer=buf)
        data = data.reshape((self.width * self.height * 4), order='C')
        self.set_pixels(PYWM_FORMATS['ARGB8888'],
                        0,
                        self.width, self.height, data.tobytes())

    @abstractmethod
    def _render(self, surface):
        pass
