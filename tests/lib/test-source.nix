# The source of a test program that is not a VM scenario: the files named, with btrc's testkit placed where
# the checkout has it (tests/testkit). `root` is this repository's root or its tests directory.
{ pkgs, inputs }: { name ? "test", root, fileset }: inputs.btrc.lib.withTestkit pkgs {
  inherit name;
  src = pkgs.lib.fileset.toSource { inherit root fileset; };
  at = pkgs.lib.removePrefix "/" (pkgs.lib.removePrefix (toString root) (toString ../testkit));
}
