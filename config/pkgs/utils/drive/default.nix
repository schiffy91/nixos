# The Drive client is schiffy91/drive. This is what connects it to this configuration: the settings a host
# sets become the module's options, and what the client must keep goes to what an erased root keeps.
{ config, inputs, lib, ... }:
let settings = config.settings.apps.utils.drive;
in {
  imports = [ inputs.drive.nixosModules.default ];
  services.drive = {
    inherit (settings) enable mountPoint store pinned placements placementDevices cacheBytes freeFloorBytes readers ignore quietSeconds followSeconds push tray api;
    user = config.settings.users.admin.username;
    group = "users";  # the store's directory keeps the group it was made with
  };
  settings.disk.immutability.persist.paths = lib.mkIf settings.enable config.services.drive.statePaths;
}
