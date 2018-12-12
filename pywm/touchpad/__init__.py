from .touchpad import Touchpad, find_touchpad
from .sanitize_bogus_ids import SanitizeBogusIds
from .gestures import (  # noqa F401
    Gestures,
    SingleFingerMoveGesture,
    TwoFingerSwipePinchGesture,
    HigherSwipeGesture
)


def create_touchpad(gesture_listener, gesture_finished_listener=None):
    event = find_touchpad()
    if event is not None:
        touchpad = Touchpad(event)
        sanitize = SanitizeBogusIds(touchpad)
        gestures = Gestures(sanitize)
        gestures.listener(gesture_listener)
        if gesture_finished_listener is not None:
            gestures.finished_listener(gesture_finished_listener)
        return touchpad
    else:
        return None
