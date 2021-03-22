import numpy as np
import logging
from imageio import imread # type: ignore

from .pywm_widget import (
    PyWMWidget,
    PYWM_FORMATS
)
from . import pywm

logger = logging.getLogger(__name__)


class PyWMBackgroundWidget(PyWMWidget):
    def __init__(self, wm: pywm.PyWM[pywm.ViewT], path: str):
        """
        transpose == 't': matrix transpose
        transpose == 'f': flip the image
        """
        super().__init__(wm)

        # Prevent division by zero
        self.width = 1
        self.height = 1

        try:
            im = imread(path)
            im_alpha = np.zeros(shape=(im.shape[0], im.shape[1], 4),
                                dtype=np.uint8)
            im_alpha[:, :, 0] = im[:, :, 2]
            im_alpha[:, :, 1] = im[:, :, 1]
            im_alpha[:, :, 2] = im[:, :, 0]
            im_alpha[:, :, 3] = 255

            self.width = im_alpha.shape[1]
            self.height = im_alpha.shape[0]

            im_alpha = im_alpha.reshape((self.width * self.height * 4), order='C')
            self.set_pixels(PYWM_FORMATS['ARGB8888'],
                            4*self.width,
                            self.width, self.height,
                            im_alpha.tobytes())
        except Exception as e:
            logger.warn("Unable to load background: %s", str(e))
