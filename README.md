# PyWM - Python-based Wayland window manager

## Status

| Application            |  XWayland | Status              | Open issues                                              |
|------------------------|-----------|---------------------|----------------------------------------------------------|
| Termite                |    no     | working             |                none                                      |
| imv                    |    no     | working             |                none                                      |
| LibreOffice            |    no     | working             | Welcome Window is not display (properly)                 |
| Alacritty              |    no     | working             | Looks terrible                                           |
| Chromium               |    no     | working             | Weird behaviour entering search (related to full-screen YT?) |
| -                      |    -      | -                   | Too large popups are placed wrong (this is an issue with sway as well) |
| Firefox                |    no     | working             | "Save as" Dialog possibly scaled weirdly                 |
| Atom                   |    no     | ?                   | ?                                                        |
| VSCodium               |    no     | ?                   | ?                                                        |
| Matplotlib (Qt5)       |    no     | working             | Requires DISPLAY=":0" to be set?!                        |

| IntelliJ               |    ?      | ?                   | ?                                                        |
| GIMP                   |    ?      | ?                   | ?                                                        |
| OpenSCAD               |    ?      | ?                   | ?                                                        |
| FreeCAD                |    ?      | ?                   | ?                                                        |

| VSCodium               |    yes    | working             | No hidpi scaling                                         |
| Firefox                |    yes    | working             | "Save as" Dialog possibly scaled weirdly and unclickable |
| -                      |           |                     | No hidpi scaling                                         |
| Matplotlib             |    yes    | not working         |                                                          |
| Zoom                   |    yes    | working very poorly | ?                                                        |
| Atom                   |    yes    | ?                   | ?                                                        |
| Chromium               |    yes    | ?                   | ?                                                        |



## ToDo

- Use sorting for z-index logic
- Alacritty does not obtain information about the scale factor --> Blurry
- Cursor not displayed initially

- Clipboard
- Change cursor (in general + e.g. when over a link)

## Backlog

### General

- Various TODO Comments
- Popup constraints on scaling
- MBP keymap (incl Command+c / Command-v)
- Server-side decorations / Close window via shortcut (e.g. Chrome popups)
- Fullscreen mode on views (xdg-shell configure?) -> i.e. Chrome without client-side-decorations
  (wlr_xdg_toplevel_set_fullscreen)
- Blurry rendering during animations on rescale
- Login mechanism
- Damaging regions
- Screensharing using xdg-desktop-portal-wlr / Screenshots / -records

### XWayland

- setenv DISPLAY
- HiDPI scaling
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side

