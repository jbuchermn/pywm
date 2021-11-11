from __future__ import annotations
from typing import TYPE_CHECKING

import numpy as np
import logging
import hashlib
from imageio import imread # type: ignore

from .pywm_widget import (
    PyWMWidget,
)

if TYPE_CHECKING:
    from .pywm import PyWM, PyWMOutput, ViewT

logger = logging.getLogger(__name__)

_cache: list[tuple[str, str, np.ndarray]] = []

def _load(path: str) -> np.ndarray:
    global _cache

    cs = hashlib.md5(open(path, 'rb').read()).hexdigest()
    for p, c, a in _cache:
        if p == path and c == cs:
            return a

    im = imread(path)
    im_alpha = np.zeros(shape=(im.shape[0], im.shape[1], 4),
                        dtype=np.uint8)
    im_alpha[:, :, 0] = im[:, :, 2]
    im_alpha[:, :, 1] = im[:, :, 1]
    im_alpha[:, :, 2] = im[:, :, 0]
    im_alpha[:, :, 3] = 255

    _cache = [(p, c, a) for p, c, a in _cache if p != path] + [(path, cs, im_alpha)]
    return im_alpha


class PyWMBackgroundWidget(PyWMWidget):
    def __init__(self, wm: PyWM[ViewT], output: PyWMOutput, path: str):
        """
        transpose == 't': matrix transpose
        transpose == 'f': flip the image
        """
        super().__init__(wm, output)

        # Prevent division by zero
        self.width = 1
        self.height = 1

        try:
            im_alpha = _load(path)
            self.width = im_alpha.shape[1]
            self.height = im_alpha.shape[0]

            im_alpha = im_alpha.reshape((self.width * self.height * 4), order='C')
            self.set_pixels(4*self.width,
                            self.width, self.height,
                            im_alpha.tobytes())
        except Exception as e:
            logger.warn("Unable to load background: %s", str(e))
