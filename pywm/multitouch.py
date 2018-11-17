import struct
from threading import Thread

_event_format = "llHHi"
_event_size = struct.calcsize(_event_format)
# _event_codes = {
#     0: "abs_x",
#     1: "abs_y",
#     24: "abs_pressure",
#     28: "abs_tool_width",
#     47: "abs_mt_slot",
#     53: "abs_mt_position_x",
#     54: "abs_mt_position_y",
#     57: "abs_mt_tracking_id",
#     58: "abs_mt_pressure",
# }


class Touch:
    def __init__(self, slot, tracking_id, on_update):
        self.slot = slot
        self.id = tracking_id
        self.on_update = on_update
        self.x = -1
        self.y = -1

    def process(self, ev_code, ev_value):
        if ev_code == 53:
            self.x = ev_value
        elif ev_code == 54:
            self.y = ev_value

        self.on_update()

    def __repr__(self):
        return "Touch(id=%d, slot=%d, x=%d, y=%d)" % \
            (self.id, self.slot, self.x, self.y)


class Multitouch(Thread):
    def __init__(self, filename, on_update):
        super().__init__()
        self._fd = open(filename, 'rb')
        self._touches = []
        self._on_update = on_update
        self._state = 0
        self._running = True

    def close(self):
        self.fd.close()

    def next_event(self):
        ev_sec, ev_usec, ev_type, ev_code, ev_value = \
            0, 0, 0, 0, 0

        while ev_type != 3:
            raw = self._fd.read(_event_size)
            ev_sec, ev_usec, ev_type, ev_code, ev_value = \
                struct.unpack(_event_format, raw)

        return ev_code, ev_value

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
        current_slot = -1
        while self._running:
            ev_code, ev_value = self.next_event()
            if ev_code == 47:
                current_slot = ev_value
            elif ev_code == 57:
                if ev_value == -1:
                    self._touches = [t for t in self._touches
                                     if t.slot != current_slot]
                    self.update()

                else:
                    if current_slot != -1:
                        self._touches += [Touch(current_slot, ev_value,
                                                self.update)]
            else:
                touch = [t for t in self._touches if
                         t.slot == current_slot]
                for t in touch:
                    t.process(ev_code, ev_value)

    def stop(self):
        self._running = False


if __name__ == '__main__':
    last = 0

    def callback(touches):
        global last
        if touches is None:
            print("Cancelled")
            last = 0
        else:
            if len(touches) != last:
                print([(t.id, t.slot) for t in touches])
                last = len(touches)

    mt = Multitouch('/dev/input/event9', callback)
    mt.run()

