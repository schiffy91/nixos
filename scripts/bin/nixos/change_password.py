#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import getpass, os, pty, pwd, select, shlex, sys, time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from lib import Config, Shell, Utils

sh = Shell(root_required=True)

def ask_for_old_password():
    return getpass.getpass("Enter current password (LUKS + account): ")

def ask_for_new_password():
    while True:
        password = getpass.getpass("Enter new password: ")
        if password == getpass.getpass("Confirm new password: "):
            return password
        Utils.log_error("Passwords do not match.")

def change_luks_password(old_password, new_password):
    Utils.log("Changing LUKS encryption password...")
    root = Config.get_disk_by_part_label_root()
    result = sh.run(
        f"printf '%s\\n%s' {shlex.quote(old_password)} {shlex.quote(new_password)}"
        f" | cryptsetup luksChangeKey {root}",
        sensitive=[old_password, new_password], check=False)
    if result.returncode != 0:
        Utils.abort("Failed to change LUKS password.")

def change_account_password_via_pam(user, old_password, new_password):
    Utils.log(f"Changing account password for '{user}' (re-encrypts KDE Wallet via PAM)...")
    pw = pwd.getpwnam(user)
    pid, fd = pty.fork()
    if pid == 0:
        os.setgid(pw.pw_gid)
        os.setuid(pw.pw_uid)
        os.environ.update(HOME=pw.pw_dir, USER=user, LOGNAME=user, PATH="/run/wrappers/bin:/run/current-system/sw/bin:/usr/bin:/bin")
        os.execvp("passwd", ["passwd"])
    responses = iter([old_password, new_password, new_password])
    buf, deadline = b"", time.time() + 30
    status = 0
    try:
        while time.time() < deadline:
            r, _, _ = select.select([fd], [], [], 5)
            if not r: continue
            try: data = os.read(fd, 4096)
            except OSError: break
            if not data: break
            buf += data
            if buf.rstrip(b" \t").endswith(b":"):
                try: response = next(responses)
                except StopIteration: break
                os.write(fd, (response + "\n").encode())
                buf = b""
                time.sleep(0.2)
        _, status = os.waitpid(pid, 0)
    finally:
        try: os.close(fd)
        except OSError: pass
    if not os.WIFEXITED(status) or os.WEXITSTATUS(status) != 0:
        Utils.log_error("passwd returned non-zero; KDE Wallet may not be re-encrypted.")
        Utils.log_error("You can re-run kwalletmanager6 to update the wallet password manually.")

def sync_hashed_password_file(new_password):
    Utils.log("Mirroring new hash to the declarative password file...")
    hashed = Shell.stdout(sh.run(
        f"mkpasswd -m sha-512 {shlex.quote(new_password)}",
        sensitive=new_password))
    sh.file_write(Config.get_hashed_password_path(), hashed, sensitive=hashed)

def update_tpm2():
    Utils.log("Re-enrolling TPM2 with new LUKS password...")
    result = sh.run("nixos tpm2 enable", capture_output=False, check=False)
    if result.returncode != 0: Utils.log_error("TPM2 enrollment failed.")
    else: Utils.log("Successfully re-enrolled TPM2")

def main(argv=None):
    args = Utils.parse_args(["--full-disk-encryption-only", "--user-account-only", "--update-tpm2"], argv)
    if args.full_disk_encryption_only and args.user_account_only:
        Utils.abort("Cannot use both --full-disk-encryption-only and --user-account-only")
    change_fde = not args.user_account_only
    change_user = not args.full_disk_encryption_only
    user = Config.eval("config.settings.user.admin.username")
    old_password = ask_for_old_password()
    new_password = ask_for_new_password()
    if change_fde:
        change_luks_password(old_password, new_password)
        if args.update_tpm2: update_tpm2()
    if change_user:
        change_account_password_via_pam(user, old_password, new_password)
        sync_hashed_password_file(new_password)
        Utils.log("Rebuilding system to lock in the new account password...")
        Config.update(rebuild_file_system=False, reboot=False,
                      delete_cache=False, upgrade=False)
    Utils.log("Password change complete!")

if __name__ == "__main__":
    main()
