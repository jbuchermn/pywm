from abc import abstractmethod
from threading import Thread
import math
import time

"""
Python is great
"""
try:
    from .lowpass import Lowpass
except Exception:
    from lowpass import Lowpass


_two_finger_min_dist = 0.1
_lp_freq = 100.
_lp_inertia = 0.85


class GestureListener:
    def __init__(self, on_update, on_terminate,
                 on_lp_update=None, on_replace=None):
        self._on_update = on_update
        self._on_terminate = on_terminate
        self._on_lp_update = on_lp_update
        self._on_replace = on_replace

    def update(self, values):
        if self._on_update is not None:
            self._on_update(values)

    def lp_update(self, values):
        if self._on_lp_update is not None:
            self._on_lp_update(values)

    def terminate(self, new_gesture=None):
        if self._on_replace is None:
            if self._on_terminate is not None:
                self._on_terminate()
        else:
            if new_gesture is not None:
                self._on_replace(new_gesture)
            else:
                if self._on_terminate is not None:
                    self._on_terminate()


class Gesture(Thread):
    def __init__(self, parent):
        super().__init__()
        self.parent = parent

        self._listeners = []

        self._values = None
        self._lp = {}

        self._running = True
        self.start()

    def listener(self, l):
        self._listeners += [l]

    def run(self):
        while self._running:
            if self._values is not None:
                lp_values = {}
                for k in self._values:
                    if k not in self._lp:
                        self._lp[k] = Lowpass(_lp_inertia)
                    lp_values[k] = self._lp[k].next(self._values[k])
                for l in self._listeners:
                    l.lp_update(lp_values)
            time.sleep(1./_lp_freq)

    def terminate(self, new_gesture):
        self._running = False
        self.join()
        for l in self._listeners:
            l.terminate(new_gesture=new_gesture)

    def update(self, values):
        self._values = values
        for l in self._listeners:
            l.update(values)

    @abstractmethod
    def process(self, update):
        """
        Returns false if the update terminates the gesture
        """
        pass


class SingleFingerMoveGesture(Gesture):
    def __init__(self, parent, update):
        super().__init__(parent)

        assert(update.n_touches == 1 and len(update.touches) == 1)
        self._initial_x = update.touches[0][1]
        self._initial_y = update.touches[0][2]

    def process(self, update):
        if update.n_touches != 1 or len(update.touches) != 1:
            return False

        self.update({
            'delta_x': update.touches[0][1] - self._initial_x,
            'delta_y': update.touches[0][2] - self._initial_y,
        })
        return True


class TwoFingerSwipePinchGesture(Gesture):
    def __init__(self, parent, update):
        super().__init__(parent)

        assert(update.n_touches == 2 and len(update.touches) == 2)
        self._initial_cog_x, \
            self._initial_cog_y, \
            self._initial_dist = self._process(update)

    def _process(self, update):
        cog_x = sum([x for _, x, _, _ in update.touches]) / 2.
        cog_y = sum([y for _, _, y, _ in update.touches]) / 2.
        dist = math.sqrt(
            (update.touches[0][1] - update.touches[1][1])**2 +
            (update.touches[0][2] - update.touches[1][2])**2
        )

        return cog_x, cog_y, max(_two_finger_min_dist, dist)

    def process(self, update):
        if update.n_touches != 2:
            return False

        if len(update.touches) != 2:
            return True

        cog_x, cog_y, dist = self._process(update)
        self.update({
            'delta_x': cog_x - self._initial_cog_x,
            'delta_y': cog_y - self._initial_cog_y,
            'scale': dist/self._initial_dist
        })

        return True


class HigherSwipeGesture(Gesture):
    def __init__(self, parent, update):
        super().__init__(parent)

        assert(update.n_touches in [3, 4, 5])

        self.n_touches = update.n_touches
        self._delta_x = -update.touches[0][1]
        self._delta_y = -update.touches[0][2]
        self._id = update.touches[0][0]
        self._update = update

    def process(self, update):
        if update.n_touches > self.n_touches:
            return False

        if update.n_touches == 0:
            return False

        if len(update.touches) == 0:
            return True

        if self._id not in [d[0] for d in update.touches]:
            intersect = [d for d in self._update.touches if d[0] in
                         [d[0] for d in update.touches]]

            last_touch = [d for d in self._update.touches
                          if d[0] == self._id][0]

            if len(intersect) == 0:
                touch = update.touches[0]
                self._id = touch[0]
                self._delta_x += last_touch[1] - touch[1]
                self._delta_y += last_touch[2] - touch[2]
            else:
                touch = intersect[0]

                self._id = touch[0]
                self._delta_x -= touch[1] - last_touch[1]
                self._delta_y -= touch[2] - last_touch[2]

        touch = [d for d in update.touches if d[0] == self._id][0]
        self.update({
            'delta_x': touch[1] + self._delta_x,
            'delta_y': touch[2] + self._delta_y
        })

        self._update = update
        return True


class Gestures:
    def __init__(self, touchpad):
        self._listeners = []
        self._active_gesture = None

        touchpad.listener(self.on_update)

    def listener(self, l):
        self._listeners += [l]

    def on_update(self, update):
        terminating = False
        if self._active_gesture is not None:
            if not self._active_gesture.process(update):
                terminating = True

        new_gesture = None
        if self._active_gesture is None or terminating:
            if update.n_touches == 1:
                new_gesture = SingleFingerMoveGesture(self, update)
            elif update.n_touches == 2:
                new_gesture = TwoFingerSwipePinchGesture(self, update)
            elif update.n_touches > 2:
                new_gesture = HigherSwipeGesture(self, update)

        if terminating:
            self._active_gesture.terminate(new_gesture)
            self._active_gesture = None

        if new_gesture is not None:
            for l in self._listeners:
                l(new_gesture)
            self._active_gesture = new_gesture


if __name__ == '__main__':
    from touchpad import find_touchpad, Touchpad
    from sanitize_bogus_ids import SanitizeBogusIds

    def callback(gesture):
        print("New Gesture: %s" % gesture.__class__)
        gesture.listener(GestureListener(
            lambda values: print(values),
            lambda: print("---- Terminated ----"),
            lambda values: 0,
            lambda _: print("---- Replaced ----")
        ))

    event = find_touchpad()
    if event is None:
        print("Could not find touchpad")
    else:
        touchpad = Touchpad(event)
        sanitize = SanitizeBogusIds(touchpad)
        gestures = Gestures(sanitize)
        gestures.listener(callback)
        touchpad.run()

