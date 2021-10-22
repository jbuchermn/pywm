from __future__ import annotations

from abc import abstractmethod
import logging
import time
from threading import Thread, Lock
from typing import Callable, Optional, Any, Type, TypeVar, Generic

from .touchpad import TouchpadDaemon, GestureListener, Gesture
from .pywm_widget import PyWMWidget
from .pywm_view import PyWMView

from ._pywm import (
    run,
    register
)

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

logger: logging.Logger = logging.getLogger(__name__)


class PyWMDownstreamState:
    def __init__(self, lock_perc: float=0.0) -> None:
        self.lock_perc = lock_perc

    def copy(self) -> PyWMDownstreamState:
        return PyWMDownstreamState(self.lock_perc)

    def get(self, update_cursor: int, open_virtual_output: Optional[str], close_virtual_output: Optional[str], terminate: int) -> tuple[int, float, str, str, bool]:
        return (
            int(update_cursor),
            self.lock_perc,
            open_virtual_output if open_virtual_output is not None else "",
            close_virtual_output if close_virtual_output is not None else "",
            bool(terminate)
        )


T = TypeVar('T')
def callback(func: Callable[..., Optional[T]]) -> Callable[..., Optional[T]]:
    def wrapped_func(*args: list[Any], **kwargs: dict[Any, Any]) -> Optional[T]:
        res = None
        try:
            res = func(*args, **kwargs)
            return res
        except Exception as e:
            logger.exception("---- Error in callback %s (RET %s)", repr(func), res)
            return None
    return wrapped_func

ViewT = TypeVar('ViewT', bound=PyWMView)
WidgetT = TypeVar('WidgetT', bound=PyWMWidget)

class PyWMIdleThread(Thread, Generic[ViewT]):
    def __init__(self, wm: PyWM[ViewT]) -> None:
        super().__init__()
        self.wm = wm

        self._running = True

    def run(self) -> None:
        while self._running:
            t = time.time()
            time.sleep(.5)
            t = time.time() - t

            # Detect if we wake up from suspend
            if t > 1.:
                self.wm._update_idle()
            else:
                self.wm._update_idle(False)

    def stop(self) -> None:
        self._running = False



