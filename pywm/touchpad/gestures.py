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
_lp_freq = 200.
_lp_inertia = 0.8

_validate_thresholds = {
    'delta_x': .02,
    'delta_y': .02,
    'delta2_s': .0005,
    'scale': .02
}

_validate_centers = {
    'delta_x': 0.,
    'delta_y': 0.,
    'delta2_s': 0.,
    'scale': 1.
}


class GestureListener:
    def __init__(self, on_update, on_terminate):
        self._on_update = on_update
        self._on_terminate = on_terminate

    def update(self, values):
        if self._on_update is not None:
            self._on_update(values)

    def terminate(self):
        if self._on_terminate is not None:
            self._on_terminate()


class LowpassGesture(Thread):
    def __init__(self, gesture):
        super().__init__()

        self.gesture = gesture
        self._listeners = []

        gesture.listener(GestureListener(
            self.on_update,
            self.on_terminate
        ))

        self._lp = {}
        self._values = None
        self._running = True
        self.start()

    def listener(self, l):
        self._listeners += [l]

    def on_update(self, values):
        self._values = values

    def on_terminate(self):
        self._running = False
        self.join()

        for l in self._listeners:
            l.terminate()

    def run(self):
        while self._running:
            if self._values is not None:
                lp_values = {}
                for k in self._values:
                    if k not in self._lp:
                        self._lp[k] = Lowpass(_lp_inertia)
                    lp_values[k] = self._lp[k].next(self._values[k])
                for l in self._listeners:
                    l.update(lp_values)
            time.sleep(1./_lp_freq)


class Gesture:
    def __init__(self, parent):
        self.parent = parent
        self.pending = True

        self._listeners = []

        self._offset = None

    def listener(self, l):
        self._listeners += [l]

    def terminate(self):
        for l in self._listeners:
            l.terminate()

    def update(self, values):
        if self.pending:
            validate = False
            for k in values:
                v = values[k]
                if abs(v - _validate_centers[k]) > _validate_thresholds[k]:
                    validate = True
                    break

            if validate:
                self._offset = {k: values[k] - _validate_centers[k]
                                for k in values}
                self.pending = False

        if not self.pending:
            for l in self._listeners:
                l.update({k: values[k] - self._offset[k] for k in values})

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

    def __str__(self):
        return "SingleFingerMove"


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

    def __str__(self):
        return "TwoFingerSwipePinch"



class HigherSwipeGesture(Gesture):
    def __init__(self, parent, update):
        super().__init__(parent)

        assert(update.n_touches in [3, 4, 5])

        self.n_touches = update.n_touches
        self._update = update
        self._begin_t = update.t

        self._d2s = 0
        self._dx = 0
        self._dy = 0

    def process(self, update):
        """
        "Upgrade" three- to four-finger gesture and so on
        """
        if update.n_touches > self.n_touches:
            if update.t - self._begin_t < 0.2:
                return False

        """
        Gesture finished?
        """
        if update.n_touches == 0:
            return False

        if len(update.touches) == 0:
            return True

        dx = 0
        dy = 0
        d2s = 0
        for i, x, y, z in update.touches:
            try:
                idx = [it[0] for it in self._update.touches].index(i)

                dx += x - [it[1] for it in self._update.touches][idx]
                d2s += (x - [it[1] for it in self._update.touches][idx])**2

                dy += y - [it[2] for it in self._update.touches][idx]
                d2s += (y - [it[2] for it in self._update.touches][idx])**2
            except Exception:
                pass

        self._dx += dx / self.n_touches
        self._dy += dy / self.n_touches
        self._d2s += d2s / self.n_touches

        """
        Update
        """
        self.update({
            'delta_x': self._dx,
            'delta_y': self._dy,
            'delta2_s': self._d2s,
        })


        self._update = update
        return True

    def __str__(self):
        return "HigherSwipe(%d)" % self.n_touches


class Gestures:
    def __init__(self, touchpad):
        self._listeners = []
        self._active_gesture = None

        touchpad.listener(self.on_update)

    def listener(self, l):
        self._listeners += [l]

    def reset(self):
        self._active_gesture = None

    def on_update(self, update):
        was_pending = True
        if self._active_gesture is not None:
            was_pending = self._active_gesture.pending

            if not self._active_gesture.process(update):
                self._active_gesture.terminate()
                self._active_gesture = None

        if self._active_gesture is None:
            if update.n_touches == 1:
                self._active_gesture = SingleFingerMoveGesture(self, update)
            elif update.n_touches == 2:
                self._active_gesture = TwoFingerSwipePinchGesture(self, update)
            elif update.n_touches > 2:
                self._active_gesture = HigherSwipeGesture(self, update)

        if self._active_gesture is not None:
            if was_pending and not self._active_gesture.pending:
                for l in self._listeners:
                    l(self._active_gesture)


if __name__ == '__main__':
    from touchpad import find_touchpad, Touchpad

    def callback(gesture):
        print("New Gesture: %s" % gesture)
        gesture.listener(GestureListener(
            lambda values: print(values),
            lambda: print("---- Terminated ----")
        ))

    event = find_touchpad()
    if event is None:
        print("Could not find touchpad")
    else:
        touchpad = Touchpad(event)
        gestures = Gestures(
            touchpad
        )
        gestures.listener(callback)
        touchpad.run()

