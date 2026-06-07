# ntdll Delay-Load Protection

Protects delay-load IAT patching when a module maps its thunk table read-only
after loader initialization. This keeps explorer-side tray code from crashing
when a delayed import is resolved after startup.
