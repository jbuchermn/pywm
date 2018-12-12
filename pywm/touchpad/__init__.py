from .touchpad import Touchpad, find_touchpad
from .sanitize_bogus_ids import SanitizeBogusIds
from .gestures import (  # noqa F401
    Gestures,
    SingleFingerMoveGesture,
    TwoFingerSwipePinchGesture,
    HigherSwipeGesture,
    GestureListener
)


def create_touchpad(gesture_listener):
    event = find_touchpad()
    if event is not None:
        touchpad = Touchpad(event)
        sanitize = SanitizeBogusIds(touchpad)
        gestures = Gestures(sanitize)
        gestures.listener(gesture_listener)
        return touchpad
    else:
        return None
