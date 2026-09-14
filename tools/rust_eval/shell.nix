# © 2026 Massachusetts Institute of Technology
# MIT License

let
  pkgs = import <nixpkgs> {};
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    python313
    python313Packages.xmltodict
    openssl
    pkg-config
    pcre2
    zlib
  ];

  shellHook = ''
    echo "Entered python shell"
  '';
}
