# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status          | Open issues                                                   |
|------------------------|-----------|-----------------|---------------------------------------------------------------|
| Termite                |    no     | working         |                none                                           |
| imv                    |    no     | working         |                none                                           |
| LibreOffice            |    no     | working         | Welcome Window is not display (properly)                      |
| Alacritty              |    no     | working         | Looks terrible                                                |
| Chromium               |    no     | working         | Menus outside view                                            |
| Firefox                |    no     | working         | "Save as" Dialog possibly scaled weirdly                      |
| Atom                   |    no     | ?               | ?                                                             |
| VSCodium               |    no     | ?               | ?                                                             |
| Matplotlib             |    no     | ?               | ?                                                             |

| IntelliJ               |    ?      | ?               | ?                                                             |
| GIMP                   |    ?      | ?               | ?                                                             |
| OpenSCAD               |    ?      | ?               | ?                                                             |
| FreeCAD                |    ?      | ?               | ?                                                             |

| VSCodium               |    yes    | working         | no hidpi scaling                                              |
| Firefox                |    yes    | working         | "Save as" Dialog possibly scaled weirdly / no hidpi scaling   |
| Matplotlib             |    yes    | not working     |                                                               |
| Atom                   |    yes    | ?               | ?                                                             |
| Chromium               |    yes    | ?               | ?                                                             |


## ToDo

- Alacritty does not obtain information about the scale factor --> Blurry
- Have menus render inside parent boundaries (xdg)

- Cursor not displayed initially
- Change cursor (in general + e.g. when over a link)
- Clipboard

## Backlog

### General

- Clean up threading / asyncio --> possibly related to:
- Random hangs - e.g. playing around with nnn, ordering widgets in `layout.py` is changed
    Possibly:[ERROR] [backend/drm/atomic.c:36] eDP-1: Atomic commit failed (pageflip): Device or resource busy
- Improve touchpad integration (bouncy scrolling at edges e.g., do not fix gesture to a single finger, which is
  necessary on Lenovo, not on MBP, ...)
- Function / Media keys + widgets
- MBP keymap (incl Command+c / Command-v)
- Server-side decorations / Close window via shortcut (e.g. Chrome popups)
- Blurry rendering during animations on rescale
- NW.js shell
- Login mechanism

### XWayland

- setenv DISPLAY
- HiDPI scaling
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side

### Python reference implementation

- Center windows of 1x2 or 2x1 in regular view and overview
- Improve "find next window" logic on Alt-hjkl
- Titles during "far-away" view
