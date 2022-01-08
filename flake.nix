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
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      packages.pywm = (
        let
          a = 1;
        in (
          pkgs.python3.pkgs.buildPythonPackage rec {
            pname = "pywm";
            version = "0.3alpha";

            # BEGIN Fucking subprojects bug workaround for 'src = ./.'
            srcs = [
              ./.
              (builtins.fetchGit {
                url = "https://gitlab.freedesktop.org/wlroots/wlroots";
                rev = "1fbd13ec799c472558aef37436367f0e947f7d89";
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
              mesa

              libpng
              ffmpeg
              libcap
            ];

            propagatedBuildInputs = with pkgs.python3Packages; [
              imageio
              numpy
              pycairo
              evdev
              matplotlib
            ];
          }
        )
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
          pylsp-mypy
          mypy
        ]);
      in
        pkgs.mkShell {
          nativeBuildInputs = with pkgs; [ 
            meson_0_60
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
        };
    }
  );
}
