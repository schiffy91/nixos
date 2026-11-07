{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
in pkgs.testers.runNixOSTest {
  name = "boot-fallback";
  nodes.machine = { lib, ... }: {
    boot.loader.grub.enable = lib.mkForce false;
    boot.loader.systemd-boot = { enable = true; configurationLimit = 1; };
    boot.loader.efi.canTouchEfiVariables = true;
    environment.etc."generation-marker".text = lib.mkDefault "baseline";
    specialisation.updated.configuration.environment.etc."generation-marker".text = "updated";
    virtualisation.useBootLoader = true;
    virtualisation.useEFIBoot = true;
    virtualisation.memorySize = 1024;
  };
  testScript = ''
    import shlex
    start_all()
    machine.wait_for_unit("multi-user.target")
    baseline = machine.succeed("readlink -f /run/booted-system").strip()
    updated = machine.succeed("readlink -f /run/current-system/specialisation/updated").strip()
    machine.succeed("mkdir -p /nix/var/nix/profiles/system-profiles")
    machine.succeed(f"nix-env -p /nix/var/nix/profiles/system-profiles/boot-tested --set {shlex.quote(baseline)}")
    machine.succeed(f"nix-env -p /nix/var/nix/profiles/system --set {shlex.quote(updated)}")
    machine.succeed(f"{shlex.quote(updated)}/bin/switch-to-configuration boot")
    machine.shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target")
    assert machine.succeed("cat /etc/generation-marker").strip() == "updated"

    # Recreate an ordinary generation link for an old boot-tested system without
    # activating it, then return the selected profile to the current system.
    machine.succeed(f"nix-env -p /nix/var/nix/profiles/system --set {shlex.quote(baseline)}")
    machine.succeed(f"nix-env -p /nix/var/nix/profiles/system --set {shlex.quote(updated)}")
    assert machine.succeed("readlink -f /run/current-system").strip() == updated
    machine.succeed(f"{shlex.quote(updated)}/bin/switch-to-configuration boot")
    machine.succeed(f"test $(readlink -f /nix/var/nix/profiles/system) = {shlex.quote(updated)}")
    entry = machine.succeed(f"grep -l -F 'init={baseline}/init ' /boot/loader/entries/*.conf | head -n 1 | xargs basename").strip()
    assert entry
    machine.succeed(f"bootctl set-oneshot {shlex.quote(entry)}")
    machine.shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target")
    assert machine.succeed("cat /etc/generation-marker").strip() == "baseline"
    # A one-shot recovery choice must leave the updated generation as default.
    machine.shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target")
    assert machine.succeed("cat /etc/generation-marker").strip() == "updated"
  '';
}
