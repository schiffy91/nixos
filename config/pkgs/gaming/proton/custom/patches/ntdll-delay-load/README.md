# Delay-load IAT protection in ntdll

## Symptom
A PE module crashes with an access violation inside
`LdrResolveDelayLoadedAPI()` the first time one of its delay-loaded imports
is called.

## Root cause
`LdrResolveDelayLoadedAPI()` writes the resolved function pointer straight
into the module's delay-load import address table. Modules built by
winebuild keep that table in `.data`, so the write always succeeds for
Wine's own DLLs. MSVC builds using `/guard:cf` with
`IMAGE_GUARD_PROTECT_DELAYLOAD_IAT` set in their load configuration place
the delay IAT in a read-only `.didat` section instead and rely on the loader
to make it writable for the duration of the patch, which is what Windows'
`LdrResolveDelayLoadedAPI()` does. Wine's loader wrote through the read-only
page and faulted.

## Fix
Reprotect the thunk as `PAGE_EXECUTE_READWRITE` around the store and restore
the previous protection afterwards. Execute permission is kept in case the
table shares a page with code. The protect/store/restore sequence runs under
the loader lock so two threads resolving imports on the same page cannot
restore it to read-only underneath each other. A failed reprotect is logged
with `WARN` and the store skipped; the resolved pointer is still returned,
so the call succeeds and only the caching is lost.

## Affected upstream
`dlls/ntdll/loader.c`.
