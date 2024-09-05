{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  name = "os-challange";

  buildInputs = [
    pkgs.clang-tools
    pkgs.gnumake
    pkgs.gcc
  ];
}
