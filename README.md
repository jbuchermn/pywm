# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status              | Open issues                                                   |
|------------------------|-----------|---------------------|---------------------------------------------------------------|
| Termite                |    no     | working             |                none                                           |
| imv                    |    no     | working             |                none                                           |
| LibreOffice            |    no     | working             | Welcome Window is not display (properly)                      |
| Alacritty              |    no     | working             | Looks terrible                                                |
| Chromium               |    no     | working             | Menus outside view / weird behaviour entering search term (related to full-screen YT?)     |
| Firefox                |    no     | working             | "Save as" Dialog possibly scaled weirdly                      |
| Atom                   |    no     | ?                   | ?                                                             |
| VSCodium               |    no     | ?                   | ?                                                             |
| Matplotlib (Qt5)       |    no     | working             | Requires DISPLAY=":0" to be set?!                             |

| IntelliJ               |    ?      | ?                   | ?                                                             |
| GIMP                   |    ?      | ?                   | ?                                                             |
| OpenSCAD               |    ?      | ?                   | ?                                                             |
| FreeCAD                |    ?      | ?                   | ?                                                             |

| VSCodium               |    yes    | working             | no hidpi scaling                                              |
| Firefox                |    yes    | working             | "Save as" Dialog possibly scaled weirdly / no hidpi scaling   |
| Matplotlib             |    yes    | not working         |                                                               |
| Zoom                   |    yes    | working very poorly | ?                                                             |
| Atom                   |    yes    | ?                   | ?                                                             |
| Chromium               |    yes    | ?                   | ?                                                             |


## ToDo

- Clipboard
- Use sorting fpr z-index logic

- Alacritty does not obtain information about the scale factor --> Blurry
- Have menus render inside parent boundaries (xdg)

- Cursor not displayed initially
- Change cursor (in general + e.g. when over a link)

## Backlog

### General

- Various TODO Comments
- MBP keymap (incl Command+c / Command-v)
- Server-side decorations / Close window via shortcut (e.g. Chrome popups)
- Blurry rendering during animations on rescale
- Login mechanism
- Damaging regions
- Screensharing using xdg-desktop-portal-wlr

### XWayland

- setenv DISPLAY
- HiDPI scaling
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side

