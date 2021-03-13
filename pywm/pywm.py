from __future__ import annotations

from abc import abstractmethod
import logging
import time
from threading import Thread, Lock
from typing import Callable, Tuple, Optional


from .pywm_widget import PyWMWidget
from .pywm_view import PyWMView
from .touchpad import TouchpadDaemon, GestureListener, Gesture

from ._pywm import (  # noqa E402
    run,
    register
)


_instance = None

PYWM_MOD_SHIFT = 1
PYWM_MOD_CAPS = 2
PYWM_MOD_CTRL = 4
PYWM_MOD_ALT = 8
PYWM_MOD_MOD2 = 16
PYWM_MOD_MOD3 = 32
PYWM_MOD_LOGO = 64
PYWM_MOD_MOD5 = 128

PYWM_RELEASED = 0
PYWM_PRESSED = 1


class PyWMDownstreamState:
    def __init__(self, lock_perc: float=0.0):
        self.lock_perc = lock_perc

    def copy(self) -> PyWMDownstreamState:
        return PyWMDownstreamState(self.lock_perc)

    def get(self, update_cursor: int, terminate: int) -> Tuple[int, float, bool]:
        return (
            int(update_cursor),
            self.lock_perc,
            bool(terminate)
        )


def callback(func):
    def wrapped_func(*args, **kwargs):
        res = None
        try:
            res = func(*args, **kwargs)
            return res
        except Exception:
            logging.exception("---- Error in callback %s (RET %s)", repr(func), res)
    return wrapped_func


