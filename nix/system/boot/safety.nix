{ config, lib, pkgs, ... }:
let
  guard = pkgs.writeShellScript "check-boot-policy" ''
    set -euo pipefail
    if [ ! -d /sys/firmware/efi ]; then exit 0; fi
    variable=/sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c
    # Linux treats EFI_NOT_FOUND as disabled. Require mounted efivarfs so an
    # unavailable variable filesystem is not mistaken for unsupported firmware.
    if [ ! -e "$variable" ] && [ "$(${pkgs.coreutils}/bin/stat -f -c %T /sys/firmware/efi/efivars)" = efivarfs ]; then
      exit 0
    fi
    if [ "$(${pkgs.coreutils}/bin/stat -c %s "$variable")" != 5 ]; then
      echo "Cannot determine firmware Secure Boot state; refusing unsigned boot files." >&2
      exit 1
    fi
    state=$(${pkgs.coreutils}/bin/od -An -tu1 -j4 -N1 "$variable" | ${pkgs.coreutils}/bin/tr -d '[:space:]')
    case "$state" in
      1) echo "Refusing Standard-Boot while firmware Secure Boot is enabled. Use the Secure-Boot target." >&2; exit 1 ;;
      0) exit 0 ;;
      *) echo "Cannot determine firmware Secure Boot state; refusing unsigned boot files." >&2; exit 1 ;;
    esac
  '';
  standard = config.settings.boot.method == "Standard-Boot";
in {
  config._module.args.bootSafety = guard;
  # Also guard the installer itself: nixos-install and NIXOS_NO_CHECK bypass
  # pre-switch checks. This runs before the first write to either ESP.
  options.system.build.installBootLoader = lib.mkOption {
    apply = installer: if standard then pkgs.writeShellScript "install-bootloader-guarded" ''
      set -eu
      ${guard}
      exec ${installer} "$@"
    '' else installer;
  };
  config.system.preSwitchChecks = lib.mkIf standard {
    secureBoot = "${guard}";
  };
}
