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
| GIMP-2.99              |    no     | working             | Popups cannot be moved / resized                         |
| OpenSCAD               |    no     | working             |                none                                      |


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

- Moveable and resizable popups

- Cursor not displayed initially
- Clipboard
- Change cursor (in general + e.g. when over a link)

- Fullscreen mode on views (xdg-shell configure?) -> i.e. Chrome without client-side-decorations (wlr_xdg_toplevel_set_fullscreen)
- Close window functionality from Python (e.g. via Shortcut, for Chrome popups)

## Backlog

### General

- Various TODO Comments
- Move popups
- Popup constraints on scaling
- MBP keymap (incl Command+c / Command-v)
- Blurry rendering during animations on rescale
- Damaging regions
- Screensharing using xdg-desktop-portal-wlr / Screenshots / -records
- Blur the background via OpenGL shaders?

- Multiple outputs
- Login mechanism

### XWayland

- setenv DISPLAY
- HiDPI scaling
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side

## Notes

- Firefox: MOZ_ENABLE_WAYLAND=1
- Chromium: --enable-features=UseOzonePlatform --ozone-platform=wayland
- Matplotlib / Qt5 on Wayland requires DISPLAY=":0" to be set?!
