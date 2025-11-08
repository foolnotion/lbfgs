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
          stdenv = pkgs.llvmPackages_20.stdenv;
        in
        rec {
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
              tl-expected
            ];
          };

          devShells.default = stdenv.mkDerivation {
            name = "lbfgs dev";

            nativeBuildInputs = packages.default.nativeBuildInputs ++ (with pkgs; [
              clang-tools
              cppcheck
              include-what-you-use
              cmake-language-server
            ]);

            buildInputs = packages.default.buildInputs ++ [ pkgs.libnano ]; 

            shellHook = ''
              alias bb="cmake --build build -j"
            '';
          };
        };
    };
}
