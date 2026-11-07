{ system ? builtins.currentSystem, rotate ? false, layout ? "us", light ? false, multi ? false, scale ? 1 }:
import ../plymouth-keyboard { inherit system rotate layout light multi scale; }
