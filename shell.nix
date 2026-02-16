{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
	packages = with pkgs; [
		(python314.withPackages (ps: [ ps.pytest ]))
		nixd
		sbctl
	];
}
