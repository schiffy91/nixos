import os, sys, shlex, subprocess, getpass

def confirm(prompt):
    while True:
        r = input(prompt).lower()
        if r in ("y","yes"): return True
        if r in ("n","no"): return False
        print("Invalid input. Enter 'y' or 'n'.")

def extract_value(file_path, *, start_marker, end_marker, default_value, warning_message=None):
    try:
        with open(file_path, encoding="utf-8") as f:
            return f.read().split(start_marker)[1].split(end_marker)[0]
    except (FileNotFoundError, IndexError):
        if warning_message and confirm(f"WARNING: {warning_message}"):
            return default_value
        sys.exit(1)

def select_host():
    h = [x.replace(".nix","") for x in os.listdir("hosts") if x.endswith(".nix")]
    while True:
        for i, x in enumerate(h): print(f"{i+1}. {x}")
        try:
            c = int(input("> ")) - 1
            if 0 <= c < len(h): return h[c]
        except Exception: pass
        print("Invalid choice.")

def set_password():
    hp = extract_value("modules/users.nix",
                        start_marker='hashedPasswordFile = "',
                        end_marker='"',
                        default_value="/etc/nixos/secrets/hashed_password.txt",
                        warning_message="Failed to extract 'hashedPasswordFile' from modules/users.nix")
    pt = extract_value("modules/disk.nix",
                        start_marker='passwordFile = "',
                        end_marker='"',
                        default_value="/tmp/plain_text_password.txt",
                        warning_message="Failed to extract 'passwordFile' from modules/disk.nix")
    while True:
        p = getpass.getpass("Set your password: ")
        if p == getpass.getpass("Confirm your password: "): break
        print("Passwords do not match.")
    os.makedirs("/tmp", exist_ok=True)
    os.makedirs("/etc/nixos/secrets", exist_ok=True)
    with open(pt,"w",encoding="utf-8") as f: f.write(p)
    r = execute_command(f"mkpasswd -m sha-512 {p}",
                        error_message="Failed to write a SHA-512 of your password.",
                        capture_output=True,
                        text=True)
    with open(hp,"w",encoding="utf-8") as f: f.write(r.stdout)

def execute_command(cmd, error_message=None, warning_message=None, **kwargs):
    try:
        return subprocess.run(shlex.split(f"sudo {cmd}"), check=True, **kwargs)
    except Exception as e:
        if warning_message:
            print(f"WARNING: {warning_message}")
            if confirm("Continue anyway? (y/n): "):
                return False
            abort_installation()
        if error_message:
            print(f"ERROR: {error_message}\n{e}")
            sys.exit(1)

def disko_operation(host, mode):
    v = extract_value(  "flake.nix",
                        start_marker="github:nix-community/disko/",
                        end_marker='";',
                        default_value="latest",
                        warning_message="Failed to extract the disko version from flake.nix")
    c = f"nix --extra-experimental-features nix-command --extra-experimental-features flakes run github:nix-community/disko/{v} -- --flake .#{host}-DISKO --mode {mode}"
    if "format" in mode or "destroy" in mode: c += " --yes-wipe-all-disks"
    execute_command(c, error_message=f"Failed disko operation: {mode}.")

def abort_installation():
    print("Installation aborted.")
    sys.exit(1)

def main():
    if os.geteuid() != 0:
        print("ERROR: Please run this script with sudo.")
        sys.exit(1)
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    host = select_host()
    set_password()
    disk = extract_value(   f"hosts/{host}.nix",
                            start_marker='diskOverrides.device = "',
                            end_marker='";',
                            default_value="UNKNOWN",
                            warning_message=f"Failed to extract 'diskOverrides.device' from hosts/{host}.nix")
    if confirm(f"Format {disk}? (y/n): "):
        disko_operation(host,"destroy,format,mount")
    else:
        print("Skipping formatting.")
        print("Mounting disk.")
        disko_operation(host,"mount")
    if confirm("Install nixos? /mnt/etc/nixos will be deleted (y/n): "):
        print("Removing /mnt/etc/nixos.")
        for mnt_dir in ["/mnt/etc", "/mnt/nix/store", "/mnt/nix/tmp"]:
            execute_command(f"mkdir -p {mnt_dir}", error_message="Failed to create directory {mnt_dir}")
        print("Copying files.")
        execute_command("rm -rf /mnt/etc/nixos", error_message="Failed to delete /mnt/etc/nixos.")
        execute_command("cp -r /etc/nixos /mnt/etc/nixos", error_message="Failed to copy /etc/nixos to /mnt/etc/nixos.")
        print(f"Installing {host} to {disk}.")
        c = f"TMPDIR=/mnt/nix/tmp nixos-install --flake /mnt/etc/nixos#{host} --no-bootloader --root /mnt --no-channel-copy --show-trace --no-root-password"
        execute_command(c, error_message="Failed to install nixos.")
        execute_command("rm -rf /mnt/nix/tmp", error_message="Failed to remove /mnt/nix/tmp.")
        print(f"Installed {host} to {disk}.")
    if confirm("Create Secure Boot keys? (y/n): "):
        pki_bundle = extract_value("modules/boot.nix",
                                    start_marker="pkiBundle = \"",
                                    end_marker="\";",
                                    default_value="/etc/secureboot",
                                    warning_message="Failed to extract 'pkiBundle' from modules/boot.nix")
        print("Creating secure boot keys inside the target system")
        if  execute_command("nixos-enter", warning_message="Failed to enter nixos.") is not False and \
            execute_command(f"mkdir -p {pki_bundle}", warning_message=f"Failed to make {pki_bundle}.") is not False:
                execute_command(f"nix run nixpkgs#sbctl -- create-keys --export {pki_bundle}", warning_message="Failed to run sbctl")
        print (f"Created Secure Boot Keys at {pki_bundle}")
        execute_command(c, "Secure Boot keys failed.", "Secure Boot keys failed.")
        if confirm("Enroll TPM2? (y/n): "):
            tpm_available = execute_command("[ -f /sys/class/tpm/tpm0/device/tpm_version_major ] && [ \"$(cat /sys/class/tpm/tpm0/device/tpm_version_major)\" -eq 2 ]", warning_message="TPM2 is unavailable")
            if tpm_available is not False and tpm_available != "":
                tpm_enroll = execute_command(f"systemd-cryptenroll --tpm2-device=auto --tpm2-pcrs=0+7 {disk}", warning_message="Failed to enroll TPM2.0")
                if tpm_enroll is not False and tpm_enroll != "":
                    print("Enrolled TPM2")
            else:
                print("TPM2 is unavailable")
    if confirm("Restart now? (y/n): "):
        execute_command("shutdown -r now", error_message="Shutdown failed.")
if __name__=="__main__":
    main()