# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status          | Open issues                                                   |
|------------------------|-----------|-----------------|---------------------------------------------------------------|
| Termite                |    no     | working         |                none                                           |
| imv                    |    no     | working         |                none                                           |
| LibreOffice            |    no     | working         | Welcome Window is not display (properly)                      |
| Alacritty              |    no     | working         | Looks terrible                                                |
| Chromium               |    no     | working         | Menus outside view / weird behaviour entering search term (related to full-screen YT?)     |
| Firefox                |    no     | working         | "Save as" Dialog possibly scaled weirdly                      |
| Atom                   |    no     | ?               | ?                                                             |
| VSCodium               |    no     | ?               | ?                                                             |
| Matplotlib (Qt5)       |    no     | working         | Requires DISPLAY=":0" to be set?!                             |

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

- Change cursor (in general + e.g. when over a link)
- Clipboard

## Backlog

### General

- Random hangs - e.g. playing around with nnn, reordering widgets in `layout.py`, poor wifi / iwd does something
    Possibly:[ERROR] [backend/drm/atomic.c:36] eDP-1: Atomic commit failed (pageflip): Device or resource busy

- Various TODO Comments
- Function / Media keys + widgets
- Cursor not displayed initially
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
- Scale windows in on open
- Swipe Overlay: Bouncy overswipe effect (i.e. not a whole tile)
