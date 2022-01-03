with import <nixpkgs> {};
with python39Packages;

buildPythonPackage rec {
  pname = "pywm";
  version = "0.2";

  src = ../..;

  nativeBuildInputs = [
    meson_0_60
    ninja
    pkg-config
    wayland-scanner
    glslang
  ];

  preBuild = "cd ..";

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
    pkgs.mesa # Prevent clash with python module mesa

    libpng
    ffmpeg
    libcap
  ];

  propagatedBuildInputs = [
    imageio
    numpy
    pycairo
    evdev
    matplotlib
  ];

  LC_ALL = "en_US.UTF-8";
}
