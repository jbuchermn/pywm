# Status

### termite, gnome-mpv, gnome-calculator
- No apparent issues


### GIMP (XWayland)
- Appears to be usable, however, not checked in detail

### LibreOffice
- Subsurface tooltips accept input which kills drag-to-autofill (calc) when hovering over tooltips. IMHO that's a LibreOffice issue

### Firefox, Chromium (XWayland)
- Usable
- Weird behaviour when "Save as" dialog is too large, can only click "Cancel" not "Save"
- Mouse clicks missing in fullscreen YT, seems to be related to scaling
- Opening a second Chrome window does not work 

### IntelliJ (XWayland)
- Usable

## Matplotlib (XWayland)
- Basically usable
- However tooltips appear as extra views, which can possibly break the wm (High frequency animating around), but is extremely annoying anyways


# ToDo

### XWayland
- Check that it works: Detect modals as child-views (analogously to child-toplevels)
- Views are allowed to configure in any way they want (-> lots of overlapping modals and similar shit)
- Certain welcome windows (OpenSCAD, FreeCAD) are not displayed at all. In case of OpenSCAD, a wlr_surface is registered, but no view ever reaches the python side

### General
- Views change their minimal width/height after initial set; this is ignored leading to ugly scaling
- Cursor not displayed initially (Open termite, fullscreen, open chromium, move cursor.. nothing there)
- Natural Scrolling
