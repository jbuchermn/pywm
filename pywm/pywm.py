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

    def get(self, update_cursor: int, open_virtual_output: Optional[str], close_virtual_output: Optional[str], terminate: int, config: Optional[dict[str, Any]]) -> tuple[int, float, str, str, bool, Optional[dict[str, Any]]]:
        return (
            int(update_cursor),
            self.lock_perc,
            open_virtual_output if open_virtual_output is not None else "",
            close_virtual_output if close_virtual_output is not None else "",
            bool(terminate),
            config
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

class PyWMOutput:
    def __init__(self, name: str, scale: float, width: int, height: int, pos: tuple[int, int]):
        self.name = name
        self.scale = scale
        self.width = width
        self.height = height
        self.pos = pos

    def __str__(self) -> str:
        return "Output(%s) with %dx%d, scale %f at %d, %d" % (self.name, self.width, self.height, self.scale, *self.pos)

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, PyWMOutput):
            return False
        return self.name == other.name

class PyWM(Generic[ViewT]):
    def __init__(self, view_class: type=PyWMView, **kwargs: Any) -> None:
        logger.debug("PyWM init")

        register("ready", self._ready)
        register("layout_change", self._layout_change)
        register("motion", self._motion)
        register("button", self._button)
        register("axis", self._axis)
        register("key", self._key)
        register("modifiers", self._modifiers)

        register("update_view", self._update_view)
        register("destroy_view", self._destroy_view)
        register("view_event", self._view_event)
        register("view_resized", self._view_resized)

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

        self._pending_config: Optional[dict[str, Any]] = None

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
        self.layout: list[PyWMOutput] = []
        self.modifiers = 0
        self.cursor_pos: tuple[float, float] = (0, 0)

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
    def _motion(self, time_msec: int, delta_x: float, delta_y: float, abs_x: float, abs_y: float) -> bool:
        self._update_idle()
        if self._touchpad_captured:
            return True

        self.cursor_pos = (abs_x, abs_y)
        return self.on_motion(time_msec, delta_x, delta_y)

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
    def _layout_change(self, outputs: list[tuple[str, float, int, int, int, int]]) -> None:
        self._update_idle()
        self.layout = [PyWMOutput(n, s, w, h, (px, py)) for n, s, w, h, px, py in outputs]
        logger.debug("PyWM layout change:")
        for o in self.layout:
            logger.debug("  %s", str(o))

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
    def _view_resized(self, handle: int, width: int, height: int) -> None:
        try:
            self._views[handle].on_resized(width, height)
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
    def _update(self) -> tuple[int, float, str, str, bool, Optional[dict[str, Any]]]:
        if self._damaged:
            self._damaged = False
            self._down_state = self.process()

        res = self._down_state.get(
            self._pending_update_cursor,
            self._pending_open_virtual_output,
            self._pending_close_virtual_output,
            self._pending_terminate,
            self._encode(self._pending_config)
        )

        self._pending_update_cursor = -1
        self._pending_open_virtual_output = None
        self._pending_close_virtual_output = None
        self._pending_terminate = False
        self._pending_config = None

        return res
    
    def damage(self) -> None:
        self._damaged = True

    def widget_destroy(self, widget: PyWMWidget) -> None:
        self._widgets.pop(widget._handle, None)
        if widget._handle >= 0:
            self._pending_destroy_widgets += [widget]
        else:
            self._pending_widgets = [w for w in self._pending_widgets if id(w) != id(widget)]

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


    def _encode(self, v: Any) -> Any:
        if isinstance(v, str):
            return v.encode("ascii", "ignore")
        elif isinstance(v, dict):
            return {k:self._encode(v[k]) for k in v}
        elif isinstance(v, list):
            return [self._encode(k) for k in v]
        else:
            return v
    """
    Public API
    """

    def run(self) -> None:
        logger.debug("PyWM run")
        run(**self._encode(self.config))
        logger.debug("PyWM finished")

    def reconfigure(self, config: dict[str, Any]) -> None:
        self._pending_config = config
        self.config = config

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

    def create_widget(self, widget_class: Callable[..., WidgetT], output: Optional[PyWMOutput], *args: Any, **kwargs: Any) -> WidgetT:
        widget = widget_class(self, output, *args, **kwargs)
        self._pending_widgets += [widget]
        return widget

    def update_cursor(self, enabled: bool=True) -> None:
        self._pending_update_cursor = 0 if not enabled else 1

    def is_locked(self) -> bool:
        return self._down_state.lock_perc != 0.0

    def _get_round_scale(self, x: float, y: float, w: float, h: float) -> float:
        scale: Optional[float] = None
        for o in self.layout:
            if o.pos[0] + o.width < x or x + w < o.pos[0]:
                continue
            if o.pos[1] + o.height < y or y + h < o.pos[1]:
                continue
            if scale is None or o.scale < scale:
                scale = o.scale
        return scale if scale is not None else 1.


    def round(self, x: float, y: float, w: float, h: float, wh_logical: bool=True) -> tuple[float, float, float, float]:
        # Round positions to 1/scale logical pixels, width and height to logical pixels (if wh_logical)
        # where scale is the smallest hidpi scale intersected by (x, y, w, h)

        scale = self._get_round_scale(x, y, w, h)


        wh_scale = 1 if wh_logical else scale
        cx = x + .5*w
        cy = y + .5*h
        w = round(w * wh_scale) / wh_scale
        h = round(h * wh_scale) / wh_scale

        return (
            round((cx - .5*w) * scale) / scale,
            round((cy - .5*h) * scale) / scale,
            w,
            h)


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

