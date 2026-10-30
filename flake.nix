# a nix flake to build the jdm-alloc static c library.
# this library provides a memory allocator with the following features:
# - arena
# - stack
# - pool
# - free list
# - custom

{
  description = "A flake to build the jdm-alloc static c library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    jdm-defs.url = "github:jesseDMoore1994/jdm-defs";
    jdm-log.url = "github:jesseDMoore1994/jdm-log";
  };

  outputs = { 
    self,
    nixpkgs,
    jdm-defs,
    jdm-log
  }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
    jdm-defs-h = jdm-defs.packages.${system}.jdm-defs;
    jdm-log-lib = jdm-log.packages.${system}.jdm-log;
    jdm-alloc = pkgs.stdenv.mkDerivation {
      pname = "jdm-alloc";
      version = "1.0.0";

      src = ./.;

      buildInputs = [ pkgs.cmake jdm-defs-h jdm-log-lib ];

      meta = with pkgs.lib; {
        description = "Hand-rolled memory allocator library for C";
        license = licenses.mit;
        maintainers = with pkgs.maintainers; [ jesseDMoore1994 ];
      };
    };
  in {
    packages.x86_64-linux.jdm-alloc = with nixpkgs.legacyPackages.x86_64-linux; stdenv.mkDerivation {
      pname = "jdm-alloc";
      version = "1.0.0";

      src = ./.;

      buildInputs = [ cmake jdm-defs-h jdm-log-lib ];

      meta = with lib; {
        description = "Hand-rolled memory allocator library for C";
        license = licenses.mit;
        maintainers = with maintainers; [ jesseDMoore1994 ];
      };

      doCheck = true;
    };

    # setup a devShell for development
    devShells.x86_64-linux.default = with nixpkgs.legacyPackages.x86_64-linux; mkShell {
      buildInputs = [ clang-tools cmake gcc gdb valgrind jdm-defs-h jdm-log-lib ];
      shellHook = ''
        echo "Welcome to the jdm-alloc development shell!"
      '';
    };

    # set up defaultPackage to be the jdm-alloc package
    packages.x86_64-linux.default = self.packages.x86_64-linux.jdm-alloc;
  };
}
