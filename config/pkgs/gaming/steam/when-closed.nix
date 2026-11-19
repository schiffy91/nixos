# A command that writes Steam's files runs only while Steam is closed, because Steam writes them back
# as it exits: at once if it is closed, and otherwise when it has exited. A unit of Type=notify is told
# it is ready either way, so a boot waits for the work and an update does not wait for Steam.
{ pkgs, user, ready ? "${pkgs.systemd}/bin/systemd-notify --ready" }:
name: command: pkgs.writeShellScript "${name}-when-steam-is-closed" ''
  if ${pkgs.procps}/bin/pgrep -u ${user} -x steam > /dev/null; then
    ${ready} --status="waiting for Steam to close"
    ${pkgs.procps}/bin/pidwait -u ${user} -x steam || true  # sleeps until the last one exits; no polling
    ${command}
  else
    ${command}
    ${ready} --status="done"
  fi
''
