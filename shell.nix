{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    xorg.libX11
    xorg.libXft
    xorg.libXinerama
    coreutils
  ];
}
