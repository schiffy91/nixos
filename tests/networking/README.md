# Trusted network connections

`nix-build tests/networking/policy --no-out-link` runs the BTRC unit checks for
rule planning, address changes, IPv6, malformed inputs, interface boundaries
and pre-down removal. `nix-instantiate --eval --strict
--json --expr 'import ./tests/networking/eval.nix {}'` checks static, roaming
and unconfigured Nix policy independently.

The `networking-policy` flake check runs both small checks. The two VMs of the
`networking-roaming` graph node use real
NetworkManager profiles and the production dispatcher: trusted access, profile
switches on the same interface/subnet, subnet changes, firewall restart and
connection teardown. Undeclared ports remain closed.
Malformed configuration also revokes existing grants, and the next valid
reconciliation must acquire the same lock and restore the permitted access.

The `networking-wifi-scan` graph node boots three machines, each a simulated
home network of two access points and a device on it. Unset, NetworkManager
gives wpa_supplicant its own background scan policy; a declared threshold with
no recurring scan, and scanning switched off, reach wpa_supplicant on connect,
on reconnect and on a switch.

Fixed hosts may set `settings.networking.lanSubnet`. Roaming hosts instead list
trusted NetworkManager profile UUIDs in `settings.networking.trustedConnections`.
Only the active profile's directly connected global prefixes get the configured
SSH, discovery and application ports. Other profiles grant no inbound access.
Trust survives DHCP address changes; it does not extend to arbitrary Wi-Fi.
