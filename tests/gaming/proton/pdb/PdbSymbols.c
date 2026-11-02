#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <string.h>

struct symbols {
    DWORD64 first, second, public_only;
    unsigned calls;
    BOOL stop;
};

static BOOL CALLBACK collect(SYMBOL_INFO *symbol, ULONG size, void *context)
{
    struct symbols *symbols = context;
    symbols->calls++;
    if (!strcmp(symbol->Name, "folded_first")) symbols->first = symbol->Address;
    if (!strcmp(symbol->Name, "folded_second")) symbols->second = symbol->Address;
    if (!strcmp(symbol->Name, "public_only")) symbols->public_only = symbol->Address;
    return !symbols->stop;
}

int main(int argc, char **argv)
{
    HANDLE process = GetCurrentProcess();
    DWORD64 base;
    struct symbols all = {0}, wildcard = {0}, cancelled = {0};
    BOOL enumerated;
    if (argc != 2) return 2;
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    if (!SymInitialize(process, NULL, FALSE)) return 3;
    base = SymLoadModuleEx(process, NULL, argv[1], "fixture", 0, 0, NULL, 0);
    if (!base) { fprintf(stderr, "SymLoadModuleEx: %lu\n", GetLastError()); return 4; }
    enumerated = SymEnumSymbols(process, base, "*", collect, &all);
    printf("all: success=%d first=%llx second=%llx public=%llx calls=%u\n",
           enumerated, all.first, all.second, all.public_only, all.calls);
    if (!enumerated || !all.first || all.first != all.second || !all.public_only) return 5;
    enumerated = SymEnumSymbols(process, base, "folded_*", collect, &wildcard);
    if (!enumerated || wildcard.first != all.first || wildcard.second != all.second || wildcard.public_only) return 6;
    cancelled.stop = TRUE;
    SymEnumSymbols(process, base, "*", collect, &cancelled);
    printf("cancellation: callbacks=%u\n", cancelled.calls);
    if (cancelled.calls != 1) return 7;
    SymCleanup(process);
    puts("PASS: PDB folded aliases, public symbols, wildcard matching, callback cancellation");
    return 0;
}
