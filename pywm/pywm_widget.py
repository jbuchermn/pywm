PYWM_FORMATS = dict()

with open('/usr/include/wayland-server-protocol.h', 'r') as header:
    started = False
    for r in header:
        data = r.replace(" ", "").replace("\t", "").replace(",", "").split("=")
        if data[0].startswith('WL_SHM_FORMAT_'):
            name = data[0][14:]
            code = int(data[1], 0)

            PYWM_FORMATS[name] = code


class PyWMWidget:
    def __init__(self, wm):
        self._handle = None

        """
        Consider these readonly
        """
        self.wm = wm

        self.box = (0, 0, 0, 0)
        self.z_index = 0

        self._pending_pixels = None # (fmt, stride, width, height, data)

    def _update(self):
        return (self.box, self.z_index)

    def _update_pixels(self):
        if self._pending_pixels is not None:
            res = self._pending_pixels
            self._pending_pixels = None
            return res

        return None


    def set_box(self, x, y, w, h):
        self.box = (x, y, w, h)

    def set_z_index(self, z_index):
        self.z_index = z_index

    def destroy(self):
        self.wm.widget_destroy(self)

    def set_pixels(self, fmt, stride, width, height, data):
        self._pending_pixels = (fmt, stride, width, height, data)