class PyWM(Generic[ViewT]):
    def __init__(self, view_class: type=PyWMView, **kwargs: Any) -> None:
        logger.debug("PyWM init")

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

        self._views: dict[int, ViewT] = {}
        self._widgets: dict[int, PyWMWidget] = {}

        self._pending_widgets: list[PyWMWidget] = []
        self._pending_destroy_widgets: list[PyWMWidget] = []

        self._last_absolute_x: float = 0.
        self._last_absolute_y: float = 0.

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
        self._pending_open_virtual_output: Optional[str] = None
        self._pending_close_virtual_output: Optional[str] = None
        self._pending_terminate = False

        """
        Consider these read-only
        """
        self.config: dict[str, Any] = kwargs
        self.modifiers = 0

        # TODO
        self.output_scale: float = kwargs['output_scale'] if 'output_scale' in kwargs else 1.0
        self.round_scale: float = kwargs['round_scale'] if 'round_scale' in kwargs else 1.0
        self.width = 0
        self.height = 0

        self._idle_thread: PyWMIdleThread[ViewT] = PyWMIdleThread(self)
        self._idle_last_activity: float = time.time()
        self._idle_last_update_active: float = time.time()
        self._idle_last_update_inactive: float = time.time()

        self._lock = Lock()


    def _update_idle(self, activity: bool=True) -> None:
        # Is called from touchpad thread as well as regular thread
        if self._lock.acquire(blocking=False):
            try:
                inhibited = False
                for _, v in self._views.items():
                    if (up_state := v.up_state) is not None and up_state.is_inhibiting_idle:
                        inhibited = True
                        break

                if activity:
                    self._idle_last_activity = time.time()

                    if time.time() - self._idle_last_update_active > 1:
                        try:
                            self.on_idle(0.0, inhibited)
                        except Exception:
                            logger.exception("on_idle active")
                        self._idle_last_update_active = time.time()
                else:
                    inactive_time = time.time() - self._idle_last_activity
                    if time.time() - self._idle_last_update_inactive > 5 and inactive_time > 5:
                        try:
                            self.on_idle(inactive_time, inhibited)
                        except Exception:
                            logger.exception("on_idle inactive")
                        self._idle_last_update_inactive = time.time()

            finally:
                self._lock.release()



    def _exec_main(self) -> None:
        if self._touchpad_daemon is not None:
            logger.debug("Starting Touchpad daemon")
            self._touchpad_daemon.start()

        logger.debug("Starting idle thread")
        self._idle_thread.start()
        logger.debug("Executing main")
        self.main()

    @callback
    def _ready(self) -> None:
        logger.debug("PyWM ready")
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

        x -= self._last_absolute_x
        y -= self._last_absolute_y
        self._last_absolute_x += x
        self._last_absolute_y += y
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
        logger.debug("PyWM layout change: %dx%d" % (width, height))
        self._update_idle()
        self.width = width
        self.height = height
        self.on_layout_change()
        

    @callback
    def _update_view(self, handle: int, *args): # type: ignore
        try:
            v = self._views[handle]
            try:
                res = v._update(*args)
                return res
            except Exception:
                logger.exception("view._update failed")
                return None
        except Exception:
            view = self._view_class(self, handle)
            self._views[handle] = view

            view._update(*args)
            self._execute_view_main(view)

            res = view._update(*args)
            return res

    @callback
    def _update_widget(self, handle: int, *args): # type: ignore
        try:
            res = self._widgets[handle]._update(*args)
            return res
        except Exception:
            return None


    @callback
    def _update_widget_pixels(self, handle: int, *args): # type: ignore
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
            if (p := self._views[h].parent) is not None and p._handle == handle:
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
    def _update(self) -> tuple[int, float, str, str, bool]:
        if self._damaged:
            self._damaged = False
            self._down_state = self.process()

        res = self._down_state.get(
            self._pending_update_cursor,
            self._pending_open_virtual_output,
            self._pending_close_virtual_output,
            self._pending_terminate
        )

        self._pending_update_cursor = -1
        self._pending_open_virtual_output = None
        self._pending_close_virtual_output = None
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

    def configure_gestures(self, *args: float) -> None:
        self._touchpad_daemon.update_config(*args)

    """
    Public API
    """

    def _encode(self, v: Any) -> Any:
        if isinstance(v, str):
            return v.encode("ascii", "ignore")
        elif isinstance(v, dict):
            return {k:self._encode(v[k]) for k in v}
        elif isinstance(v, list):
            return [self._encode(k) for k in v]
        else:
            return v

    def run(self) -> None:
        logger.debug("PyWM run")
        run(**self._encode(self.config))
        logger.debug("PyWM finished")

    def terminate(self) -> None:
        logger.debug("PyWM terminating")
        if self._touchpad_daemon is not None:
            self._touchpad_daemon.stop()
        self._idle_thread.stop()
        self._pending_terminate = True

    def open_virtual_output(self, name: str) -> None:
        self._pending_open_virtual_output = name

    def close_virtual_output(self, name: str) -> None:
        self._pending_close_virtual_output = name

    def create_widget(self, widget_class: Callable[..., WidgetT], *args: Any, **kwargs: Any) -> WidgetT:
        widget = widget_class(self, *args, **kwargs)
        self._pending_widgets += [widget]
        return widget

    def update_cursor(self, enabled: bool=True) -> None:
        self._pending_update_cursor = 0 if not enabled else 1

    def is_locked(self) -> bool:
        return self._down_state.lock_perc != 0.0

    def round(self, x: float, y: float, w: float, h: float) -> tuple[float, float, float, float]:
        return (
            round(x * self.round_scale) / self.round_scale,
            round(y * self.round_scale) / self.round_scale,
            round((x+w) * self.round_scale) / self.round_scale - round(x * self.round_scale) / self.round_scale,
            round((y+h) * self.round_scale) / self.round_scale - round(y * self.round_scale) / self.round_scale)


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

    def _execute_view_main(self, view: ViewT) -> None:
        pass

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

