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

        self.parent.update()

    def __repr__(self):
        return "Touch(id=%d, slot=%d, x=%d, y=%d)" % \
            (self.id, self.slot, self.x, self.y)


class Multitouch(Thread):
    def __init__(self, filename, on_update):
        super().__init__()
        self._device = evdev.InputDevice(filename)
        self._touches = []
        self._on_update = on_update
        self._state = 0
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
        if len(self._touches) < 2 \
                or min([t.x for t in self._touches]) < 0 \
                or min([t.y for t in self._touches]) < 0:
            if self._state != 0:
                self._on_update(None)
                self._state = 0
        else:
            self._state = len(self._touches)
            ts = sorted(self._touches, key=lambda t: t.slot)
            self._on_update(ts)

    def run(self):
        current_slot = 0
        for event in self._device.read_loop():
            if not self._running:
                break
            if not event.type == 3:
                continue

            if event.code == 47:
                current_slot = event.value
            elif event.code == 57:
                if event.value == -1:
                    self._touches = [t for t in self._touches
                                     if t.slot != current_slot]
                    self.update()

                else:
                    if current_slot != -1:
                        self._touches += [Touch(self, current_slot,
                                                event.value)]
            else:
                touch = [t for t in self._touches if
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
            if len(touches) != last:
                print([(t.id, t.slot, t.x, t.y) for t in touches])
                last = len(touches)

    name = "SynPS/2"
    mt = find_multitouch(name, callback)
    if mt is None:
        print("No multitouch device named %s found" % name)
    else:
        mt.run()

