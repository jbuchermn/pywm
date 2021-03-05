from abc import abstractmethod

PYWM_FORMATS = dict()

with open('/usr/include/wayland-server-protocol.h', 'r') as header:
    started = False
    for r in header:
        data = r.replace(" ", "").replace("\t", "").replace(",", "").split("=")
        if data[0].startswith('WL_SHM_FORMAT_'):
            name = data[0][14:]
            code = int(data[1], 0)

            PYWM_FORMATS[name] = code


class PyWMWidgetDownstreamState:
    def __init__(self, z_index=0, box=(0, 0, 0, 0), opacity=1., lock_enabled=True):
        self.z_index = int(z_index)
        self.box = (float(box[0]), float(box[1]), float(box[2]), float(box[3]))
        self.opacity = opacity
        self.lock_enabled=lock_enabled

    def copy(self):
        return PyWMWidgetDownstreamState(self.z_index, self.box)

    def get(self):
        return (
            self.lock_enabled,
            self.box,
            self.opacity,
            self.z_index
        )

class PyWMWidget:
    def __init__(self, wm):
        self._handle = None

        self.wm = wm

        self._down_state = PyWMWidgetDownstreamState(0, (0, 0, 0, 0))
        self._damaged = True

        """
        (fmt, stride, width, height, data)
        """
        self._pending_pixels = None

    def _update(self):
        if self._damaged:
            self._damaged = False
            self._down_state = self.process()
        return self._down_state.get()

    def _update_pixels(self):
        if self._pending_pixels is not None:
            res = self._pending_pixels
            self._pending_pixels = None
            return res

        return None

    def damage(self):
        self._damaged = True

    def destroy(self):
        self.wm.widget_destroy(self)

    def set_pixels(self, fmt, stride, width, height, data):
        self._pending_pixels = (fmt, stride, width, height, data)

    @abstractmethod
    def process(self):
        """
        return next down_state based on whatever state the implementation uses
        """
        pass
