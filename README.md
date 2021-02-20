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
| masm11/Emacs           |    no     | working             |                none                                      |
| Chromium               |    no     | working             | Too large popups are placed wrong (this is an issue with sway as well) |
| OpenSCAD               |    no     | working             | Open Dialog is display very small                        |
| VLC                    |    no     | working             | Popup-toplevels are not recognized as floating           |
| Zoom                   |    no     | working             | Popup-toplevels are not recognized as floating           |
| Spotify                |    yes    | working             |                none                                      |
| VSCodium               |    yes    | working             | Dialogs not really working                               |
| Atom                   |    yes    | working             | Dialogs not really working                               |
| Emacs                  |    yes    | working             | Dialogs not really working                               |
| VLC                    |    yes    | working             | Dialogs not really working                               |
| Firefox                |    yes    | working             | "Save as" Dialog possibly scaled weirdly and unclickable |
| -                      |    -      | -                   | No hidpi scaling                                         |
| Zoom                   |    yes    | working very poorly | ?                                                        |
| Matplotlib             |    yes    | not working         |                                                          |
| Atom                   |    no     | ?                   | ?                                                        |
| VSCodium               |    no     | ?                   | ?                                                        |
| IntelliJ               |    ?      | ?                   | ?                                                        |
| FreeCAD                |    ?      | ?                   | ?                                                        |
| Atom                   |    yes    | ?                   | ?                                                        |
| Chromium               |    yes    | ?                   | ?                                                        |


## ToDo


## Backlog

### General

- Drag-n-drop
- Opacity of views
- Screensharing using xdg-desktop-portal-wlr / Screenshots / -records

- Possibly use toplevel -> parent to find floating windows?
- Popup constraints on scaling
- Damaging regions
- Multiple outputs
- Login mechanism
- Resizable toplevels
- Various TODO Comments

### XWayland

- Dialogs not working
- setenv DISPLAY

## Notes

- Firefox: MOZ_ENABLE_WAYLAND=1
- Chromium: --enable-features=UseOzonePlatform --ozone-platform=wayland
- Matplotlib / Qt5 on Wayland requires DISPLAY=":0" to be set?!
- Electron apps: --force-device-scale-factor=2
