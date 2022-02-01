from __future__ import annotations
from typing import TYPE_CHECKING, Optional

from .pywm_widget import PyWMWidget

if TYPE_CHECKING:
    from .pywm import PyWM, ViewT, PyWMOutput

class PyWMBlurWidget(PyWMWidget):
    def __init__(self, wm: PyWM[ViewT], output: Optional[PyWMOutput]) -> None:
        super().__init__(wm, output)
        self.set_blur()

    def set_blur(self, radius: int=1, passes: int=2) -> None:
        self.set_composite("blur", [radius, passes], [])
