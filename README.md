# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status          | Open issues                                                   |
|------------------------|-----------|-----------------|---------------------------------------------------------------|
| Termite                |    no     | working         | none                                                          |
| imv                    |    no     | working         | none                                                          |
| LibreOffice            |    no     | working         | Mouse over tooltip (Calc drag-complete) isnt handled properly |
| Alacritty              |    no     | working         | Looks terrible                                                |
| Chromium               |    no     | working         | "Save as" Dialog doesn't open as a subview / Menus outside view|
| Firefox                |    no     | working         | Mouse over menus is not handled / "Save as" Dialog possibly scaled weirdly |
| Atom                   |    no     | ?               | ?                                                             |
| VSCodium               |    no     | ?               | ?                                                             |
| Matplotlib             |    no     | ?               | ?                                                             |
| IntelliJ               |    ?      | ?               | ?                                                             |
| GIMP                   |    ?      | ?               | ?                                                             |
| OpenSCAD               |    ?      | ?               | ?                                                             |
| FreeCAD                |    ?      | ?               | ?                                                             |
|------------------------|-----------|-----------------|---------------------------------------------------------------|
| VSCodium               |    yes    | working         | no hidpi scaling                                              |
| Firefox                |    yes    | working         | "Save as" Dialog possibly scaled weirdly / no hidpi scaling   |
| Matplotlib             |    yes    | not working     |                                                               |
| Atom                   |    yes    | ?               | ?                                                             |
| Chromium               |    yes    | ?               | ?                                                             |


# ToDo

- Cursor not displayed initially (Open termite, fullscreen, open chromium, move cursor.. nothing there)
- Cursor changes
- Firefox / LibreOffice bug concerning popups
- Alacritty looks terrible
- Python: Improved "find next window" logic on Alt-hjkl
- Clipboard

# Backlog

### General
- Random hangs - e.g. playing around with nnn, ordering widgets in `layout.py` is changed
    Possibly:[ERROR] [backend/drm/atomic.c:36] eDP-1: Atomic commit failed (pageflip): Device or resource busy
- Improve touchpad integration (bouncy scrolling at edges e.g., do not fix gesture to a single finger, which is
  necessary on Lenove, not on MBP, ...)
- Possibly move over to asnycio instead of Threading
- MBP function keys + widgets
- Lid and hibernate
- MBP keymap (incl Command+c / Command-v)

### XWayland
- setenv DISPLAY
- Check that it works: Detect modals as child-views (analogously to child-toplevels)
- Views are allowed to configure in any way they want (-> lots of overlapping modals and similar shit)
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side

### Python reference implementation
- Center windows of 1x2 or 2x1 in regular view and overview
