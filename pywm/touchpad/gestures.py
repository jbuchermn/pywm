from __future__ import annotations

from abc import abstractmethod
from pywm.touchpad.touch import TouchpadUpdate
from threading import Thread
import math
import time
from typing import Callable, Optional

from .lowpass import Lowpass


class GesturesConfig:
    def __init__(self) -> None:
        self.two_finger_min_dist = 0.
        self.lp_freq = 0.
        self.lp_inertia = 0.
        self.validate_threshold = 0.
        self.validate_thresholds: dict[str, float] = {}

        self.validate_centers = {
            'delta_x': 0.,
            'delta_y': 0.,
            'delta2_s': 0.,
            'scale': 1.
        }

        self.update(.1, 200., .8, .02)

    def update(self, two_finger_min_dist: float, lp_freq: float, lp_inertia: float, validate_threshold: float) -> None:
        self.two_finger_min_dist = two_finger_min_dist
        self.lp_freq = lp_freq
        self.lp_inertia = lp_inertia
        self.validate_threshold = validate_threshold

        self.validate_thresholds = {
            'delta_x': self.validate_threshold,
            'delta_y': self.validate_threshold,
            'delta2_s': self.validate_threshold**2,
            'scale': self.validate_threshold
        }



class GestureListener:
    def __init__(self, on_update: Optional[Callable[[dict[str, float]], None]], on_terminate: Optional[Callable[[], None]]):
        self._on_update = on_update
        self._on_terminate = on_terminate

    def update(self, values: dict[str, float]) -> None:
        if self._on_update is not None:
            self._on_update(values)

    def terminate(self) -> None:
        if self._on_terminate is not None:
            self._on_terminate()


class LowpassGesture(Thread):
    def __init__(self, gesture: Gesture):
        super().__init__()

        self.gesture = gesture
        self._listeners: list[GestureListener] = []

        gesture.listener(GestureListener(
            self.on_update,
            self.on_terminate
        ))

        self._lp: dict[str, Lowpass] = {}
        self._values: Optional[dict[str, float]] = None
        self._running = True
        self.start()

    def listener(self, l: GestureListener) -> None:
        self._listeners += [l]

    def on_update(self, values: dict[str, float]) -> None:
        self._values = values

    def on_terminate(self) -> None:
        self._running = False
        self.join()

        for l in self._listeners:
            l.terminate()

    def run(self) -> None:
        while self._running:
            if self._values is not None:
                lp_values = {}
                for k in self._values:
                    if k not in self._lp:
                        self._lp[k] = Lowpass(self.gesture.parent.config.lp_inertia)
                    lp_values[k] = self._lp[k].next(self._values[k])
                for l in self._listeners:
                    l.update(lp_values)
            time.sleep(1./self.gesture.parent.config.lp_freq)


class Gesture:
    def __init__(self, parent: Gestures) -> None:
        self.parent = parent
        self.pending = True

        self._listeners: list[GestureListener] = []

        self._offset: Optional[dict[str, float]] = None

    def listener(self, l: GestureListener) -> None:
        self._listeners += [l]

    def terminate(self) -> None:
        for l in self._listeners:
            l.terminate()

    def update(self, values: dict[str, float]) -> None:
        if self.pending:
            validate = False
            for k in values:
                v = values[k]
                if abs(v - self.parent.config.validate_centers[k]) > self.parent.config.validate_thresholds[k]:
                    validate = True
                    break

            if validate:
                self._offset = {k: values[k] - self.parent.config.validate_centers[k]
                                for k in values}
                self.pending = False

        if not self.pending and self._offset is not None:
            for l in self._listeners:
                l.update({k: values[k] - self._offset[k] for k in values})

    @abstractmethod
    def process(self, update: TouchpadUpdate) -> bool:
        """
        Returns false if the update terminates the gesture
        """
        pass


class SingleFingerMoveGesture(Gesture):
    def __init__(self, parent: Gestures, update: TouchpadUpdate) -> None:
        super().__init__(parent)

        assert(update.n_touches == 1 and len(update.touches) == 1)
        self._initial_x = update.touches[0][1]
        self._initial_y = update.touches[0][2]

    def process(self, update: TouchpadUpdate) -> bool:
        if update.n_touches != 1 or len(update.touches) != 1:
            return False

        self.update({
            'delta_x': update.touches[0][1] - self._initial_x,
            'delta_y': update.touches[0][2] - self._initial_y,
        })
        return True

    def __str__(self) -> str:
        return "SingleFingerMove"


class TwoFingerSwipePinchGesture(Gesture):
    def __init__(self, parent: Gestures, update: TouchpadUpdate):
        super().__init__(parent)

        assert(update.n_touches == 2 and len(update.touches) == 2)
        self._initial_cog_x, \
            self._initial_cog_y, \
            self._initial_dist = self._process(update)

    def _process(self, update: TouchpadUpdate) -> tuple[float, float, float]:
        cog_x = sum([x for _, x, _, _ in update.touches]) / 2.
        cog_y = sum([y for _, _, y, _ in update.touches]) / 2.
        dist = math.sqrt(
            (update.touches[0][1] - update.touches[1][1])**2 +
            (update.touches[0][2] - update.touches[1][2])**2
        )

        return cog_x, cog_y, max(self.parent.config.two_finger_min_dist, dist)

    def process(self, update: TouchpadUpdate) -> bool:
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

    def __str__(self) -> str:
        return "TwoFingerSwipePinch"



class HigherSwipeGesture(Gesture):
    def __init__(self, parent: Gestures, update: TouchpadUpdate) -> None:
        super().__init__(parent)

        assert(update.n_touches in [3, 4, 5])

        self.n_touches = update.n_touches
        self._update = update
        self._begin_t = update.t

        self._d2s = 0.
        self._dx = 0.
        self._dy = 0.

    def process(self, update: TouchpadUpdate) -> bool:
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

        dx = 0.
        dy = 0.
        d2s = 0.
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

    def __str__(self) -> str:
        return "HigherSwipe(%d)" % self.n_touches


class Gestures:
    def __init__(self, touchpad: Touchpad) -> None:
        self._listeners: list[Callable[[Gesture], None]] = []
        self._active_gesture: Optional[Gesture] = None

        self.config = GesturesConfig()

        touchpad.listener(self.on_update)

    def listener(self, l: Callable[[Gesture], None]) -> None:
        self._listeners += [l]

    def reset(self) -> None:
        self._active_gesture = None

    def update_config(self, two_finger_min_dist: float, lp_freq: float, lp_inertia: float, validate_threshold: float) -> None:
        self.config.update(two_finger_min_dist, lp_freq, lp_inertia, validate_threshold)

    def on_update(self, update: TouchpadUpdate) -> None:
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
    from touchpad import find_all_touchpads, Touchpad # type: ignore

    def callback(gesture: Gesture) -> None:
        print("New Gesture: %s" % gesture)
        gesture.listener(GestureListener(
            lambda values: print(values),
            lambda: print("---- Terminated ----")
        ))

    event = list(find_all_touchpads())[0][1]
    touchpad = Touchpad(event)
    gestures = Gestures(
        touchpad
    )
    gestures.listener(callback)
    touchpad.run()

