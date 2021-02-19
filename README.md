# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status              | Open issues                                              |
|------------------------|-----------|---------------------|----------------------------------------------------------|
| Termite                |    no     | working             |                none                                      |
| imv                    |    no     | working             |                none                                      |
| LibreOffice            |    no     | working             |                none                                      |
| Alacritty              |    no     | working             |                none                                      |
| Firefox                |    no     | working             |                none                                      |
| Matplotlib (Qt5)       |    no     | working             |                none                                      |
| GIMP-2.99              |    no     | working             |                none                                      |
| nemo                   |    no     | working             |                none                                      |
| Nautilus               |    no     | working             |                none                                      |
| Chromium               |    no     | working             | Too large popups are placed wrong (this is an issue with sway as well) |
| OpenSCAD               |    no     | working             | Open Dialog is display very small                        |
| Zoom                   |    no     | working             | Popup-toplevels are not recognized as floating           |


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

- Clipboard
- Clean up binding to wlroots
- Cursor not displayed initially

- Hide cursor on demand
- Change cursor (in general + e.g. when over a link)

- Opacity of views


## Backlog

### General

- Possibly use toplevel -> parent to find floating windows?
- Various TODO Comments
- Popup constraints on scaling
- Screensharing using xdg-desktop-portal-wlr / Screenshots / -records
- Damaging regions
- Multiple outputs
- Login mechanism
- Resizable toplevels

### XWayland

- setenv DISPLAY
- HiDPI scaling

## Notes

- Firefox: MOZ_ENABLE_WAYLAND=1
- Chromium: --enable-features=UseOzonePlatform --ozone-platform=wayland
- Matplotlib / Qt5 on Wayland requires DISPLAY=":0" to be set?!
