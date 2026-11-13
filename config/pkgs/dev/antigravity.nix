{ config, lib, pkgs, ... }:
let home = config.settings.users.admin.homeDirectory;
in lib.mkIf config.settings.apps.dev.antigravity.enable {
  environment.systemPackages = with pkgs; [ antigravity-hub antigravity-cli ]; # the desktop app and `agy`
  settings.disk.immutability.persist.paths = [
    "${home}/.config/Antigravity"  # the app's window and UI state
    "${home}/.config/antigravity"  # which keyring entry `agy` signs in with
  ]; # conversations and settings live in ~/.gemini, kept with the agents
}
