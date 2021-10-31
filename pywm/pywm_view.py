from __future__ import annotations
from typing import TYPE_CHECKING, Optional, TypeVar, Generic, Any

import logging
from abc import abstractmethod

# Python imports are great
if TYPE_CHECKING:
    from .pywm import PyWM, PyWMOutput, ViewT
    PyWMT = TypeVar('PyWMT', bound=PyWM)
else:
    PyWMT = TypeVar('PyWMT')

logger: logging.Logger = logging.getLogger(__name__)

class PyWMViewUpstreamState:
    def __init__(self,
                 floating: bool, title: str,
                 sc_min_w: int, sc_max_w: int, sc_min_h: int, sc_max_h: int,
                 offset_x: int, offset_y: int,
                 width: int, height: int,
                 is_focused: bool, is_fullscreen: bool, is_maximized: bool, is_resizing: bool, is_inhibiting_idle: bool) -> None:

        """
        Called from C - just to be sure, wrap every attribute in type constrcutor
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
                 z_index: int=0, box: tuple[float, float, float, float]=(0, 0, 0, 0),
                 mask: tuple[float, float, float, float]=(-1, -1, -1, -1),
                 opacity: float=1., corner_radius: float=0,
                 accepts_input: bool=False, lock_enabled: bool=False, fixed_output: Optional[PyWMOutput]=None, up_state: Optional[PyWMViewUpstreamState]=None) -> None:
        """
        Just to be sure - wrap in type constructors
        """
        self.z_index = int(z_index)
        self.box = (float(box[0]), float(box[1]), float(box[2]), float(box[3]))
        self.mask = (float(mask[0]), float(mask[1]), float(mask[2]), float(mask[3]))
        self.opacity = opacity
        self.corner_radius = corner_radius
        self.accepts_input = accepts_input
        self.lock_enabled = lock_enabled
        self.fixed_output = fixed_output

        """
        Request size
        """
        self.size: tuple[int, int] = (-1, -1)

        if up_state is not None:
            self.size = up_state.size

    def copy(self) -> PyWMViewDownstreamState:
        res = PyWMViewDownstreamState(self.z_index, self.box, self.mask, self.opacity, self.corner_radius, self.accepts_input, self.lock_enabled)
        res.size = self.size
        return res

    def get(self, root: PyWM[ViewT],
            last_state: Optional[PyWMViewDownstreamState],
            focus: Optional[int], fullscreen: Optional[int], maximized: Optional[int], resizing: Optional[int], close: Optional[int]
            ) -> tuple[tuple[float, float, float, float], tuple[float, float, float, float], float, float, int, bool, bool, tuple[int, int], int, int, int, int, int, str]:

        return (
            root.round(*self.box),
            self.mask,
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
            int(close) if close is not None else -1,
            self.fixed_output.name if self.fixed_output is not None else ""
        )

    def __str__(self) -> str:
        return str(self.__dict__)


class PyWMView(Generic[PyWMT]):
    def __init__(self, wm: PyWMT, handle: int) -> None: # Python imports are great
        self._handle = handle

        self.wm = wm
        self.parent: Optional[PyWMView] = None
        self.pid: Optional[int] = None
        self.app_id: Optional[str] = None
        self.role: Optional[str] = None
        self.is_xwayland: Optional[bool] = None

        self.up_state: Optional[PyWMViewUpstreamState] = None
        self.last_up_state: Optional[PyWMViewUpstreamState] = None

        self._damaged = False
        self._down_state: Optional[PyWMViewDownstreamState] = None
        self._last_down_state: Optional[PyWMViewDownstreamState] = None

        self._down_action_focus: Optional[int] = None
        self._down_action_fullscreen: Optional[int] = None
        self._down_action_maximized: Optional[int] = None
        self._down_action_resizing: Optional[int] = None
        self._down_action_close: Optional[int] = None


    def _update(self, parent_handle: int, is_xwayland: bool, pid: int, app_id: str, role: str,
                floating: bool, title: str,
                sc_min_w: int, sc_max_w: int, sc_min_h: int, sc_max_h: int,
                offset_x: int, offset_y: int,
                width: int, height: int,
                is_focused: bool, is_fullscreen: bool, is_maximized: bool, is_resizing: bool, is_inhibiting_idle: bool
                ) -> tuple[tuple[float, float, float, float], tuple[float, float, float, float], float, float, int, bool, bool, tuple[int, int], int, int, int, int, int, str]:

        if self.parent is None and parent_handle is not None:
            try:
                self.parent = self.wm._views[parent_handle]
            except Exception:
                pass

        self.is_xwayland = is_xwayland
        self.pid = pid
        self.app_id = app_id
        self.role = role

        up_state = PyWMViewUpstreamState(
                floating, title,
                sc_min_w, sc_max_w, sc_min_h, sc_max_h,
                offset_x, offset_y,
                width, height,
                is_focused, is_fullscreen, is_maximized, is_resizing, is_inhibiting_idle)
        last_up_state = self.up_state
        down_state: Optional[PyWMViewDownstreamState] = self._down_state

        self.last_up_state = last_up_state
        self.up_state = up_state

        if down_state is None:
            """
            Initial
            """
            down_state = PyWMViewDownstreamState(up_state=up_state)

        elif self._damaged or last_up_state is None or up_state.is_update(last_up_state):
            self._damaged = False
            """
            Update
            """
            if last_up_state is None or up_state.is_focused != last_up_state.is_focused:
                self.on_focus_change()

            if last_up_state is None or up_state.size != last_up_state.size or \
                    up_state.size_constraints != last_up_state.size_constraints:
                self.on_size_or_constraints_change()

            try:
                down_state = self.process(up_state)
            except Exception as e:
                logger.exception("Exception during view.process")
                down_state = self._last_down_state

            # BEGIN DEBUG
            if down_state is not None and down_state.size != up_state.size:
                logger.debug("Size (%d %s) %s -> %s", self._handle, self.app_id, up_state.size, down_state.size)
            # END DEBUG

        self._last_down_state = self._down_state
        self._down_state = down_state

        if down_state is None:
            logger.warn("No down stream state")
            down_state = PyWMViewDownstreamState()

        res = down_state.get(
            self.wm,
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


