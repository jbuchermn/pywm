from select import select
import evdev
import time
import logging
from threading import Thread


class Slot:
    def __init__(self, parent, n):
        self.parent = parent
        self.n = n

        self.tracking_id = -1
        self.x = -1
        self.y = -1
        self.z = -1

    def set_tracking_id(self, i):
        self.tracking_id = i
        if self.tracking_id < 0:
            self.x = -1
            self.y = -1
            self.z = -1

    def __str__(self):
        return "%d: (%d, %d, %d)" % (self.tracking_id, self.x, self.y, self.z)


class TouchpadUpdate:
    def __init__(self, n_touches, touches):
        """
        touches is an array of tuples (id, x, y, z)
        """
        self.t = time.time()
        self.n_touches = n_touches

        self.touches = touches


class Touchpad(Thread):
    def __init__(self, filename):
        super().__init__()
        self._device = evdev.InputDevice(filename)
        self._running = True

        self._n_touches = 0

        """
        TODO! Read from capabilities()
        """
        self._n_slots = 2
        self._slots = []

        self._listeners = []

        self.min_x = None
        self.max_x = None
        self.min_y = None
        self.max_y = None
        self.min_z = None
        self.max_z = None
        for code, info in self._device.capabilities()[3]:
            if code == evdev.ecodes.ABS_MT_POSITION_X:
                self.min_x = info.min
                self.max_x = info.max
            elif code == evdev.ecodes.ABS_MT_POSITION_Y:
                self.min_y = info.min
                self.max_y = info.max
            elif code == evdev.ecodes.ABS_MT_PRESSURE:
                self.min_z = info.min
                self.max_z = info.max


    def listener(self, l):
        self._listeners += [l]

    def _get_slot(self, n):
        if n < 0:
            raise Exception("Invalid slot")

        while n >= len(self._slots):
            self._slots += [Slot(self, len(self._slots))]

        return self._slots[n]

    def close(self):
        self._device.close()

    def synchronize(self):
        if len([s for s in self._slots if s.tracking_id >= 0]) == 0:
            self._n_touches = 0

        """
        Skip bogus (too early) sync's
        """
        if self._n_touches >= self._n_slots and \
                len([s for s in self._slots if s.tracking_id >= 0]) \
                < self._n_slots:
            return

        update = TouchpadUpdate(
            self._n_touches,
            [(s.tracking_id,
              (s.x - self.min_x)/(self.max_x - self.min_x),
              (s.y - self.min_y)/(self.max_y - self.min_y),
              (s.z - self.min_z)/(self.max_z - self.min_z) if self.min_z is not None else 1.0
              ) for s in self._slots if s.tracking_id >= 0]
        )

        for l in self._listeners:
            l(update)

    def run(self):
        try:
            slot = 0
            while self._running:
                r, w, x = select([self._device], [], [], 0.1)

                if r:
                    for event in self._device.read():
                        if event.type == evdev.ecodes.EV_SYN:
                            self.synchronize()

                        elif event.type == evdev.ecodes.EV_KEY:
                            if event.value == 1:
                                if event.code == evdev.ecodes.BTN_TOOL_FINGER:
                                    self._n_touches = 1
                                elif event.code == evdev.ecodes.BTN_TOOL_DOUBLETAP:
                                    self._n_touches = 2
                                elif event.code == evdev.ecodes.BTN_TOOL_TRIPLETAP:
                                    self._n_touches = 3
                                elif event.code == evdev.ecodes.BTN_TOOL_QUADTAP:
                                    self._n_touches = 4
                                elif event.code == evdev.ecodes.BTN_TOOL_QUINTTAP:
                                    self._n_touches = 5

                        elif event.type == evdev.ecodes.EV_ABS:
                            if event.code == evdev.ecodes.ABS_MT_SLOT:
                                slot = event.value
                            elif event.code == evdev.ecodes.ABS_MT_TRACKING_ID:
                                self._get_slot(slot).set_tracking_id(event.value)
                            elif event.code == evdev.ecodes.ABS_MT_POSITION_X:
                                self._get_slot(slot).x = event.value
                            elif event.code == evdev.ecodes.ABS_MT_POSITION_Y:
                                self._get_slot(slot).y = event.value
                            elif event.code == evdev.ecodes.ABS_MT_PRESSURE:
                                self._get_slot(slot).z = event.value

        except Exception:
            logging.exception("Touchpad run")

    def stop(self):
        self._running = False


def find_touchpad(device_name="bcm5974"):
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
                        return '/dev/input/' + h

    return None


if __name__ == '__main__':
    event = find_touchpad()

    if event is None:
        print("Could not find touchpad")
    else:
        touchpad = Touchpad(event)
        touchpad.listener(lambda update: print(update))
        touchpad.run()
