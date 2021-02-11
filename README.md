# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status              | Open issues                                              |
|------------------------|-----------|---------------------|----------------------------------------------------------|
| Termite                |    no     | working             |                none                                      |
| imv                    |    no     | working             |                none                                      |
| LibreOffice            |    no     | working             |                none                                      |
| Alacritty              |    no     | working             |                none                                      |
| Chromium               |    no     | working             | Too large popups are placed wrong (this is an issue with sway as well) |
| Firefox                |    no     | working             |                none                                      |
| Matplotlib (Qt5)       |    no     | working             |                none                                      |
| GIMP-2.99              |    no     | working             | Popups (actually floating toplevels) cannot be moved     |
| OpenSCAD               |    no     | working             |                none                                      |
| Zoom                   |    no     | working             | Popup-toplevels are not recognized as floating           |
| nemo                   |    no     | working             | Missing decorations on popups are not nice               |


| VSCodium               |    yes    | working             | No hidpi scaling                                         |
| Firefox                |    yes    | working             | "Save as" Dialog possibly scaled weirdly and unclickable |
| -                      |    -      | -                   | No hidpi scaling                                         |
| Matplotlib             |    yes    | not working         |                                                          |
| Zoom                   |    yes    | working very poorly | ?                                                        |


| Atom                   |    no     | ?                   | ?                                                        |
| VSCodium               |    no     | ?                   | ?                                                        |

| IntelliJ               |    ?      | ?                   | ?                                                        |
| FreeCAD                |    ?      | ?                   | ?                                                        |
| Atom                   |    yes    | ?                   | ?                                                        |
| Chromium               |    yes    | ?                   | ?                                                        |



## ToDo

- Handle shadow-related CSD-offsets at GIMP
- OpenGL shaders
    - ShapeCorners for KDE?
    - Blurry background?
    - Blurry rendering on demand? (make animations look nicer)

- Possibly use toplevel->parent to find floating ones?
- XDG: Moveable and (possibly) resizable floating toplevels
- Fullscreen mode on views (xdg-shell configure?) -> i.e. Chrome without client-side-decorations (wlr_xdg_toplevel_set_fullscreen)
- Close window functionality from Python (e.g. via Shortcut, for Chrome popups)

- Clipboard
- Cursor not displayed initially
- Change cursor (in general + e.g. when over a link)



## Backlog

### General

- Various TODO Comments
- Popup constraints on scaling
- MBP keymap (incl Command+c / Command-v)
- Screensharing using xdg-desktop-portal-wlr / Screenshots / -records
- Damaging regions
- Multiple outputs
- Login mechanism

### XWayland

- setenv DISPLAY
- HiDPI scaling

## Notes

- Firefox: MOZ_ENABLE_WAYLAND=1
- Chromium: --enable-features=UseOzonePlatform --ozone-platform=wayland
- Matplotlib / Qt5 on Wayland requires DISPLAY=":0" to be set?!
