import evdev
from threading import Thread


class Touch:
    def __init__(self, parent, slot, tracking_id):
        self.parent = parent
        self.slot = slot
        self.id = tracking_id
        self.x = -1
        self.y = -1

    def process(self, ev_code, ev_value):
        if ev_code == 53:
            self.x = (ev_value - self.parent.min_x) / \
                (self.parent.max_x - self.parent.min_x)
        elif ev_code == 54:
            self.y = (ev_value - self.parent.min_y) / \
                (self.parent.max_y - self.parent.min_y)

    def __repr__(self):
        return "Touch(id=%d, slot=%d, x=%d, y=%d)" % \
            (self.id, self.slot, self.x, self.y)


class Touches:
    def __init__(self):
        """
        number may be larger than len(touches), as possibly
        only the position of two touches is reported but also
        the information that in fact three fingers are touching the pad
        """
        self.number = 1
        self.touches = []

    def add(self, touch):
        self.touches += [touch]

    def remove_slot(self, slot):
        self.touches = [t for t in self.touches if t.slot != slot]


class Multitouch(Thread):
    def __init__(self, filename, on_update):
        super().__init__()
        self._device = evdev.InputDevice(filename)
        self._touches = None
        self._on_update = on_update
        self._active = False
        self._running = True

        self.min_x = None
        self.max_x = None
        self.min_y = None
        self.max_y = None
        for code, info in self._device.capabilities()[3]:
            if code == 53:
                self.min_x = info.min
                self.max_x = info.max
            if code == 54:
                self.min_y = info.min
                self.max_y = info.max

    def close(self):
        self.fd.close()

    def update(self):
        if self._touches is None \
                or min([t.x for t in self._touches.touches]) < 0 \
                or min([t.y for t in self._touches.touches]) < 0:
            if self._active:
                self._on_update(None)
                self._active = False
        else:
            self._active = True
            self._on_update(self._touches)

    def run(self):
        current_slot = 0
        for event in self._device.read_loop():
            if not self._running:
                break

            if event.type == evdev.ecodes.SYN_REPORT:
                self.update()

            elif event.type == 1:
                if event.code == evdev.ecodes.BTN_TOOL_DOUBLETAP \
                        and event.value == 1:
                    if self._touches is not None:
                        self._touches.number = 2
                elif event.code == evdev.ecodes.BTN_TOOL_TRIPLETAP \
                        and event.value == 1:
                    if self._touches is not None:
                        self._touches.number = 3
                elif event.code == evdev.ecodes.BTN_TOOL_QUADTAP \
                        and event.value == 1:
                    if self._touches is not None:
                        self._touches.number = 4
                elif event.code == evdev.ecodes.BTN_TOOL_QUINTTAP \
                        and event.value == 1:
                    if self._touches is not None:
                        self._touches.number = 5

            elif event.type == 3:
                if event.code == 47:
                    current_slot = event.value

                elif event.code == 57:
                    if event.value == -1:
                        self._touches.remove_slot(current_slot)
                        if len(self._touches.touches) == 0:
                            self._touches = None

                    else:
                        if current_slot != -1:
                            if self._touches is None:
                                self._touches = Touches()
                            self._touches.add(Touch(self, current_slot,
                                                    event.value))
                else:
                    if self._touches is not None:
                        touch = [t for t in self._touches.touches if
                                 t.slot == current_slot]
                        for t in touch:
                            t.process(event.code, event.value)

    def stop(self):
        self._running = False


def find_multitouch(device_name, callback):
    with open('/proc/bus/input/devices', 'r') as data:
        found = False
        for d in data:
            if d.startswith("N: Name="):
                name = d.split('"')[1]

                if device_name in name:
                    found = True
            elif d.startswith("H: Handlers=") and found:
                handlers = d.split("=")[1].split()
                for h in handlers:
                    if h.startswith("event"):
                        return Multitouch('/dev/input/' + h, callback)

    return None


if __name__ == '__main__':
    last = 0

    def callback(touches):
        global last
        if touches is None:
            print("Cancelled")
            last = 0
        else:
            print("--------- %d ---------" % (touches.number))
            for t in touches.touches:
                print("%d(%d): %f, %f" % (t.id, t.slot, t.x, t.y))

    name = "SynPS/2"
    mt = find_multitouch(name, callback)
    if mt is None:
        print("No multitouch device named %s found" % name)
    else:
        mt.run()

