{
  description = "pywm - Wayland compositor core";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
  flake-utils.lib.eachDefaultSystem (
    system:
    let
      pkgs = import nixpkgs {
        inherit system;
      };
      has_xwayland = true;
      inherit (pkgs.lib) optionals;
    in
    {
      packages.pywm = (
        pkgs.python3.pkgs.buildPythonPackage rec {
          pname = "pywm";
          version = "0.3alpha";

          # BEGIN Fucking subprojects bug workaround for 'src = ./.'
          srcs = [
            ./.
            (builtins.fetchGit {
              url = "https://gitlab.freedesktop.org/wlroots/wlroots";
              rev = "e279266f714c7122e9ad97d56d047313f78cfdbe";
              submodules = true;
            })
          ];

          setSourceRoot = ''
            for i in ./*; do
              if [ -f "$i/wlroots.syms" ]; then
                wlrootsDir=$i
              fi
              if [ -f "$i/pywm/pywm.py" ]; then
                sourceRoot=$i
              fi
            done
          '';

          preConfigure = ''
            echo "--- Pre-configure --------------"
            echo "  wlrootsDir=$wlrootsDir"
            echo "  sourceRoot=$sourceRoot"
            echo "--- ls -------------------------"
            ls -al
            echo "--- ls ../wlrootsDir -----------"
            ls -al ../$wlrootsDir

            rm -rf ./subprojects/wlroots 2> /dev/null
            cp -r ../$wlrootsDir ./subprojects/wlroots
            rm -rf ./build

            echo "--- ls ./subprojects/wlroots ---"
            ls -al ./subprojects/wlroots/
            echo "--------------------------------"
          '';
          # END Fucking subprojects bug workaround

          mesonFlags = if has_xwayland then [ "-Dxwayland=enabled" ] else [];

          nativeBuildInputs = with pkgs; [
            meson
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
            vulkan-loader
            mesa
            seatd

            libpng
            ffmpeg
            libcap

            xorg.xcbutilwm
            xorg.xcbutilrenderutil
            xorg.xcbutilerrors
            xorg.xcbutilimage
            xorg.libX11
          ] ++ optionals has_xwayland [
            xwayland
          ];

          propagatedBuildInputs = with pkgs.python3Packages; [
            imageio
            numpy
            pycairo
            evdev
          ];
        }
      );

      devShell = let
        my-python = pkgs.python3;
        python-with-my-packages = my-python.withPackages (ps: with ps; [
          imageio
          numpy
          pycairo
          evdev
          matplotlib

          python-lsp-server
          (pylsp-mypy.overrideAttrs (old: { pytestCheckPhase = "true"; }))
          mypy
        ]);
      in
        pkgs.mkShell {
          nativeBuildInputs = with pkgs; [ 
            meson
            ninja
            pkg-config
            wayland-scanner
            glslang
          ];

          buildInputs = with pkgs; [ 
            libGL
            wayland
            wayland-protocols
            libinput
            libxkbcommon
            pixman
            seatd
            vulkan-loader
            mesa

            libpng
            ffmpeg
            libcap
            python-with-my-packages 

            xorg.xcbutilwm
            xorg.xcbutilrenderutil
            xorg.xcbutilerrors
            xorg.xcbutilimage
            xorg.libX11
            xwayland
          ];

        };
    }
  );
}
