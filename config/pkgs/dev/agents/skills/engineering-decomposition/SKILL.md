---
name: engineering-decomposition
description: Use when designing, refactoring, debugging, or testing complex systems by defining the desired end state, decomposing it into atomic contracts, composing those atoms into cascades, and proving risky paths in realistic simulations before touching real systems.
metadata:
  short-description: Design the end state, decompose into atoms, prove with simulation
---

# Engineering Decomposition

Start from the shape the system should have when it is good.

## Principles

1. Define the elegant end state first: architecture, API, tree, workflow, and observable behavior.

2. Decompose that end state into atomic contracts, then compose those atoms into cascading functions where each layer has a clear reason to exist.

3. Use realistic simulation environments to prove dangerous, stateful, or platform-dependent paths before touching real systems, then keep those simulations as repeatable tests.

## Workflow

1. Describe the final design in concrete terms.
2. List the atoms required to build it.
3. Give each atom one contract.
4. Test each atom directly.
5. Compose atoms into small cascades.
6. Test each cascade boundary.
7. Run the real entrypoint in the closest realistic environment available.
8. Keep the simulation or end-to-end path as regression coverage.

Do not let implementation order choose the architecture.

Do not trust a composed system whose atoms were never proven.

Do not use a mock when the real risk is boot, disk, network, editor, process, platform, or state-machine behavior. Build or use a simulator that exercises the real path.
