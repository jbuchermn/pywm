from ._pywm import (
    view_get_box,
    view_get_size,
    view_get_info,
    view_set_box,
    view_set_size,
    view_get_size_constraints,
    view_focus,
    view_is_floating,
    view_get_parent
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
        self.parent = None
        self.floating = view_is_floating(self._handle)

        parent_handle = view_get_parent(self._handle)
        for v in self.wm.views:
            if v._handle == parent_handle:
                self.parent = v

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

    def get_size(self):
        try:
            return view_get_size(self._handle)
        except Exception:
            return 0, 0

    def get_size_constraints(self):
        try:
            return view_get_size_constraints(self._handle)
        except Exception:
            return -1, -1, -1, -1

    def set_size(self, width, height):
        try:
            view_set_size(self._handle, round(width), round(height))
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
