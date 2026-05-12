{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
	packages = with pkgs; [
		(python315.withPackages (ps: [ ps.pytest ]))
		nixd
	];
}
