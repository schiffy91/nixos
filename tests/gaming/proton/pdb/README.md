# CSP symbol enumeration regression

This fixture exercises Wine's Windows `dbghelp` API with a generated PDB:
two functions folded to the same address, a public symbol without CodeView
records, wildcard filtering, and callback cancellation. The tiny C inputs
produce the Windows ABI/debug records under test; the application and launcher
tests remain BTRC in `tests/gaming/GamingTests.btrc`.

Run from `/etc/nixos` in a desktop session (uses an isolated temporary prefix):

```sh
fixture=$(nix build --impure --no-link --print-out-paths --expr '
  let flake = builtins.getFlake "git+file:///etc/nixos";
      pkgs = flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.pkgs;
  in import /etc/nixos/tests/gaming/proton/pdb { inherit pkgs; }')
proton=$(nix build --impure --no-link --print-out-paths --expr '
  let flake = builtins.getFlake "git+file:///etc/nixos";
      pkgs = flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.pkgs;
  in pkgs.callPackage /etc/nixos/nix/apps/gaming/proton/custom/package.nix {}')
test_prefix=$(mktemp -d /tmp/ac-pdb-test.XXXXXX)
cd /tmp
WINEPREFIX="$test_prefix/pfx" WINEARCH=win64 WINEDEBUG=-all \
  WINE_DBGHELP_OLD_PDB=1 steam-run "$proton/files/bin/wine" \
  "$fixture/PdbSymbols.exe" "Z:$fixture/Folded.dll"
```

Expected: `PASS` and exit 0. With GE-Proton11-7, omitting
`WINE_DBGHELP_OLD_PDB` reproduces missing symbols and exit 5. The compatible
reader passes, as does live CSP 0.2.11 initialization on the same Proton.
Recheck this when updating Proton; do not replace the shared tool with an
older release merely to recover its previous PDB reader.
