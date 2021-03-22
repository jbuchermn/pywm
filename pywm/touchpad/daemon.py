from __future__ import annotations

import logging
import time
import threading
from typing import Callable

from .touch import find_all_touchpads, Touchpad
from .gestures import Gestures, Gesture

logger = logging.getLogger(__name__)

class TouchpadDaemon(threading.Thread):
    def __init__(self, gesture_listener: Callable[[Gesture], None]) -> None:
        super().__init__()
        self.gesture_listener = gesture_listener
        self._touchpads: list[tuple[Touchpad, Gestures]] = []

        self._running = True

    def _start_pad(self, name: str, path: str) -> None:
        touchpad = Touchpad(path)
        gestures = Gestures(touchpad)
        gestures.listener(self.gesture_listener)

        self._touchpads += [
            (touchpad, gestures)
        ]
        touchpad.start()
        logger.info("Started touchpad at %s", self._touchpads[-1][0].path)

    def _stop_pad(self, idx: int) -> None:
        self._touchpads[idx][0].stop()
        logger.info("Stopping touchpad at %s...", self._touchpads[idx][0].path)
        self._touchpads[idx][0].join()
        logger.info("...stopped")
        del self._touchpads[idx]

    def update(self) -> None:
        validated = [False for _ in self._touchpads]
        for name, path in find_all_touchpads():
            try:
                i = [t.path for t, g in self._touchpads].index(path)
                validated[i] = True
            except ValueError:
                logger.info("Found new touchpad: %s at %s", name, path)
                self._start_pad(name, path)

        for i, v in reversed(list(enumerate(validated))):
            if not v:
                logger.info("Touchpad at %s disappeared", self._touchpads[i][0].path)
                self._stop_pad(i)

    def stop(self) -> None:
        self._running = False

    def reset_gestures(self) -> None:
        for _, g in self._touchpads:
            g.reset()

    def update_config(self, *args: float) -> None:
        for _, g in self._touchpads:
            g.update_config(*args)


    def run(self) -> None:
        try:
            while self._running:
                self.update()
                time.sleep(.5)
        finally:
            for i, _ in reversed(list(enumerate(self._touchpads))):
                self._stop_pad(i)

if __name__ == '__main__':
    d = TouchpadDaemon(lambda g: print("New gesture"))
    d.start()
    try:
        while True:
            time.sleep(1)
            print(".")
    except:
        d.stop()
        print("Joinig...")
        d.join()
        print("...joined")

