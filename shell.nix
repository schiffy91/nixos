# Reproducible dev shell: nixpkgs is pinned to flake.lock rather than the ambient
# <nixpkgs> channel, so `nix-shell` / direnv resolve the same nixpkgs the flake
# builds against regardless of the host's registry.
{ pkgs ? import
    (let node = (builtins.fromJSON (builtins.readFile ./flake.lock)).nodes.nixpkgs.locked;
     in builtins.fetchTree { inherit (node) type owner repo rev narHash; })
    { config.allowUnfree = true; }
}:
pkgs.mkShell {
	packages = with pkgs; [
		(python315.withPackages (ps: [ ps.pytest ]))
		gnumake
		nixd
		git
	];
}