class PyWM:
    def __init__(self, view_class=PyWMView, **kwargs):
        global _instance
        if _instance is not None:
            raise Exception("Can only have one instance!")
        _instance = self

        logging.debug("PyWM init")

        register("ready", self._ready)
        register("layout_change", self._layout_change)
        register("motion", self._motion)
        register("motion_absolute", self._motion_absolute)
        register("button", self._button)
        register("axis", self._axis)
        register("key", self._key)
        register("modifiers", self._modifiers)

        register("update_view", self._update_view)
        register("destroy_view", self._destroy_view)
        register("view_event", self._view_event)

        register("query_new_widget", self._query_new_widget)
        register("update_widget", self._update_widget)
        register("update_widget_pixels", self._update_widget_pixels)
        register("query_destroy_widget", self._query_destroy_widget)

        register("update", self._update)

        self._view_class = view_class

        self._views = {}
        self._widgets = {}

        self._pending_widgets = []
        self._pending_destroy_widgets = []

        self._last_absolute_x = None
        self._last_absolute_y = None

        self._touchpad_daemon = TouchpadDaemon(self._gesture)
        self._touchpad_captured = False

        self._down_state = PyWMDownstreamState()
        self._damaged = False

        """
        -1: Do nothing
        0: Disable cursor
        1: Enable cursor
        """
        self._pending_update_cursor = -1
        self._pending_terminate = False

        """
        Consider these read-only
        """
        self.config = kwargs
        self.width = 0
        self.height = 0
        self.modifiers = 0

        self._idle_last_activity = time.time()
        self._idle_last_update_active = time.time()
        self._idle_last_update_inactive = time.time()

        self._lock = Lock()


    def _update_idle(self, activity: bool=True) -> None:
        # Is called from touchpad thread as well as regular thread
        if self._lock.acquire(blocking=False):
            try:
                def is_inhibited():
                    inhibited = False
                    for _, v in self._views.items():
                        if v.up_state.is_inhibiting_idle:
                            inhibited = True
                            break
                    return inhibited

                if activity:
                    self._idle_last_activity = time.time()

                    if time.time() - self._idle_last_update_active > 1:
                        try:
                            self.on_idle(0.0, is_inhibited())
                        except Exception:
                            logging.exception("on_idle active")
                        self._idle_last_update_active = time.time()
                else:
                    inactive_time = time.time() - self._idle_last_activity
                    if time.time() - self._idle_last_update_inactive > 5 and inactive_time > 5:
                        try:
                            self.on_idle(inactive_time, is_inhibited())
                        except Exception:
                            logging.exception("on_idle inactive")
                        self._idle_last_update_inactive = time.time()

            finally:
                self._lock.release()



    def _exec_main(self) -> None:
        if self._touchpad_daemon is not None:
            logging.debug("Starting Touchpad daemon")
            self._touchpad_daemon.start()
        logging.debug("Executing main")
        self.main()

    @callback
    def _ready(self) -> None:
        logging.debug("PyWM ready")
        Thread(target=self._exec_main).start()

    @callback
    def _motion(self, time_msec: int, delta_x: float, delta_y: float) -> bool:
        self._update_idle()
        if self._touchpad_captured:
            return True

        if self.width > 0:
            delta_x /= self.width
        if self.height > 0:
            delta_y /= self.height
        return self.on_motion(time_msec, delta_x, delta_y)

    @callback
    def _motion_absolute(self, time_msec: int, x: float, y: float) -> bool:
        self._update_idle()
        if self._touchpad_captured:
            return True

        if self._last_absolute_x is not None and self._last_absolute_y is not None:
            x -= self._last_absolute_x
            y -= self._last_absolute_y
            self._last_absolute_x += x
            self._last_absolute_y += y
        else:
            self._last_absolute_x = x
            self._last_absolute_y = y
            x = 0
            y = 0
        return self.on_motion(time_msec, x, y)

    @callback
    def _button(self, time_msec: int, button: int, state: int) -> bool:
        self._update_idle()
        if self._touchpad_captured:
            return True

        return self.on_button(time_msec, button, state)

    @callback
    def _axis(self, time_msec: int, source: int, orientation: int, delta: float, delta_discrete: int) -> bool:
        self._update_idle()
        if self._touchpad_captured:
            return True

        return self.on_axis(time_msec, source, orientation, delta,
                            delta_discrete)

    @callback
    def _key(self, time_msec: int, keycode: int, state: int, keysyms: str) -> bool:
        self._update_idle()
        result = self.on_key(time_msec, keycode, state, keysyms)
        return result

    @callback
    def _modifiers(self, depressed: int, latched: int, locked: int, group: int) -> bool:
        self._update_idle()
        self.modifiers = depressed
        return self.on_modifiers(self.modifiers)

    @callback
    def _layout_change(self, width: int, height: int) -> None:
        self._update_idle()
        self.width = width
        self.height = height
        self.on_layout_change()

    @callback
    def _update_view(self, handle: int, *args
                     ) -> Optional[Tuple[Tuple[float, float, float, float], float, float, int, bool, bool, Tuple[int, int], int, int, int, int, int]]:
        try:
            v = self._views[handle]
            try:
                res = v._update(*args)
                return res
            except Exception:
                logging.exception("view._update failed")
                return None
        except Exception:
            view = self._view_class(self, handle)
            self._views[handle] = view

            view._update(*args)
            self._execute_view_main(view)

            res = view._update(*args)
            return res

    @callback
    def _update_widget(self, handle: int, *args):
        try:
            res = self._widgets[handle]._update(*args)
            return res
        except Exception:
            return None


    @callback
    def _update_widget_pixels(self, handle: int, *args):

       try:
            return self._widgets[handle]._update_pixels(*args)
       except Exception:
            return None


    @callback
    def _destroy_view(self, handle: int) -> None:
        view = None

        try:
            view = self._views[handle]
            self._views.pop(handle, None)
        except Exception:
            pass

        for h in self._views:
            if self._views[h].parent is not None and self._views[h].parent._handle == handle:
                self._views[h].parent = None

        if view is not None:
            view.destroy()

    @callback
    def _view_event(self, handle: int, event: str) -> None:
        try:
            self._views[handle].on_event(event)
        except Exception:
            pass


    @callback
    def _query_new_widget(self, new_handle: int) -> bool:
        if len(self._pending_widgets) > 0:
            widget = self._pending_widgets.pop(0)
            widget._handle = new_handle
            self._widgets[new_handle] = widget
            return True

        return False
    
    @callback
    def _query_destroy_widget(self) -> Optional[int]:
        if len(self._pending_destroy_widgets) > 0:
            return self._pending_destroy_widgets.pop(0)._handle

        return None

    @callback
    def _update(self) -> Tuple[int, float, bool]:
        self._update_idle(False)
        if self._damaged:
            self._damaged = False
            self._down_state = self.process()

        res = self._down_state.get(
            self._pending_update_cursor,
            self._pending_terminate
        )

        self._pending_update_cursor = -1
        self._pending_terminate = False

        return res
    
    def damage(self) -> None:
        self._damaged = True

    def widget_destroy(self, widget: PyWMWidget) -> None:
        self._widgets.pop(widget._handle, None)
        self._pending_destroy_widgets += [widget]

    def _gesture(self, gesture: Gesture) -> None:
        self._update_idle()
        self._touchpad_captured = self.on_gesture(gesture)
        gesture.listener(GestureListener(None, self._gesture_finished))

    def _gesture_finished(self) -> None:
        self._touchpad_captured = False

    def reallow_gesture(self) -> None:
        if self._touchpad_captured:
            return

        if self._touchpad_daemon is not None:
            self._touchpad_daemon.reset_gestures()

    """
    Public API
    """

    def run(self) -> None:
        logging.debug("PyWM run")
        run(**{k:v if not isinstance(v, str) else v.encode("ascii", "ignore") for k, v in self.config.items()})
        logging.debug("PyWM finished")

    def terminate(self) -> None:
        logging.debug("PyWM terminating")
        if self._touchpad_daemon is not None:
            self._touchpad_daemon.stop()
        self._pending_terminate = True

    def create_widget(self, widget_class: Callable, *args, **kwargs) -> PyWMWidget:
        widget = widget_class(self, *args, **kwargs)
        self._pending_widgets += [widget]
        return widget

    def update_cursor(self, enabled: bool=True) -> None:
        self._pending_update_cursor = 0 if not enabled else 1

    def is_locked(self) -> bool:
        return self._down_state.lock_perc != 0.0

    """
    Virtual methods
    """

    @abstractmethod
    def process(self) -> PyWMDownstreamState:
        """
        return next down_state based on whatever state the implementation uses
        """
        pass

    def main(self) -> None:
        pass

    def _execute_view_main(self, view: PyWMView) -> None:
        view.main()

    def on_layout_change(self) -> None:
        pass

    def on_motion(self, time_msec: int, delta_x: float, delta_y: float) -> bool:
        return False

    def on_button(self, time_msec: int, button: int, state: int) -> bool:
        return False

    def on_axis(self, time_msec: int, source: int, orientation: int, delta: float, delta_discrete: int) -> bool:
        return False

    def on_gesture(self, gesture: Gesture) -> bool:
        return False

    def on_key(self, time_msec: int, keycode: int, state: int, keysyms: str) -> bool:
        """
        keycode: raw xkb keycode, probably useless
        state: PYWM_PRESSED, PYWM_RELEASED
        keysyms: translated keysymbols, like "a", "b", "A", "Left", ...
        """
        return False

    def on_modifiers(self, modifiers: int) -> bool:
        return False

    def on_idle(self, elapsed: float, idle_inhibited: bool) -> None:
        """
        elapsed == 0 means there has been an activity, possibly a wakeup from idle is necessary
        elapsed > 0 describes the amount of seconds which have passed since the last activity, possibly sleep is necessary
        idle_inhibited is True if there is at least one view with is_inhibiting_idle==True
        """
        pass
