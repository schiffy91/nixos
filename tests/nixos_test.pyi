"""Names supplied by the NixOS VM test driver, for standalone editor checks.

Machines expose a dynamic driver API; runtime VM tests validate its operations.
These declarations are imported only under TYPE_CHECKING.
"""

from typing import Any, ContextManager

machine: Any
client: Any
deck: Any
fractal: Any
real_suspend: bool
with_sunshine: bool

def start_all() -> None: ...
def subtest(name: str) -> ContextManager[None]: ...
