# Build and run the hidden-present regression test inside a Proton prefix.
#   make            build dcomp-hidden-present.exe with mingw (proton-custom dev shell)
#   make run TOOL=~/.local/share/Steam/compatibilitytools.d/proton-custom-GE-Proton11-7
#                   run it under that tool through umu with the Wayland driver; exit 0 = pass, 2 = hang
MINGW_CXX ?= x86_64-w64-mingw32-g++  # the dev shell exports CXX as the host compiler
TOOL ?= $(HOME)/.local/share/Steam/compatibilitytools.d/proton-custom-GE-Proton11-7
PREFIX ?= $(HOME)/Games/Battle.net/prefix-dcomp-test
EXE := dcomp-hidden-present.exe

.PHONY: all run clean

all: $(EXE)

$(EXE): main.cpp
	$(MINGW_CXX) -O1 -Wall -static -o $@ main.cpp -ld3d11 -ldxgi -ldcomp -ldxguid -luser32

# pressure-vessel cannot see /etc/nixos, so the exe is staged under the home directory
STAGE := $(HOME)/Games/Battle.net/dcomp-hidden-present

run: $(EXE)
	mkdir -p "$(PREFIX)" "$(STAGE)"
	cp -f "$(EXE)" "$(STAGE)/"
	rm -f "$(STAGE)/result.txt"
	cd "$(STAGE)" && WINEPREFIX="$(PREFIX)" GAMEID=umu-dcomp-hidden-present PROTONPATH="$(TOOL)" \
	PROTON_USE_WOW64=1 PROTON_ENABLE_WAYLAND=1 WINEDEBUG=-all DXVK_LOG_LEVEL=none TMPDIR=/tmp \
	timeout 120 umu-run "$(STAGE)/$(EXE)" >/dev/null 2>&1; echo "umu exit=$$?"
	@cat "$(STAGE)/result.txt" 2>/dev/null || echo "FAIL: no result written"
	@grep -q '^PASS' "$(STAGE)/result.txt" 2>/dev/null

clean:
	rm -f $(EXE)
