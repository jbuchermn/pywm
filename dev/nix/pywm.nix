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
pkgs.python3.pkgs.buildPythonPackage rec {
  pname = "pywm";
  version = "0.2";

  src = ../..;

  nativeBuildInputs = with pkgs; [
    meson_0_60
    ninja
    pkg-config
    wayland-scanner
    glslang
  ];

  preBuild = "cd ..";

  buildInputs = with pkgs; [
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
    pkgs.mesa # Prevent clash with python module mesa

    libpng
    ffmpeg
    libcap

    python-with-my-packages
  ];
}
