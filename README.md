# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status          | Open issues                                                   |
|------------------------|-----------|-----------------|---------------------------------------------------------------|
| Termite                |           | working         | Touchpad scrolling not yet working                            |
| Alacritty              |           | working         | Touchpad scrolling not yet working / looks terrible           |
| Chromium               |           | working         | "Save as" Dialog doesn't open as a subview                    |
| Chromium               |    yes    | ?               | ?                                                             |
| Firefox                |           | ?               | ?                                                             |
| Firefox                |    yes    | ?               | ?                                                             |
| Matplotlib             |    ?      | ?               | ?                                                             |
| IntelliJ               |    ?      | ?               | ?                                                             |
| Atom                   |    ?      | ?               | ?                                                             |
| VSCode                 |    ?      | ?               | ?                                                             |
| GIMP                   |    ?      | ?               | ?                                                             |
| LibreOffice            |           | working         | Very poor performance / Mouse movement is not tracked         |
| imv                    |           | working         | Touchpad scrolling / dragging not yet working                 |
| OpenSCAD               |    ?      | ?               | ?                                                             |


# ToDo

- Cursor not displayed initially (Open termite, fullscreen, open chromium, move cursor.. nothing there)
- Mouse interaction
- XWayland: mouse interaction
- Alacritty looks terrible

# Backlog

### General
- Random hangs - e.g. playing around with nnn
- Possibly move over to asnycio instead of Threading
- Improve touchpad integration (bouncy scrolling at edges e.g., do not fix gesture to a single finger, which is
  necessary on Lenove, not on MBP, ...)
- Mysterious bugs when ordering widgets in `layout.py` is changed
- MBP function keys + widgets
- Lid and hibernate

### XWayland
- setenv DISPLAY
- Check that it works: Detect modals as child-views (analogously to child-toplevels)
- Views are allowed to configure in any way they want (-> lots of overlapping modals and similar shit)
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side
