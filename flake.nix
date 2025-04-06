{
  description = "lbfgs";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:nixos/nixpkgs/master";
    foolnotion.url = "github:foolnotion/nur-pkg";
    foolnotion.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = inputs@{ self, flake-parts, nixpkgs, foolnotion }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" "aarch64-darwin" ];

      perSystem = { pkgs, system, ... }:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [ foolnotion.overlay ];
          };
          stdenv = pkgs.llvmPackages_18.stdenv;
        in
        rec {
          #_module.args.pkgs = pkgs;

          packages.default = stdenv.mkDerivation {
            name = "lbfgs";
            src = self;

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DCMAKE_CXX_FLAGS=${
                if pkgs.stdenv.hostPlatform.isx86_64 then "-march=x86-64" else ""
              }"
            ];
            nativeBuildInputs = with pkgs; [ cmake ];

            buildInputs = with pkgs; [
              eigen
              ned14-outcome
              ned14-quickcpplib
              ned14-status-code
            ];
          };

          devShells.default = stdenv.mkDerivation {
            name = "lbfgs dev";

            nativeBuildInputs = packages.default.nativeBuildInputs ++ (with pkgs; [
              clang-tools_18
              cppcheck
              include-what-you-use
              cmake-language-server
            ]);

            buildInputs = packages.default.buildInputs ++ (with pkgs; [
              gdb
              linuxPackages_latest.perf
              valgrind
            ]) ++ (with pkgs; if pkgs.stdenv.isx86_64 then [ libnano ] else []);

            shellHook = ''
              alias bb="cmake --build build -j"
            '';
          };
        };
    };
}
