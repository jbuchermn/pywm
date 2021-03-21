from __future__ import annotations

import logging
from typing import Optional, Tuple

from . import pywm
from abc import abstractmethod

logger = logging.getLogger(__name__)

class PyWMViewUpstreamState:
    def __init__(self,
                 floating: bool, title: str,
                 sc_min_w: int, sc_max_w: int, sc_min_h: int, sc_max_h: int,
                 offset_x: int, offset_y: int,
                 width: int, height: int,
                 is_focused: bool, is_fullscreen: bool, is_maximized: bool, is_resizing: bool, is_inhibiting_idle: bool):

        """
        Called from C - just to bes ure, wrap every attribute in type constrcutor
        """
        self.is_floating = bool(floating)
        self.title = str(title)

        """
        min_w, max_w, min_h, max_h
        """
        self.size_constraints = (int(sc_min_w), int(sc_max_w), int(sc_min_h), int(sc_max_h))        
        """
        describe the offset of actual content within the view
        (in case of CSD)

        offset_x, offset_y
        """
        self.offset = (int(offset_x), int(offset_y))

        """
        width, height
        """
        self.size = (int(width), int(height))

        self.is_focused = bool(is_focused)
        self.is_fullscreen = bool(is_fullscreen)
        self.is_maximized = bool(is_maximized)
        self.is_resizing = bool(is_resizing)
        self.is_inhibiting_idle = bool(is_inhibiting_idle)

    def is_update(self, other: PyWMViewUpstreamState) -> bool:
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
        if self.is_inhibiting_idle != other.is_inhibiting_idle:
            return True
        return False


class PyWMViewDownstreamState:
    def __init__(self,
                 z_index: int=0, box: Tuple[float, float, float, float]=(0, 0, 0, 0),
                 opacity: float=1., corner_radius: float=0,
                 accepts_input: bool=False, lock_enabled: bool=False, up_state: Optional[PyWMViewUpstreamState]=None):
        """
        Just to be sure - wrap in type constructors
        """
        self.z_index = int(z_index)
        self.box = (float(box[0]), float(box[1]), float(box[2]), float(box[3]))
        self.opacity = opacity
        self.corner_radius = corner_radius
        self.accepts_input = accepts_input
        self.lock_enabled = lock_enabled

        """
        Request size
        """
        self.size = (-1, -1)

        if up_state is not None:
            self.size = up_state.size

    def copy(self) -> PyWMViewDownstreamState:
        res = PyWMViewDownstreamState(self.z_index, self.box, self.opacity, self.corner_radius, self.accepts_input, self.lock_enabled)
        res.size = self.size
        return res

    def get(self,
            last_state: Optional[PyWMViewDownstreamState],
            focus: Optional[int], fullscreen: Optional[int], maximized: Optional[int], resizing: Optional[int], close: Optional[int]
            ) -> Tuple[Tuple[float, float, float, float], float, float, int, bool, bool, Tuple[int, int], int, int, int, int, int]:
        return (
            self.box,
            self.opacity,
            self.corner_radius,
            int(self.z_index),
            bool(self.accepts_input),
            bool(self.lock_enabled),

            self.size if last_state is None or self.size != last_state.size else (-1, -1),

            int(focus) if focus is not None else -1,
            int(fullscreen) if fullscreen is not None else -1,
            int(maximized) if maximized is not None else -1,
            int(resizing) if resizing is not None else -1,
            int(close) if close is not None else -1
        )

    def __str__(self) -> str:
        return str(self.__dict__)



class PyWMView:
    def __init__(self, wm: pywm.PyWM, handle: int): # Python imports are great
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

        self._parent: Optional[PyWMView] = None


    def _update(self, parent_handle: int, is_xwayland: bool, app_id: str, role: str,
                floating: bool, title: str,
                sc_min_w: int, sc_max_w: int, sc_min_h: int, sc_max_h: int,
                offset_x: int, offset_y: int,
                width: int, height: int,
                is_focused: bool, is_fullscreen: bool, is_maximized: bool, is_resizing: bool, is_inhibiting_idle: bool
                ) -> Tuple[Tuple[float, float, float, float], float, float, int, bool, bool, Tuple[int, int], int, int, int, int, int]:
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

        self.up_state = PyWMViewUpstreamState(
                floating, title,
                sc_min_w, sc_max_w, sc_min_h, sc_max_h,
                offset_x, offset_y,
                width, height,
                is_focused, is_fullscreen, is_maximized, is_resizing, is_inhibiting_idle)

        if self._down_state is None:
            """
            Initial
            """
            self._last_down_state = PyWMViewDownstreamState(up_state=self.up_state)
            self._down_state = self._last_down_state.copy()

        elif self._damaged or self.last_up_state is None or self.up_state.is_update(self.last_up_state):
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
                logger.exception("Exception during view.process")
                self._down_state = self._last_down_state

            # BEGIN DEBUG
            try:
                if self._down_state.size != self.up_state.size:
                    logger.debug("Size (%d %s) %s -> %s", self._handle, self.app_id, self.up_state.size, self._down_state.size)
            except:
                pass
            # END DEBUG

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


    def focus(self) -> None:
        self._down_action_focus = True

    def set_resizing(self, val: bool) -> None:
        self._down_action_resizing = bool(val)

    def set_fullscreen(self, val: bool) -> None:
        self._down_action_fullscreen = bool(val)

    def set_maximized(self, val: bool) -> None:
        self._down_action_maximized = bool(val)

    def close(self) -> None:
        self._down_action_close = True

    def damage(self) -> None:
        self._damaged = True

    
    """
    Virtual methods
    """
    def main(self, *args) -> None:

        """
        Called after view is initialized, before first process
        """
        pass

    def on_event(self, event: str) -> None:
        pass

    @abstractmethod
    def process(self, up_state: PyWMViewUpstreamState) -> PyWMViewDownstreamState:
        """
        return next down_state based on up_state (and whatever state the implementation uses)
        """
        pass


    def destroy(self) -> None:
        """
        Called after the view is destroyed on c-side
        """
        pass

    """
    Callbacks on various specific updates
    Notice, that these will always be called together with (before) process
    """
    def on_focus_change(self) -> None:
        pass

    def on_size_or_constraints_change(self) -> None:
        pass


