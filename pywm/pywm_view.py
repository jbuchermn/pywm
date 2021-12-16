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
                 mapped: bool,
                 floating: bool,
                 size_constraints: list[int],
                 offset_x: int, offset_y: int,
                 width: int, height: int,
                 is_focused: bool, is_fullscreen: bool, is_maximized: bool, is_resizing: bool, is_inhibiting_idle: bool, shows_csd: bool, fixed_output: Optional[PyWMOutput]) -> None:

        """
        Called from C - just to be sure, wrap every attribute in type constrcutor
        """

        self.is_mapped = bool(mapped)
        self.is_floating = bool(floating)

        """
        min_w, max_w, min_h, max_h for regular views
        anchor, desired_width, desired_height, exclusive_zone, layer, margin - left, top, right, bottom for layer shell
        """
        self.size_constraints = [int(i) for i in size_constraints]
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

        self.shows_csd = shows_csd

        self.fixed_output = fixed_output

    def is_update(self, other: PyWMViewUpstreamState) -> bool:
        if self.is_floating != other.is_floating:
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
        if self.fixed_output != other.fixed_output:
            return True
        return False


class PyWMViewDownstreamState:
    def __init__(self,
                 z_index: int=0, box: tuple[float, float, float, float]=(0, 0, 0, 0),
                 mask: tuple[float, float, float, float]=(-1, -1, -1, -1),
                 opacity: float=1., corner_radius: float=0,
                 accepts_input: bool=False,
                 lock_enabled: bool=False,
                 floating: Optional[bool]=None,
                 workspace: Optional[tuple[float, float, float, float]]=None,
                 fixed_output: Optional[PyWMOutput]=None,
                 up_state: Optional[PyWMViewUpstreamState]=None) -> None:
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
        self.floating = floating
        self.fixed_output = fixed_output
        self.workspace = workspace

        """
        Request size
        """
        self.size: tuple[int, int] = (-1, -1)

        if up_state is not None:
            self.fixed_output = up_state.fixed_output
            self.size = up_state.size

    def copy(self) -> PyWMViewDownstreamState:
        res = PyWMViewDownstreamState(self.z_index, self.box, self.mask, self.opacity, self.corner_radius, self.accepts_input, self.lock_enabled, self.floating, self.workspace, self.fixed_output)
        res.size = self.size
        return res

    def get(self, root: PyWM[ViewT],
            last_state: Optional[PyWMViewDownstreamState],
            force_size: bool,
            focus: Optional[int], fullscreen: Optional[int], maximized: Optional[int], resizing: Optional[int], close: Optional[int]
            ) -> tuple[tuple[float, float, float, float], tuple[float, float, float, float], float, float, int, bool, bool, int, tuple[int, int], int, int, int, int, int, int, tuple[float, float, float, float]]:

        return (
            root.round(*self.box),
            self.mask,
            self.opacity,
            self.corner_radius,
            int(self.z_index),
            bool(self.accepts_input),
            bool(self.lock_enabled),
            int(self.floating) if self.floating is not None else -1,

            self.size if last_state is None or self.size != last_state.size or force_size else (-1, -1),

            int(focus) if focus is not None else -1,
            int(fullscreen) if fullscreen is not None else -1,
            int(maximized) if maximized is not None else -1,
            int(resizing) if resizing is not None else -1,
            int(close) if close is not None else -1,
            int(self.fixed_output._key) if self.fixed_output is not None else -1,
            root.round(*self.workspace) if self.workspace is not None else (0, 0, -1, -1)
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

        self._down_force_size: bool = False
        self._down_action_focus: Optional[int] = None
        self._down_action_fullscreen: Optional[int] = None
        self._down_action_maximized: Optional[int] = None
        self._down_action_resizing: Optional[int] = None
        self._down_action_close: Optional[int] = None

        # Debug flag
        self._debug_scaling = False
        self._last_update_potential_scaling_issue = False

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, PyWMView):
            return False
        return self._handle == other._handle


    def _update(self,
                general: Optional[tuple[int, bool, int, str, str, str]],
                width: int, height: int,
                is_mapped: bool, is_floating: bool, is_focused: bool, is_fullscreen: bool, is_maximized: bool, is_resizing: bool, is_inhibiting_idle: bool,
                size_constraints: list[int],
                offset_x: int, offset_y: int,
                shows_csd: bool,
                fixed_output_key: int,
                ) -> tuple[tuple[float, float, float, float], tuple[float, float, float, float], float, float, int, bool, bool, int, tuple[int, int], int, int, int, int, int, int, tuple[float, float, float, float]]:

        if general is not None:
            if self.parent is None:
                try:
                    self.parent = self.wm._views[general[0]]
                except Exception:
                    pass

            self.is_xwayland = general[1]
            self.pid = general[2]
            self.app_id = general[3]
            self.role = general[4]
            self.title = general[5]

        up_state = PyWMViewUpstreamState(
            is_mapped,
            is_floating,
            size_constraints,
            offset_x, offset_y,
            width, height,
            is_focused, is_fullscreen, is_maximized, is_resizing, is_inhibiting_idle,
            shows_csd,
            self.wm.get_output_by_key(fixed_output_key) if fixed_output_key >= 0 else None
        )
        last_up_state = self.up_state
        down_state: Optional[PyWMViewDownstreamState] = self._down_state

        self.last_up_state = last_up_state
        self.up_state = up_state

        if down_state is None:
            """
            Initial
            """
            down_state = PyWMViewDownstreamState(up_state=up_state)

            if up_state.is_mapped:
                self.on_map()

        elif self._damaged or last_up_state is None or up_state.is_update(last_up_state):
            """
            Update
            """
            self._damaged = False
            if last_up_state is None or up_state.is_focused != last_up_state.is_focused:
                self.on_focus_change()

            if (last_up_state is None or up_state.size != last_up_state.size):
                self.on_resized(*up_state.size, self._last_down_state is None or up_state.size != self._last_down_state.size)

            if up_state.is_mapped and (last_up_state is None or not last_up_state.is_mapped):
                self.on_map()

            try:
                down_state = self.process(up_state)
            except Exception as e:
                logger.exception("Exception during view.process")
                down_state = self._last_down_state

        self._last_down_state = self._down_state
        self._down_state = down_state

        if down_state is None:
            logger.warn("No down stream state")
            down_state = PyWMViewDownstreamState()

        res = down_state.get(
            self.wm,
            self._last_down_state,
            self._down_force_size,
            self._down_action_focus,
            self._down_action_fullscreen,
            self._down_action_maximized,
            self._down_action_resizing,
            self._down_action_close
        )
        self._down_force_size = False
        self._down_action_focus = None
        self._down_action_fullscreen = None
        self._down_action_maximized = None
        self._down_action_resizing = None
        self._down_action_close = None

        if self._debug_scaling:
            if res[8] != (-1, -1):
                logger.debug("Scaling - Resize to %dx%d" % res[8])

            check_wh = res[0][2], res[0][3]
            check_size = down_state.size if down_state.size is not None and down_state.size[0] > 0 and down_state.size[1] > 0 else up_state.size
            if (0.99 * check_wh[0] < check_size[0] < 1.01 * check_wh[0]) and \
               (0.99 * check_wh[1] < check_size[1] < 1.01 * check_size[1]) and \
               (abs(check_wh[0] - check_size[0]) > 0.001 or abs(check_wh[1] - check_size[1]) > 0.001):

                logger.debug("Scaling WRONG: %dx%d placed in %fx%f" % (*check_size, *check_wh))
                self._last_update_potential_scaling_issue = True
            elif self._last_update_potential_scaling_issue:
                logger.debug("Scaling OK:    %dx%d placed in %fx%f" % (*check_size, *check_wh))
                self._last_update_potential_scaling_issue = False

        return res


    def focus(self) -> None:
        self._down_action_focus = True

    def set_resizing(self, val: bool) -> None:
        self._down_action_resizing = bool(val)

    def set_fullscreen(self, val: bool) -> None:
        self._down_action_fullscreen = bool(val)

    def set_maximized(self, val: bool) -> None:
        self._down_action_maximized = bool(val)

    def force_size(self) -> None:
        self._down_force_size = True

    def close(self) -> None:
        self._down_action_close = True

    def damage(self) -> None:
        self._damaged = True

    
    """
    Virtual methods
    """

    @abstractmethod
    def init(self) -> PyWMViewDownstreamState:
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
    def on_map(self) -> None:
        pass

    def on_focus_change(self) -> None:
        pass

    def on_resized(self, width: int, height: int, client_leading: bool) -> None:
        pass
