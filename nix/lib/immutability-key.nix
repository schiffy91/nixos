{ lib }:
path: lib.replaceStrings [ "%2F" ] [ "!" ] (lib.strings.escapeURL (lib.removePrefix "/" path))
