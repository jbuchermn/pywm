import traceback
import time
from threading import Thread

from .pywm_view import PyWMView
from .touchpad import create_touchpad, GestureListener

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


def callback(func):
    def wrapped_func(*args, **kwargs):
        res = None
        try:
            res = func(*args, **kwargs)
            return res
        except Exception as e:
            print("---- Error in callback %s" % repr(func))
            print(e)
            print("Function returned", res)
            traceback.print_exc()
    return wrapped_func


class PyWM:
    def __init__(self, view_class=PyWMView, **kwargs):
        global _instance
        if _instance is not None:
            raise Exception("Can only have one instance!")
        _instance = self

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

        register("query", self._query)

        self._view_class = view_class

        self._views = {}
        self._widgets = {}

        self._pending_widgets = []
        self._pending_destroy_widgets = []

        self._last_absolute_x = None
        self._last_absolute_y = None

        self._touchpad_main = None
        self._touchpad_main = create_touchpad(kwargs["touchpad_device_name"], self._gesture) \
            if "touchpad_device_name" in kwargs else None
        self._touchpad_captured = False

        self._pending_update_cursor = False
        self._pending_terminate = False

        """
        Consider these read-only
        """
        self.config = kwargs
        self.width = 0
        self.height = 0
        self.modifiers = 0

    def _exec_main(self):
        if self._touchpad_main is not None:
            print("Python: Starting pytouchpad")
            self._touchpad_main.start()
        self.main()

    @callback
    def _ready(self):
        Thread(target=self._exec_main).start()

    @callback
    def _motion(self, time_msec, delta_x, delta_y):
        if self._touchpad_captured:
            return True

        if self.width > 0:
            delta_x /= self.width
        if self.height > 0:
            delta_y /= self.height
        return self.on_motion(time_msec, delta_x, delta_y)

    @callback
    def _motion_absolute(self, time_msec, x, y):
        if self._touchpad_captured:
            return True

        if self._last_absolute_x is not None:
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
    def _button(self, time_msec, button, state):
        if self._touchpad_captured:
            return True

        return self.on_button(time_msec, button, state)

    @callback
    def _axis(self, time_msec, source, orientation, delta, delta_discrete):
        if self._touchpad_captured:
            return True

        return self.on_axis(time_msec, source, orientation, delta,
                            delta_discrete)

    @callback
    def _key(self, time_msec, keycode, state, keysyms):
        result = self.on_key(time_msec, keycode, state, keysyms)
        return result

    @callback
    def _modifiers(self, depressed, latched, locked, group):
        self.modifiers = depressed
        return self.on_modifiers(self.modifiers)

    @callback
    def _layout_change(self, width, height):
        self.width = width
        self.height = height
        self.on_layout_change()

    @callback
    def _update_view(self, handle, *args):
        try:
            v = self._views[handle]
            try:
                res = v._update(*args)
                return res
            except Exception as e:
                print(e)
        except Exception:
            view = self._view_class(self, handle)
            self._views[handle] = view

            view._update(*args)
            self._execute_view_main(view)

            res = view._update(*args)
            return res

    @callback
    def _update_widget(self, handle, *args):
        try:
            res = self._widgets[handle]._update(*args)
            return res
        except Exception as e:
            return None


    @callback
    def _update_widget_pixels(self, handle, *args):
        try:
            return self._widgets[handle]._update_pixels(*args)
        except Exception:
            return None


    @callback
    def _destroy_view(self, handle):
        try:
            self._views[handle].destroy()
            self._views.pop(handle, None)
        except Exception:
            pass

        for h in self._views:
            if self._views[h].parent is not None and self._views[h].parent._handle == handle:
                self._views[h].parent = None

    @callback
    def _view_event(self, handle, event):
        try:
            self._views[handle].on_event(event)
        except Exception:
            pass


    @callback
    def _query_new_widget(self, new_handle):
        if len(self._pending_widgets) > 0:
            widget = self._pending_widgets.pop(0)
            widget._handle = new_handle
            self._widgets[new_handle] = widget
            return True

        return False
    
    @callback
    def _query_destroy_widget(self):
        if len(self._pending_destroy_widgets) > 0:
            return self._pending_destroy_widgets.pop(0)._handle

        return None

    @callback
    def _query(self):
        res = (self._pending_update_cursor, self._pending_terminate)
        self._pending_update_cursor = False

        return res

    def widget_destroy(self, widget):
        self._widgets.pop(widget._handle, None)
        self._pending_destroy_widgets += [widget]

    def _gesture(self, gesture):
        self._touchpad_captured = self.on_gesture(gesture)
        gesture.listener(GestureListener(None, self._gesture_finished))

    def _gesture_finished(self):
        self._touchpad_captured = False

    """
    Public API
    """

    def run(self):
        print("Python: Running PyWM...")
        run(**self.config)
        print("Python: ...finished")

    def terminate(self):
        if self._touchpad_main is not None:
            self._touchpad_main.stop()
        self._pending_terminate = True

    def create_widget(self, widget_class, *args, **kwargs):
        widget = widget_class(self, *args, **kwargs)
        self._pending_widgets += [widget]
        return widget

    def update_cursor(self):
        self._pending_update_cursor = True

    """
    Virtual methods
    """

    def main(self):
        pass

    def _execute_view_main(self, view):
        view.main()

    def on_layout_change(self):
        pass

    def on_motion(self, time_msec, delta_x, delta_y):
        return False

    def on_button(self, time_msec, button, state):
        return False

    def on_axis(self, time_msec, source, orientation, delta, delta_discrete):
        return False

    def on_gesture(self, gesture):
        return False

    def on_key(self, time_msec, keycode, state, keysyms):
        """
        keycode: raw xkb keycode, probably useless
        state: PYWM_PRESSED, PYWM_RELEASED
        keysyms: translated keysymbols, like "a", "b", "A", "Left", ...
        """
        return False

    def on_modifiers(self, modifiers):
        return False

