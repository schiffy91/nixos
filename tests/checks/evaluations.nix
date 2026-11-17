# What the flake's outputs must evaluate to. An expectation names an output and text its value must
# contain as `nix eval` prints it; an assertion is anything else that has to hold. The VM graph used to
# ask these one `nix eval` at a time; here one evaluation answers them all and fails naming each miss.
{ lib, outputs }:
{ pkgs, name, expectations, assertions ? [ ] }:
let
  printed = lib.generators.toPretty { multiline = false; };
  missed = expectation:
    let
      value = lib.attrByPath (lib.splitString "." expectation.attr)
        (throw "${name}: no output ${expectation.attr}") outputs;
      text = builtins.unsafeDiscardStringContext (printed value);
    in lib.optional (!lib.hasInfix expectation.expect text)
      "${expectation.attr}: expected ${expectation.expect}, is ${text}";
  failures = lib.concatMap missed expectations
    ++ map (assertion: assertion.message) (lib.filter (assertion: !assertion.holds) assertions);
in {
  inherit name;
  value = if failures == [ ] then pkgs.runCommand name { } "touch $out"
    else throw "${name}:\n${lib.concatStringsSep "\n" failures}";
}
