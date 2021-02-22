from .touchpad import Touchpad, find_touchpad
from .sanitize_bogus_ids import SanitizeBogusIds
from .gestures import (  # noqa F401
    Gestures,
    SingleFingerMoveGesture,
    TwoFingerSwipePinchGesture,
    HigherSwipeGesture,
    GestureListener,
    LowpassGesture
)


def create_touchpad(device_name, gesture_listener):
    try:
        event = find_touchpad(device_name)
        touchpad = Touchpad(event)
        gestures = Gestures(
            # SanitizeBogusIds(
                touchpad
            # )
        )
        gestures.listener(gesture_listener)
        return touchpad, gestures
    except Exception as e:
        print("Could not open touchpad: %s" % e)
        return None, None
