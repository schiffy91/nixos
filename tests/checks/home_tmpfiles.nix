{ lib }:
{ pkgs, system, configurations }:
let
  pathOf = rule: let fields = lib.splitString " " (lib.replaceStrings [ "\t" ] [ " " ] rule);
                 in lib.elemAt (lib.filter (field: field != "") fields) 1;
  typeOf = rule: let fields = lib.splitString " " rule;
                 in lib.head (lib.filter (field: field != "") fields);
  ancestorsUnder = home: path:
    let
      relative = lib.removePrefix "${home}/" path;
      parts = lib.init (lib.splitString "/" relative);
      step = state: part:
        let next = "${state.prefix}/${part}";
        in { prefix = next; paths = state.paths ++ [ next ]; };
    in (lib.foldl' step { prefix = home; paths = [ ]; } parts).paths;
  offenders = entry:
    let
      config = entry.configuration.config;
      homes = lib.mapAttrsToList (_: user: user.home)
        (lib.filterAttrs (_: user: user.isNormalUser or false) config.users.users);
      rules = config.systemd.tmpfiles.rules;
      declared = map pathOf rules;
      directoryRules = map pathOf (lib.filter (rule: typeOf rule == "d") rules);
      checkRule = rule:
        let
          path = pathOf rule;
          home = lib.findFirst (candidate: lib.hasPrefix "${candidate}/" path) null homes;
        in lib.optionals (home != null)
          (map (ancestor: "${entry.name}: tmpfiles rule for ${path} leaves ${ancestor} to be created as root; declare it with a d rule owned by the user")
            (lib.filter (ancestor: !(lib.elem ancestor directoryRules)) (ancestorsUnder home path)));
    in lib.concatMap checkRule rules;
  failures = lib.unique (lib.concatMap offenders configurations);
in {
  name = "home-tmpfiles";
  value = pkgs.runCommand "home-tmpfiles-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
