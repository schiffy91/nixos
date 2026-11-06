{ system ? builtins.currentSystem, rotate ? false }:
import ../plymouth-keyboard { inherit system rotate; }
