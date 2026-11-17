# A VM scenario of this repository: btrc's vmTest with the build tree laid out as the checkout is, so the path
# from a manifest to the testkit is the same in a build and in the working tree.
{ pkgs, inputs }: definition: inputs.btrc.lib.vmTest pkgs ({ root = ../..; testkit = "tests/testkit"; } // definition)
