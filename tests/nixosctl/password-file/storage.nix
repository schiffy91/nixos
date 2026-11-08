{ inputs, system }:
let
  inherit (inputs.nixpkgs) lib;
  valid = module:
    let
      config = (lib.nixosSystem {
        inherit system;
        modules = [
          ../../../config/settings.nix
          ../../../config/system/nixos/users/password.nix
          { _module.args.inputs = inputs; }
          module
        ];
      }).config;
      failures = lib.filter (entry: !entry.assertion && lib.hasInfix "settings.users.admin.hashedPasswordFile" entry.message) config.assertions;
    in failures == [];
in
assert valid {
  settings.disk.immutability.enable = true;
  fileSystems."/var" = { device = "none"; fsType = "tmpfs"; };
};
assert !(valid {
  settings.disk.immutability.enable = true;
  settings.users.admin.hashedPasswordFile = "/root/password-hash";
});
assert !(valid {
  settings.users.admin.hashedPasswordFile = "/srv/private/password-hash";
  fileSystems."/srv" = { device = "none"; fsType = "tmpfs"; };
});
assert !(valid {
  fileSystems."/var/lib/nixos/credentials/password-hash" = { device = "/fixture/hash"; fsType = "none"; options = [ "bind" ]; };
});
assert !(valid {
  fileSystems."/var/lib/nixos/credentials/password-hash" = { device = "/fixture/hash"; fsType = "none"; options = [ "bind" ]; neededForBoot = true; };
});
assert !(valid {
  fileSystems."/var" = { device = "/dev/missing"; fsType = "ext4"; options = [ "nofail" ]; };
});
assert !(valid {
  fileSystems."/var" = { device = "/dev/missing"; fsType = "ext4"; options = [ "noauto" ]; };
});
assert valid {
  settings.disk.immutability.enable = true;
  settings.users.admin.hashedPasswordFile = "/etc/private/password-hash";
  settings.disk.immutability.persist.neededForBoot = [ "/etc/private" ];
  fileSystems."/etc/private" = { device = "none"; fsType = "tmpfs"; neededForBoot = true; };
};
assert !(valid {
  settings.disk.immutability.enable = true;
  settings.disk.persistence.enable = false;
  settings.users.admin.hashedPasswordFile = "/etc/private/password-hash";
  settings.disk.immutability.persist.neededForBoot = [ "/etc/private" ];
});
true
