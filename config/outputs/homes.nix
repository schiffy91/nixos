{ lib, hosts, systems }:
lib.listToAttrs (map (host:
  let
    darwin = systems.darwinConfigurations.${host.name};
    username = darwin.config.settings.users.admin.username;
    home = darwin.config.home-manager.users.${username};
    failures = map (entry: entry.message) (lib.filter (entry: !entry.assertion) (darwin.config.assertions ++ home.assertions));
    checked = assert lib.assertMsg (failures == [ ]) (lib.concatStringsSep "\n" failures); home;
  in {
    name = "${username}@${host.name}";
    value = {
      inherit (darwin) pkgs;
      config = checked;
      activationPackage = checked.home.activationPackage;
      newsDisplay = checked.news.display;
      newsEntries = lib.sort (a: b: a.time > b.time) (lib.filter (entry: entry.condition) checked.news.entries);
    };
  }) hosts.darwin)
