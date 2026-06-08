{ lib }:
path: lib.replaceStrings [ "/" ] [ "!" ] (lib.removePrefix "/" path)
