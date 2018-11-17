from ._pywm import (
    view_get_box,
    view_get_dimensions,
    view_get_info,
    view_set_box,
    view_set_dimensions,
    view_focus
)


class PyWMView:
    def __init__(self, wm, handle):
        self._handle = handle

        """
        Consider these readonly
        """
        self.wm = wm
        self.box = view_get_box(self._handle)
        self.focused = False

    def focus(self):
        try:
            view_focus(self._handle)
        except Exception:
            pass

    def set_box(self, x, y, w, h):
        try:
            view_set_box(self._handle, x, y, w, h)
        except Exception:
            pass
        self.box = (x, y, w, h)

    def get_info(self):
        try:
            return view_get_info(self._handle)
        except Exception:
            return "Destroyed", "", "", False

    def get_dimensions(self):
        try:
            return view_get_dimensions(self._handle)
        except Exception:
            return 0, 0

    def set_dimensions(self, width, height):
        try:
            view_set_dimensions(self._handle, round(width), round(height))
        except Exception:
            pass

    """
    Virtual methods
    """

    def main(self):
        pass

    def destroy(self):
        pass

    def on_focus_change(self):
        pass
