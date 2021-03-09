# PyWM - Python-based Wayland window manager

Documentation to come

## Status

| Application            |  XWayland | Status              | Open issues                                              |
|------------------------|-----------|---------------------|----------------------------------------------------------|
| Termite                |           | working             |                none                                      |
| imv                    |           | working             |                none                                      |
| LibreOffice            |           | working             |                none                                      |
| Alacritty              |           | working             |                none                                      |
| Firefox                |           | working             |                none                                      |
| Matplotlib (Qt5)       |           | working             |                none                                      |
| GIMP-2.99              |           | working             |                none                                      |
| nemo                   |           | working             |                none                                      |
| Nautilus               |           | working             |                none                                      |
| masm11/Emacs           |           | working             |                none                                      |
| Chromium               |           | working             | Too large popups are placed wrong (this is an issue with sway as well) |
| OpenSCAD               |           | working             |                none                                      |
| VLC                    |           | working             | Popup-toplevels are not recognized as floating           |
| mpv                    |           | working             |                none                                      |
| Zoom                   |           | working             | Popup-toplevels are not recognized as floating           |
| Atom                   |           | ?                   | ?                                                        |
| VSCodium               |           | ?                   | ?                                                        |
| IntelliJ               |    ?      | ?                   | ?                                                        |
| FreeCAD                |    ?      | ?                   | ?                                                        |
| Spotify                |    yes    | working             |                none                                      |
| VSCodium               |    yes    | working             |                none                                      |
| Atom                   |    yes    | working             |                none                                      |
| VLC                    |    yes    | working             |                none                                      |
| Firefox                |    yes    | working             |                none                                      |
| Zoom                   |    yes    | working             | Does not look very nice                                  |
| Matplotlib             |    yes    | not working         |                                                          |
| Emacs                  |    yes    | working             | ?                                                        |
| Atom                   |    yes    | ?                   | ?                                                        |
| Chromium               |    yes    | ?                   | ?                                                        |


## Installing

```
sudo pip3 install -v git+https://github.com/jbuchermn/pywm
```

## Notes

- Depending on the settings (see main.py) XWayland apps are responsible for handling HiDPI themselves and will per default appear very small
    - GDK on XWayland: GDK_DPI_SCALE=2
    - Electron apps: --force-device-scale-factor=2

- Screen record: wf-recorder
- Screen shot: grim
- Firefox: MOZ_ENABLE_WAYLAND=1
- Chromium: --enable-features=UseOzonePlatform --ozone-platform=wayland
- Matplotlib / Qt5 on Wayland requires DISPLAY=":0" to be set
