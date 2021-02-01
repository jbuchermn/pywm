# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status          | Open issues                                                   |
|------------------------|-----------|-----------------|---------------------------------------------------------------|
| Termite                |           | working         | Touchpad scrolling not yet working                            |
| Chromium               |           | working         | "Save as" Dialog doesn't open as a subview                    |
| Chromium               |    yes    | ?               | ?                                                             |
| Firefox                |           | ?               | ?                                                             |
| Firefox                |    yes    | ?               | ?                                                             |
| Matplotlib             |    ?      | ?               | ?                                                             |
| IntelliJ               |    ?      | ?               | ?                                                             |
| Atom                   |    ?      | ?               | ?                                                             |
| VSCode                 |    ?      | ?               | ?                                                             |
| GIMP                   |    ?      | ?               | ?                                                             |
| LibreOffice            |    ?      | ?               | ?                                                             |
| imv                    |           | not working     | Doesn't open - causes other apps to hang                      |
| OpenSCAD               |    ?      | ?               | ?                                                             |


# Backlog

### General
- Random hangs - e.g. playing around with nnn, opening imv
- Possibly move over to asnycio instead of Threading
- Improve touchpad integration (bouncy scrolling at edges e.g., do not fix gesture to a single finger, which is
  necessary on Lenove, not on MBP, ...)
- Views change their minimal width/height after initial set; this is ignored leading to ugly scaling
- Cursor not displayed initially (Open termite, fullscreen, open chromium, move cursor.. nothing there)
- Mysterious bugs when ordering widgets in `layout.py` is changed

### XWayland
- Check that it works: Detect modals as child-views (analogously to child-toplevels)
- Views are allowed to configure in any way they want (-> lots of overlapping modals and similar shit)
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side
