# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status          | Open issues                                                   |
|------------------------|-----------|-----------------|---------------------------------------------------------------|
| Termite                |    no     | working         | none                                                          |
| Alacritty              |    no     | working         | Looks terrible                                                |
| Chromium               |    no     | working         | "Save as" Dialog doesn't open as a subview                    |
| Chromium               |    yes    | ?               | ?                                                             |
| Firefox                |    no     | ?               | ?                                                             |
| Firefox                |    yes    | ?               | ?                                                             |
| Matplotlib             |    ?      | ?               | ?                                                             |
| IntelliJ               |    ?      | ?               | ?                                                             |
| Atom                   |    ?      | ?               | ?                                                             |
| VSCode                 |    ?      | ?               | ?                                                             |
| GIMP                   |    ?      | ?               | ?                                                             |
| LibreOffice            |    no     | working         | Very poor performance / Mouse movement is not tracked         |
| imv                    |    no     | working         | none                                                          |
| OpenSCAD               |    ?      | ?               | ?                                                             |
| FreeCAD                |    ?      | ?               | ?                                                             |


# ToDo

- Cursor not displayed initially (Open termite, fullscreen, open chromium, move cursor.. nothing there)
- XWayland: mouse interaction
- Alacritty looks terrible

# Backlog

### General
- Random hangs - e.g. playing around with nnn, ordering widgets in `layout.py` is changed
    Possibly:[ERROR] [backend/drm/atomic.c:36] eDP-1: Atomic commit failed (pageflip): Device or resource busy
- Possibly move over to asnycio instead of Threading
- Improve touchpad integration (bouncy scrolling at edges e.g., do not fix gesture to a single finger, which is
  necessary on Lenove, not on MBP, ...)
- MBP function keys + widgets
- Lid and hibernate

### XWayland
- setenv DISPLAY
- Check that it works: Detect modals as child-views (analogously to child-toplevels)
- Views are allowed to configure in any way they want (-> lots of overlapping modals and similar shit)
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side
