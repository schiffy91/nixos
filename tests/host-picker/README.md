# Host picker

Run the graph node `host-picker`. The BTRC test compiles a driver around the real
`Interactive.askHostPath` method and queries this checkout's actual Nix registry.
It selects every physical Linux host, retries invalid input, and checks EOF is
bounded. A separate input-free Nix flake emits a diagnostic with its registry,
ensuring stderr cannot corrupt JSON parsing even in a clean CI checkout.

The test only selects paths; it never saves host identity or starts installation.
