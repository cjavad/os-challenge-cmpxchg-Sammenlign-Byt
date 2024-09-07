{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  name = "os-challange";
  src = ./.;

  buildInputs = [
    pkgs.clang-tools
    pkgs.gnumake
    pkgs.gcc
  ];

  nativeBuildInputs = [
    pkgs.openssl
  ];

  installPhase = ''
    mkdir -p $out/bin
    cp server $out/bin
  '';
}
