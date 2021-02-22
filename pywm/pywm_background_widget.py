import numpy as np
from imageio import imread

from .pywm_widget import (
    PyWMWidget,
    PYWM_FORMATS
)


class PyWMBackgroundWidget(PyWMWidget):
    def __init__(self, wm, path):
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
                            0,
                            self.width, self.height,
                            im_alpha.tobytes())
        except Exception as e:
            print("Unable to load background: %s" % e)
