{ pkgs }:
let
  my-python = pkgs.python3;
  python-with-my-packages = my-python.withPackages (p: with p; [
    imageio
    numpy
    pycairo
    evdev
    matplotlib
  ]);
in
with pkgs;
mkShell {
  nativeBuildInputs = [
    meson_0_60
    ninja
    pkg-config
    wayland-scanner
    glslang
  ];

  buildInputs = [
    libGL
    wayland
    wayland-protocols
    libinput
    libxkbcommon
    pixman
    xorg.xcbutilwm
    xorg.xcbutilrenderutil
    xorg.xcbutilerrors
    xorg.xcbutilimage
    xorg.libX11
    seatd
    xwayland
    vulkan-loader
    mesa

    libpng
    ffmpeg
    libcap

    python-with-my-packages
  ];
}
