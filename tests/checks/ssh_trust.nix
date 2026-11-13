{ lib, hosts, systems }:
{ pkgs }:
let
  linux = map (host: {
    name = hosts.hostOutputName host.module;
    inherit (systems.mkSystem host.module (lib.head systems.bootableTargets) systems.systemModuleFiles) config;
  }) hosts.physical;
  darwin = map (host: { inherit (host) name; inherit (systems.darwinConfigurations.${host.name}) config; }) hosts.darwin;
  trusted = host: host.config.settings.users.admin.authorizedKeys;
  installed = host: host.config.users.users.${host.config.settings.users.admin.username}.openssh.authorizedKeys.keys;
  reference = trusted (lib.head linux);
  sshdConfig = host: host.config.environment.etc."ssh/sshd_config.d/100-nix-darwin.conf".text;
  failures =
    lib.optional (lib.length reference != 1) "the admin trusts ${toString (lib.length reference)} SSH keys instead of the one in 1Password"
    ++ lib.concatMap (host:
      lib.optional (trusted host != reference) "${host.name}: trusted SSH keys differ from the other hosts"
      ++ lib.optional (installed host != reference) "${host.name}: the admin account's authorized keys are not the declared ones") (linux ++ darwin)
    ++ lib.concatMap (host:
      lib.optional (host.config.services.openssh.settings.PasswordAuthentication != false) "${host.name}: sshd accepts passwords") linux
    ++ lib.concatMap (host:
      lib.optional (host.config.services.openssh.enable != true) "${host.name}: Remote Login is not declared"
      ++ lib.optional (!(lib.hasInfix "PasswordAuthentication no" (sshdConfig host) && lib.hasInfix "KbdInteractiveAuthentication no" (sshdConfig host))) "${host.name}: sshd accepts passwords"
      ++ lib.optional (!(lib.hasInfix "IdentityAgent \"~/Library/Group Containers/2BUA8C4S2C.com.1password/t/agent.sock\"" host.config.programs.ssh.extraConfig)) "${host.name}: ssh does not use the 1Password agent"
      ++ lib.optional (!(host.config.launchd.user.agents ? onepassword-ssh-auth-sock)) "${host.name}: SSH_AUTH_SOCK is not pointed at the 1Password agent") darwin;
in {
  name = "ssh-trust";
  value = pkgs.runCommand "ssh-trust" { } (
    if failures == [ ] then "touch $out"
    else throw (lib.concatStringsSep "\n" failures));
}
