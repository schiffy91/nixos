from typing import TYPE_CHECKING

if TYPE_CHECKING:
    # NixOS supplies these names when executing the test script.
    from nixos_test import client, machine, start_all, subtest  # ty: ignore[unresolved-import]

start_all()
machine.wait_for_unit("NetworkManager.service")
machine.wait_for_unit("firewall.service")
machine.wait_for_unit("sshd.service")
machine.wait_for_unit("http-8080.service")
machine.wait_for_unit("http-8081.service")
client.wait_for_unit("network.target")
machine.succeed("nmcli device set eth1 managed yes")
machine.succeed("nmcli connection add type ethernet ifname eth1 con-name trusted connection.uuid 11111111-1111-4111-8111-111111111111 connection.autoconnect no ipv4.method manual ipv4.addresses 10.55.1.1/24 ipv6.method disabled")
machine.succeed("nmcli connection add type ethernet ifname eth1 con-name untrusted connection.uuid 22222222-2222-4222-8222-222222222222 connection.autoconnect no ipv4.method manual ipv4.addresses 10.55.1.1/24 ipv6.method disabled")

with subtest("trusted profile opens only configured ports on its current subnet"):
    machine.succeed("nmcli connection up trusted")
    machine.wait_until_succeeds("iptables -C nixos-trusted -i eth1 -s 10.55.1.0/24 -p tcp --dport 22 -j nixos-fw-accept")
    client.wait_until_succeeds("nc -z -w 2 10.55.1.1 22")
    client.succeed("nc -z -w 2 10.55.1.1 8080")
    client.fail("nc -z -w 2 10.55.1.1 8081")

with subtest("another profile on the same interface and subnet loses access"):
    machine.succeed("nmcli connection up untrusted")
    machine.wait_until_succeeds("test -z \"$(iptables -S nixos-trusted | sed -n '/^-A /p')\"")
    client.fail("nc -z -w 2 10.55.1.1 22")
    client.fail("nc -z -w 2 10.55.1.1 8080")

with subtest("a trusted profile follows a changed subnet without stale grants"):
    machine.succeed("nmcli connection modify trusted ipv4.addresses 10.77.2.1/24")
    client.succeed("ip address add 10.77.2.2/24 dev eth1")
    machine.succeed("nmcli connection up trusted")
    machine.wait_until_succeeds("iptables -C nixos-trusted -i eth1 -s 10.77.2.0/24 -p tcp --dport 22 -j nixos-fw-accept")
    machine.fail("iptables -C nixos-trusted -i eth1 -s 10.55.1.0/24 -p tcp --dport 22 -j nixos-fw-accept")
    client.wait_until_succeeds("nc -z -w 2 10.77.2.1 22")
    client.succeed("nc -z -w 2 10.77.2.1 8080")

with subtest("firewall restart reconciles the active profile"):
    machine.succeed("systemctl restart firewall.service")
    client.wait_until_succeeds("nc -z -w 2 10.77.2.1 22")

with subtest("disconnect removes access before the interface is reused"):
    machine.succeed("nmcli connection down trusted")
    machine.wait_until_succeeds("test -z \"$(iptables -S nixos-trusted | sed -n '/^-A /p')\"")
