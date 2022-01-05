{
  description = "pywm - Wayland compositor core";

  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
  flake-utils.lib.eachDefaultSystem (
    system:
    let
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      devShell = import ./dev/nix/shell.nix { inherit pkgs; };
      packages.pywm = import ./dev/nix/pywm.nix { inherit pkgs; };
    }
  );
}
