# pywm - backend for [newm](https://github.com/jbuchermn/newm)

pywm is an abstraction layer for [newm](https://github.com/jbuchermn/newm) encapsulating all c code.

Basically this is a very tiny compositor built on top of [wlroots](https://github.com/swaywm/wlroots), making all the assumptions that wlroots does not. On the Python side pywm exposes Wayland clients (XDG and XWayland) as so-called views and passes along all input. This way, handling the positioning of views, animating their movement, ... based on keystrokes or touchpad inputs (i.e. the logical, not performance-critical part of any compositor) is possible Python-side, whereas rendering and all other performance-critical aspects are handled by c code.

Check the Python class `PyWM` and c struct `wm_server` for a start, as well as newms `Layout`. 


## Status

See [TESTS.org](TESTS.org) for known issues and software with which pywm and newm have been tested. The goal here is to have (apart from XWayland) all software working on [sway](https://github.com/swaywm/sway) also working on pywm in a comparable manner - a lot of code dealing with special client-behaviour therefore is simply taken form sway.

## Installing

If you install [newm](https://github.com/jbuchermn/newm) via the AUR, pywm is installed automatically.

### Prerequisites

Prerequisites for PyWM, apart from Python, are given by [wlroots](https://github.com/swaywm/wlroots):

* python and pip
* gcc, meson and ninja
* pkg-config
* wayland
* wayland-protocols
* xorg-xwayland
* EGL
* GLESv2
* libdrm
* GBM
* libinput
* xkbcommon
* udev
* pixman
* libseat

### Install

Compilation is handled by meson and started automatically via pip:

```
pip3 install git+https://github.com/jbuchermn/pywm@v0.1
```

In case of issues, clone the repo and execute `meson build && ninja -C build` in order to debug.

### Troubleshooting

#### seatd

Be aware that current wlroots requires `seatd` - example systemd service (replace `<YourUser>` or handle via groups, see `man seatd`):

```
[Unit]
Description=Seat management daemon
Documentation=man:seatd(1)

[Service]
Type=simple
ExecStart=seatd -g _seatd
Restart=always
RestartSec=1

[Install]
WantedBy=multi-user.target
```


#### Touchpads

Ensure that your user is in the correct group

```
ls -al /dev/input/event*
```

As a sidenote, this is not necessary for a Wayland compositor in general as the devices can be accessed through `systemd-logind` or `seatd` or similar.
However the python `evdev` module does not allow instantiation given a file descriptor (only a path which it then opens itself),
so usage of that module would no longer be possible in this case (plus at first sight there is no easy way of getting that file descriptor to the 
Python side). Also `wlroots` (`libinput` in the backend) does not expose touchpads as what they are (`touch-down`, `touch-up`, `touch-motion` for any
number of parallel slots), but only as pointers (`motion` / `axis`), so gesture detection around `libinput`-events is not possible as well.

Therefore, we're stuck with the less secure (and a lot easier) way of using the (probably named `input`) group.

## Notes

- Depending on the settings (see newm) XWayland apps are responsible for handling HiDPI themselves and will per default appear very small
    - GDK on XWayland: GDK_DPI_SCALE=2
    - Electron apps: --force-device-scale-factor=2

- Screen record: wf-recorder
- Screen shot: grim
- Firefox: MOZ_ENABLE_WAYLAND=1
- Chromium: --enable-features=UseOzonePlatform --ozone-platform=wayland
- Matplotlib / Qt5 on Wayland requires DISPLAY=":0" to be set
- Apple Trackpad
    https://medium.com/macoclock/how-to-pair-apple-magic-keyboard-a1314-on-ubuntu-18-04-and-act-as-numpad-42fe4402454c
    https://wiki.archlinux.org/index.php/Bluetooth
