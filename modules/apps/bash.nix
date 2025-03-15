{ ... }: {
  programs.bash = {
    enable = true;
    bashrcExtra = 
''
nix-shell-unstable() {
  local pkgs_list=""
  for pkg in "$@"; do
    if [ -z "$pkgs_list" ]; then
      pkgs_list="unstable.$pkg"
    else
      pkgs_list="$pkgs_list unstable.$pkg"
    fi
  done
  tmp_shell=/tmp/nix-shell-unstable.nix
  cat > "$tmp_shell" << EOF
{ pkgs ? import <nixpkgs> {} }:
let unstable = import <nixos-unstable> {}; in
pkgs.mkShell { nativeBuildInputs = [ $pkgs_list ]; }
EOF
  nix-shell "$tmp_shell"
}
'';
  };
}