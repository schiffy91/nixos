{ lib }:
let
  # These are valid off configurations. Contradictory parent/child settings
  # are tested separately and must fail rather than silently disable a child.
  disable = tree: lib.filterAttrs (_: value: value != {}) (lib.mapAttrs (name: value:
    if name == "enable" && builtins.isBool value then lib.mkForce false
    else if builtins.isAttrs value && !lib.isDerivation value then disable value
    else {}
  ) tree);
in disable
