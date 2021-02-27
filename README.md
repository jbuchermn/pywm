# PyWM - Python-based Wayland window manager

Documentation to come

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
| VSCodium               |    yes    | working             |                none                                      |
| Atom                   |    yes    | working             |                none                                      |
| VLC                    |    yes    | working             |                none                                      |
| Firefox                |    yes    | working             |                none                                      |
| Zoom                   |    yes    | working very poorly | ?                                                        |
| Matplotlib             |    yes    | not working         |                                                          |
| Emacs                  |    yes    | working             | ?                                                        |
| Atom                   |    no     | ?                   | ?                                                        |
| VSCodium               |    no     | ?                   | ?                                                        |
| IntelliJ               |    ?      | ?                   | ?                                                        |
| FreeCAD                |    ?      | ?                   | ?                                                        |
| Atom                   |    yes    | ?                   | ?                                                        |
| Chromium               |    yes    | ?                   | ?                                                        |


# Notes

- XWayland apps are responsible for handling HiDPI themselves (per default they will appear very small)
- Screen record: wf-recorder
- Screen shot: grim

- Firefox: MOZ_ENABLE_WAYLAND=1
- Chromium: --enable-features=UseOzonePlatform --ozone-platform=wayland
- Matplotlib / Qt5 on Wayland requires DISPLAY=":0" to be set
- Electron apps: --force-device-scale-factor=2
