# The tool's library as every installed program takes it: without the VM lab (lib/simulator, a package of
# its own that lab.nix builds). An edit to the test harness then changes no system, and the scenarios
# keyed on those systems keep their recorded results.
{ lib }: lib.fileset.difference ./lib ./lib/simulator
