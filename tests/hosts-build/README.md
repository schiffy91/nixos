# Building the real hosts

`host-build-<host>-<target>` builds the system FRACTAL-NORTH and STEAM-DECK-OLED themselves run, for both
boot targets. The flake checks evaluate these systems and the `host-install-*` nodes install VM twins of
them; neither builds the real one, so a package that evaluates and does not build reached a machine
unseen. Most of each system is already in the store, so a node builds what the change touched.
