{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  name = "os-challange";
  src = ./.;

  buildInputs = [
    pkgs.clang-tools_19
    pkgs.gnumake
    pkgs.gcc
    pkgs.liburing
  ];

  nativeBuildInputs = [
    pkgs.openssl
  ];

  installPhase = ''
    mkdir -p $out/bin
    cp server $out/bin
  '';
}
