import logging

from abc import abstractmethod

class PyWMViewUpstreamState:
    def __init__(self, *args):
        if len(args) != 14:
            raise Exception("Unexpected number of args")
        self.is_floating = bool(args[0])
        self.title = str(args[1])

        """
        min_w, max_w, min_h, max_h
        """
        self.size_constraints = (int(args[2]), int(args[3]), int(args[4]), int(args[5]))        
        """
        describe the offset of actual content within the view
        (in case of CSD)

        offset_x, offset_y
        """
        self.offset = (int(args[6]), int(args[7]))

        """
        width, height
        """
        self.size = (int(args[8]), int(args[9]))

        self.is_focused = bool(args[10])
        self.is_fullscreen = bool(args[11])
        self.is_maximized = bool(args[12])
        self.is_resizing = bool(args[13])

    def is_update(self, other):
        if self.is_floating != other.is_floating:
            return True
        if self.title != other.title:
            return True
        if self.size_constraints != other.size_constraints:
            return True
        if self.offset != other.offset:
            return True
        if self.size != other.size:
            return True
        if self.is_focused != other.is_focused:
            return True
        if self.is_fullscreen != other.is_fullscreen:
            return True
        if self.is_maximized != other.is_maximized:
            return True
        if self.is_resizing != other.is_resizing:
            return True
        return False


class PyWMViewDownstreamState:
    def __init__(self, z_index=0, box=(0, 0, 0, 0), corner_radius=0, accepts_input=False, up_state=None):
        self.z_index = int(z_index)
        self.box = (float(box[0]), float(box[1]), float(box[2]), float(box[3]))
        self.corner_radius = corner_radius
        self.accepts_input = accepts_input

        """
        Request size
        """
        self.size = (-1, -1)

        if up_state is not None:
            self.size = up_state.size

    def copy(self):
        res = PyWMViewDownstreamState(self.z_index, self.box, self.accepts_input)
        res.size = self.size
        return res

    def get(self, last_state, focus, fullscreen, maximized, resizing, close):
        return (
            self.box,
            self.corner_radius,
            int(self.z_index),
            bool(self.accepts_input),

            self.size if self.size != last_state.size else (-1, -1),

            int(focus) if focus is not None else -1,
            int(fullscreen) if fullscreen is not None else -1,
            int(maximized) if maximized is not None else -1,
            int(resizing) if resizing is not None else -1,
            int(close) if close is not None else -1
        )

    def __str__(self):
        return str(self.__dict__)



class PyWMView:
    def __init__(self, wm, handle):
        self._handle = handle

        self.wm = wm
        self.parent = None
        self.app_id = None
        self.role = None
        self.is_xwayland = None

        self.up_state = None
        self.last_up_state = None

        self._damaged = False
        self._down_state = None
        self._last_down_state = None

        self._down_action_focus = None
        self._down_action_fullscreen = None
        self._down_action_maximized = None
        self._down_action_resizing = None
        self._down_action_close = None


    def _update(self, parent_handle, is_xwayland, app_id, role, *args):
        if self.parent is None and parent_handle is not None:
            try:
                self._parent = self.wm._views[parent_handle]
            except Exception:
                pass

        self.is_xwayland = is_xwayland
        self.app_id = app_id
        self.role = role

        self.last_up_state = self.up_state
        self._last_down_state = self._down_state

        self.up_state = PyWMViewUpstreamState(*args)

        if self._down_state is None:
            """
            Initial
            """
            self._last_down_state = PyWMViewDownstreamState(up_state=self.up_state)
            self._down_state = self._last_down_state.copy()

        elif self._damaged or self.up_state.is_update(self.last_up_state):
            self._damaged = False
            """
            Update
            """
            if self.up_state.is_focused != self.last_up_state.is_focused:
                self.on_focus_change()

            if self.up_state.size != self.last_up_state.size or \
                    self.up_state.size_constraints != self.last_up_state.size_constraints:
                self.on_size_or_constraints_change()

            try:
                self._down_state = self.process(self.up_state)
            except Exception as e:
                logging.exception("Exception during view.process")
                self._down_state = self._last_down_state

        res = self._down_state.get(
            self._last_down_state, 
            self._down_action_focus,
            self._down_action_fullscreen,
            self._down_action_maximized,
            self._down_action_resizing,
            self._down_action_close
        )
        self._down_action_focus = None
        self._down_action_fullscreen = None
        self._down_action_maximized = None
        self._down_action_resizing = None
        self._down_action_close = None
        return res


    def focus(self):
        self._down_action_focus = True

    def set_resizing(self, val):
        self._down_action_resizing = bool(val)

    def set_fullscreen(self, val):
        self._down_action_fullscreen = bool(val)

    def set_maximized(self, val):
        self._down_action_maximized = bool(val)

    def close(self):
        self._down_action_close = True

    def damage(self):
        self._damaged = True

    
    """
    Virtual methods
    """
    def main(self, *args):

        """
        Called after view is initialized, before first process
        """
        pass

    def on_event(self, event):
        pass

    @abstractmethod
    def process(self, up_state):
        """
        return next down_state based on up_state (and whatever state the implementation uses)
        """
        pass


    def destroy(self):
        """
        Called after the view is destroyed on c-side
        """
        pass

    """
    Callbacks on various specific updates
    Notice, that these will always be called together with (before) process
    """
    def on_focus_change(self):
        pass

    def on_size_or_constraints_change(self):
        pass
