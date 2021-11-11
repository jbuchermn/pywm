# pywm - backend for [newm](https://github.com/jbuchermn/newm)

pywm is an abstraction layer for [newm](https://github.com/jbuchermn/newm) encapsulating all c code.

Basically this is a very tiny compositor built on top of [wlroots](https://github.com/swaywm/wlroots), making all the assumptions that wlroots does not. On the Python side pywm exposes Wayland clients (XDG and XWayland) as so-called views and passes along all input. This way, handling the positioning of views, animating their movement, ... based on keystrokes or touchpad inputs (i.e. the logical, not performance-critical part of any compositor) is possible Python-side, whereas rendering and all other performance-critical aspects are handled by c code.

Check the Python class `PyWM` and c struct `wm_server` for a start, as well as newms `Layout`. 


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
pip3 install git+https://github.com/jbuchermn/pywm
```

In case of issues, clone the repo and execute `meson build && ninja -C build` in order to debug.

### Configuration

Configuration is handled via key-value pairs given to the `PyWM` contructor:

| Key                             | Default | Description                                                                                                                                                                                                         |
|---------------------------------|---------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `output_scale`                  | `1.0`   | Number: HiDPI scale of output                                                                                                                                                                                       |
| `round_scale`                   | `1.0`   | Number: Round positions and widths finer than logical pixels on HiDPI screens (i.e. if set equal to `output_scale`, windows will be positioned according to actual pixels, if set to 1 according to logical pixels) |
| `xkb_model`                     |         | String: Keyboard model (`xkb`)                                                                                                                                                                                      |
| `xkb_layout`                    | `us`    | String: Keyboard layout (`xkb`)                                                                                                                                                                                     |
| `xkb_options`                   |         | String: Keyboard options (`xkb`)                                                                                                                                                                                    |
| `enable_xwayland`               | `False` | Boolean: Start `XWayland`                                                                                                                                                                                           |
| `enable_output_manager`         | `True`  | Boolean: Enable the wayland protocol `xdg_output_manager_v1`                                                                                                                                                        |
| `xcursor_theme`                 |         | String: `XCursor` theme                                                                                                                                                                                             |
| `xcursor_size`                  | `24`    | Integer: `XCursor` size                                                                                                                                                                                             |
| `output_name`                   | `""`    | String: If not "", pick the output (remember, only one supported at the moment) based on its name                                                                                                                   |
| `output_width`                  | `0`     | Integer: Output configuration, width (or zero to use preferred)                                                                                                                                                     |
| `output_height`                 | `0`     | Integer: Output configuration, height (or zero to use preferred)                                                                                                                                                    |
| `output_mHz`                    | `0`     | Integer: Output configuration, refresh rate in milli Hertz (or zero to use preferred)                                                                                                                               |
| `tap_to_click`                  | `True`  | Boolean: On tocuhpads use tap for click enter                                                                                                                                                                       |
| `natural_scroll`                | `True`  | Boolean: On touchpads use natural scrolling enter                                                                                                                                                                   |
| `focus_follows_mouse`           | `True`  | Boolean: `Focus` window upon mouse enter                                                                                                                                                                            |
| `contstrain_popups_to_toplevel` | `False` | Boolean: Try to keep popups contrained within their window                                                                                                                                                          |
| `encourage_csd`                 | `True`  | Boolean: Encourage clients to show client-side-decorations (see `wlr_server_decoration_manager`)                                                                                                                    |
| `debug_f1`                      | `False` | Boolean (Debug only): Output debug information to stdout on every F1 press                                                                                                                                          |


### Troubleshooting

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

#### seatd

Be aware that current wlroots requires `seatd` under certain circumstances. Example systemd service (replace `<YourUser>` or handle via groups, see `man seatd`):

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



## Status

See [TESTS.org](TESTS.org) for known issues and software with which pywm and newm have been tested. The goal here is to have (apart from XWayland) all software working on [sway](https://github.com/swaywm/sway) also working on pywm in a comparable manner - a lot of code dealing with special client-behaviour therefore is simply taken form sway.


