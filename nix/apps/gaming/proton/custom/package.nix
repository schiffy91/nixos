# proton-custom - GE-Proton11-5 with winewayland.drv cleanup patches.
#
# Builds the exact Wine and DXVK revisions GE-Proton11-5 uses, then layers
# our active patch series on top. Replaces the changed binaries touched by the
# active series plus the matching 32/64-bit Unix-side Wine modules. The PE and
# Unix halves must stay ABI-matched when D3D11/CEF exercises generated Unix
# thunk tables.
#
# Patches live under ./patches/<topic>/, one subfolder per upstreamable topic.
# The default package applies the explicit activePatchSeries list below, not a
# broad glob. Patch files are in `git format-patch -s` style (Wine upstream
# convention), so their 0001/0002 names are series-local commit order only.
#
# Active series:
#   Wayland startup deadlock (non-blocking second init roundtrip)
#   Bounded WM_CANCELMODE on keyboard leave
#   Layered surface alpha uploads (GE-Proton11 ships the pUpdateLayeredWindow hook)
#   SNI StatusNotifierItem systray via libdbus (winewayland dock bridge)
#   Delay-load IAT protection for PE modules with read-only thunk pages
#   DComp/DXGI/winewayland GPU presentation path
#   winevulkan, winewayland, and win32u PE/Unix pairs rebuilt from the same Wine source
#   DXVK composition swap-chain support
#
# Dropped at the GE-Proton11-5 rebase, now covered upstream:
#   xdg_popup for transient windows (GE wine-wayland 0031)
#   pUpdateLayeredWindow hook (GE wine-wayland 0014)
#   D3DKMT shared GPU resources (win32u implements OpenResource/QueryResourceInfo)
{ stdenv
, stdenv_32bit
, pkgs
, fetchgit
, fetchFromGitHub
, makeWrapper
, rsync
, unzip
}:

let
  toolVersion = "GE-Proton11-5";
  toolName    = "proton-custom-${toolVersion}";

  # false ships the pristine GE tarball (plus WineASIO) under the same tool
  # name, skipping the Wine/DXVK rebuilds entirely — for A/B verification of
  # whether the patch series is still needed on this GE base.
  overlayPatchedBinaries = false;

  # The exact Valve wine commit GE-Proton11-5 uses (from proton-ge-custom
  # git submodule `wine` at tag GE-Proton11-5).
  valveWineRev  = "36078f5f947532885a596dabbc7893c048133660";
  valveWineHash = "sha256-US/ts2HLhKr+xHMCUWIFlpmQdJ3CDkYeMUB2EAzOblU=";

  geProtonRev  = "GE-Proton11-5";
  geProtonHash = "sha256-v1uwzVNzneBBRbaWRz2NTBNeTkqOjdwyEfBhDHJAeMc=";

  dxvkVersion = "v3.0.2-21-g3a4c6fa3";
  dxvkRev     = "3a4c6fa3cb1548d56a90a38dd8f526b6c13e63fd";
  dxvkHash    = "sha256-KWPOc+wA3zivLEYXBEHOJgVzCWU0X7joK+PuSFoDplE=";
  wineasio64 = pkgs.wineasio;
  wineasio32Files = ../../rocksmith/assets;

  # GE-Proton binary tarball (DXVK, VKD3D-Proton, Proton scripts, mono, gecko).
  # Keep this pinned to the exact tool whose Wine tree is patched below; using
  # pkgs.proton-ge-bin.src here would silently follow nixpkgs updates.
  ge-proton-src = pkgs.fetchurl {
    url = "https://github.com/GloriousEggRoll/proton-ge-custom/releases/download/${toolVersion}/${toolVersion}-x86_64.tar.gz";
    hash = "sha256-3kPEsl88BH20m5bETYR1mVLFoBMypogFoJ5p+V3DinU=";
  };

  activePatchSeries = [
    ./patches/wine-wayland-roundtrip/0001-winewayland.drv-Avoid-second-init-roundtrip.patch
    ./patches/wine-wayland-focus/0001-winewayland.drv-Bound-WM_CANCELMODE-on-keyboard-leav.patch
    ./patches/wine-wayland-layered-windows/0001-winewayland.drv-Fix-layered-surface-alpha-uploads.patch
    ./patches/wine-wayland-layered-windows/0002-winewayland.drv-Handle-fully-zero-alpha-layered-surf.patch
    ./patches/wine-wayland-status-notifier/0001-winewayland.drv-Add-StatusNotifierItem-tray-support.patch
    ./patches/wine-wayland-status-notifier/0002-winewayland.drv-Polish-SNI-context-menu-callbacks.patch
    ./patches/wine-wayland-status-notifier/0003-explorer-Forward-docked-tray-icon-updates.patch
    ./patches/wine-wayland-status-notifier/0004-winewayland.drv-Keep-SNI-items-self-contained.patch
    ./patches/ntdll-delay-load/0001-ntdll-Make-delay-load-IAT-writable-before-patching.patch
    ./patches/dcomp-wayland-gpu-present/0001-dcomp-Implement-D3D11-backed-desktop-composition.patch
    ./patches/dcomp-wayland-gpu-present/0002-dcomp-Clip-composition-host-windows-to-the-target-cl.patch
    ./patches/dcomp-wayland-gpu-present/0003-dcomp-Do-not-mark-composition-host-windows-transpare.patch
    ./patches/dcomp-wayland-gpu-present/0004-dcomp-Make-composition-host-windows-presentation-onl.patch
    ./patches/dcomp-wayland-gpu-present/0005-dcomp-Keep-composition-hosts-above-target-backing.patch
    ./patches/dcomp-wayland-gpu-present/0006-dxgi-Create-a-hidden-swap-chain-for-composition.patch
    ./patches/dcomp-wayland-gpu-present/0007-dcomp-Present-IDCompositionSurface-content.patch
    ./patches/dcomp-wayland-gpu-present/0008-dcomp-Handle-incremental-surface-draws.patch
    ./patches/dcomp-wayland-gpu-present/0009-dcomp-Clip-composition-hosts-with-window-regions.patch
    ./patches/dcomp-wayland-gpu-present/0010-dcomp-Host-placed-composition-swap-chain-visuals.patch
    ./patches/dcomp-wayland-gpu-present/0011-dcomp-Avoid-redundant-composition-host-updates.patch
    ./patches/dcomp-wayland-gpu-present/0012-dcomp-Implement-GPU-backed-surface-factories.patch
    ./patches/dcomp-wayland-gpu-present/0013-dcomp-Unbind-removed-composition-hosts.patch
    ./patches/dcomp-wayland-gpu-present/0014-dcomp-Implement-virtual-surfaces-and-scrolling.patch
    ./patches/dcomp-wayland-gpu-present/0015-dcomp-Unbind-composition-targets-before-destroying-h.patch
    ./patches/dcomp-wayland-gpu-present/0016-dcomp-Hide-composition-host-windows-from-parent.patch
    ./patches/dcomp-wayland-gpu-present/0017-dcomp-Show-surface-hosts-after-content-binding.patch
    ./patches/dcomp-wayland-gpu-present/0018-dcomp-Clip-target-parents-around-composition-hosts.patch
    ./patches/dcomp-wayland-gpu-present/0019-dcomp-Avoid-hosts-for-unplaced-child-swapchains.patch
    ./patches/win32u-load-driver-deadlock/0001-win32u-Bound-desktop-driver-readiness-wait.patch
  ];

  dxvkPatchSeries = [
    ./patches/dxvk-composition-swapchain/0001-dxgi-Bind-composition-swap-chains-to-DComp-windows.patch
    ./patches/dxvk-composition-swapchain/0002-d3d11-Pace-composition-swap-chains-with-the-composit.patch
    ./patches/dxvk-composition-swapchain/0003-d3d11-Allow-limiting-shared-resource-tier.patch
    ./patches/dxvk-composition-swapchain/0004-d3d11-Pace-all-composition-swap-chains.patch
    ./patches/dxvk-composition-swapchain/0005-d3d11-Keep-composition-target-windows-sized-to-swap.patch
    ./patches/dxvk-composition-swapchain/0006-d3d11-Preserve-composition-swap-chain-contents.patch
    ./patches/dxvk-composition-swapchain/0007-d3d11-Show-composition-targets-on-first-present.patch
    ./patches/dxvk-composition-swapchain/0008-d3d11-Replay-composition-content-after-target-binds.patch
    ./patches/dxvk-composition-swapchain/0009-d3d11-Use-opaque-WSI-alpha-for-composition-hosts.patch
    ./patches/dxvk-composition-swapchain/0010-d3d11-Trace-composition-present-paths.patch
    ./patches/dxvk-composition-swapchain/0011-d3d11-Gate-composition-target-bind-trace.patch
  ];

  applyActivePatchSeries = pkgs.lib.concatMapStringsSep "\n" (patchFile: ''
      echo "proton-custom: applying ${patchFile}"
      patch -p1 < ${patchFile}
  '') activePatchSeries;

  # -- Source: Valve wine + GE wayland patches + active cleanup series -------
  wine-proton-custom-src = stdenv.mkDerivation {
    pname = "wine-proton-custom-src";
    version = toolVersion;

    src = fetchgit {
      url    = "https://github.com/ValveSoftware/wine";
      rev    = valveWineRev;
      hash   = valveWineHash;
      deepClone = false;
    };

    geProtonSrc = fetchFromGitHub {
      owner = "GloriousEggRoll";
      repo  = "proton-ge-custom";
      rev   = geProtonRev;
      hash  = geProtonHash;
    };

    nativeBuildInputs = [ pkgs.git ];

    dontConfigure = true;
    dontBuild     = true;

    installPhase = ''
      cp -r . "$out"
      chmod -R u+w "$out"
      cd "$out"

      # Apply 504 GE-Proton wine-wayland patches in order
      for p in $(ls "$geProtonSrc"/patches/wine-hotfixes/wine-wayland/*.patch | sort); do
        if patch -p1 --dry-run < "$p" >/dev/null 2>&1; then
          patch -p1 < "$p" >/dev/null
        fi
      done

      # Apply only the explicit active series in the order listed above.
      ${applyActivePatchSeries}

      find . -name '*.orig' -delete
    '';
  };

  # -- Build: configure wine and build only artifacts touched by active patches
  wine-proton-custom = stdenv_32bit.mkDerivation {
    pname = "wine-proton-custom";
    version = toolVersion;
    src = wine-proton-custom-src;

    nativeBuildInputs = with pkgs; [
      autoconf automake bison flex perl python3 pkg-config
      wayland-scanner
      pkgsCross.mingwW64.buildPackages.gcc
      pkgsCross.mingw32.buildPackages.gcc
    ];

    buildInputs =
      (with pkgs; [
        wayland dbus libxkbcommon mesa libGL
        vulkan-headers vulkan-loader
        libx11 freetype fontconfig
      ])
      ++ (with pkgs.pkgsi686Linux; [
        wayland dbus libxkbcommon mesa libGL
        vulkan-loader
        libx11 freetype fontconfig
      ]);

    postPatch = ''
      # make_vulkan writes a cache under HOME; the nix builder's /homeless-shelter
      # is read-only, so point HOME at $TMPDIR before running it.
      # Feed make_vulkan the registry this Wine tree ships, not pkgs.vulkan-headers:
      # a newer vk.xml emits structs (VkDeviceAddressRangeEXT and friends) whose
      # 32-bit conversions this generator cannot write, and the thunks stop compiling.
      HOME=$TMPDIR python3 dlls/winevulkan/make_vulkan \
        -x "$PWD/dlls/winevulkan/vk.xml" \
        -X "$PWD/dlls/winevulkan/video.xml"

      # Proton 11 stopped committing the generated request and syscall tables,
      # so regenerate them here. Both tools carry /usr/bin/perl shebangs, hence
      # patchShebangs first; failures must be fatal or configure dies later on a
      # missing ntsyscalls.h.
      patchShebangs tools
      ./tools/make_requests
      ./tools/make_specfiles

      # Wine source ships with autogen.sh, not a pre-generated ./configure -
      # run it to produce ./configure from configure.ac.
      HOME=$TMPDIR autoreconf -fi
    '';

    dontConfigure = true;

    buildPhase = ''
      runHook preBuild

      source_dir="$PWD"
      mkdir -p "$TMPDIR/wine64" "$TMPDIR/wine32"

      # GE-Proton ships the shared WoW64 layout with separate i386-unix and
      # x86_64-unix modules. Build both trees so generated Unix-call tables
      # match their corresponding PE DLLs.
      cd "$TMPDIR/wine64"
      HOME=$TMPDIR "$source_dir/configure" \
        --enable-win64 \
        --without-x \
        --disable-tests
      make -j"$NIX_BUILD_CORES" \
        dlls/dcomp/all \
        dlls/dxgi/all \
        dlls/ntdll/all \
        dlls/win32u/all \
        dlls/winevulkan/all \
        dlls/winewayland.drv/all \
        programs/explorer/all

      cd "$TMPDIR/wine32"
      HOME=$TMPDIR "$source_dir/configure" \
        --with-wine64="$TMPDIR/wine64" \
        --without-x \
        --disable-tests
      make -j"$NIX_BUILD_CORES" \
        dlls/dcomp/all \
        dlls/dxgi/all \
        dlls/ntdll/all \
        dlls/win32u/all \
        dlls/winevulkan/all \
        dlls/winewayland.drv/all \
        programs/explorer/all

      runHook postBuild
    '';

    installPhase = ''
      copy_required() {
        local dst="$1"
        shift
        local src
        for src in "$@"; do
          if [ -e "$src" ]; then
            install -Dm644 "$src" "$out/lib/wine/$dst"
            return 0
          fi
        done
        echo "missing expected patched artifact: $dst" >&2
        printf '  tried: %s\n' "$@" >&2
        return 1
      }

      wine64_build="$TMPDIR/wine64"
      wine32_build="$TMPDIR/wine32"

      copy_required x86_64-unix/winewayland.so \
        "$wine64_build/dlls/winewayland.drv/winewayland.so" \
        "$wine64_build/dlls/winewayland.drv/x86_64-unix/winewayland.so"
      copy_required i386-unix/winewayland.so \
        "$wine32_build/dlls/winewayland.drv/winewayland.so" \
        "$wine32_build/dlls/winewayland.drv/i386-unix/winewayland.so"
      copy_required x86_64-unix/winevulkan.so \
        "$wine64_build/dlls/winevulkan/winevulkan.so" \
        "$wine64_build/dlls/winevulkan/x86_64-unix/winevulkan.so"
      copy_required i386-unix/winevulkan.so \
        "$wine32_build/dlls/winevulkan/winevulkan.so" \
        "$wine32_build/dlls/winevulkan/i386-unix/winevulkan.so"
      copy_required x86_64-unix/win32u.so \
        "$wine64_build/dlls/win32u/win32u.so" \
        "$wine64_build/dlls/win32u/x86_64-unix/win32u.so"
      copy_required i386-unix/win32u.so \
        "$wine32_build/dlls/win32u/win32u.so" \
        "$wine32_build/dlls/win32u/i386-unix/win32u.so"
      copy_required x86_64-unix/ntdll.so \
        "$wine64_build/dlls/ntdll/ntdll.so" \
        "$wine64_build/dlls/ntdll/x86_64-unix/ntdll.so"
      copy_required i386-unix/ntdll.so \
        "$wine32_build/dlls/ntdll/ntdll.so" \
        "$wine32_build/dlls/ntdll/i386-unix/ntdll.so"
      copy_required x86_64-windows/winewayland.drv \
        "$wine64_build/dlls/winewayland.drv/x86_64-windows/winewayland.drv" \
        "$wine64_build/dlls/winewayland.drv/winewayland.drv"
      copy_required i386-windows/winewayland.drv \
        "$wine32_build/dlls/winewayland.drv/i386-windows/winewayland.drv" \
        "$wine32_build/dlls/winewayland.drv/winewayland.drv"
      copy_required x86_64-windows/dcomp.dll \
        "$wine64_build/dlls/dcomp/x86_64-windows/dcomp.dll" \
        "$wine64_build/dlls/dcomp/dcomp.dll"
      copy_required i386-windows/dcomp.dll \
        "$wine32_build/dlls/dcomp/i386-windows/dcomp.dll" \
        "$wine32_build/dlls/dcomp/dcomp.dll"
      copy_required x86_64-windows/dxgi.dll \
        "$wine64_build/dlls/dxgi/x86_64-windows/dxgi.dll" \
        "$wine64_build/dlls/dxgi/dxgi.dll"
      copy_required i386-windows/dxgi.dll \
        "$wine32_build/dlls/dxgi/i386-windows/dxgi.dll" \
        "$wine32_build/dlls/dxgi/dxgi.dll"
      copy_required x86_64-windows/winevulkan.dll \
        "$wine64_build/dlls/winevulkan/x86_64-windows/winevulkan.dll" \
        "$wine64_build/dlls/winevulkan/winevulkan.dll"
      copy_required i386-windows/winevulkan.dll \
        "$wine32_build/dlls/winevulkan/i386-windows/winevulkan.dll" \
        "$wine32_build/dlls/winevulkan/winevulkan.dll"
      copy_required x86_64-windows/win32u.dll \
        "$wine64_build/dlls/win32u/x86_64-windows/win32u.dll" \
        "$wine64_build/dlls/win32u/win32u.dll"
      copy_required i386-windows/win32u.dll \
        "$wine32_build/dlls/win32u/i386-windows/win32u.dll" \
        "$wine32_build/dlls/win32u/win32u.dll"
      copy_required x86_64-windows/ntdll.dll \
        "$wine64_build/dlls/ntdll/x86_64-windows/ntdll.dll" \
        "$wine64_build/dlls/ntdll/ntdll.dll"
      copy_required i386-windows/ntdll.dll \
        "$wine32_build/dlls/ntdll/i386-windows/ntdll.dll" \
        "$wine32_build/dlls/ntdll/ntdll.dll"
      copy_required x86_64-windows/explorer.exe \
        "$wine64_build/programs/explorer/x86_64-windows/explorer.exe" \
        "$wine64_build/programs/explorer/explorer.exe"
      copy_required i386-windows/explorer.exe \
        "$wine32_build/programs/explorer/i386-windows/explorer.exe" \
        "$wine32_build/programs/explorer/explorer.exe"
    '';

    meta.platforms = [ "x86_64-linux" ];
  };

  dxvk-proton-custom-src = stdenv.mkDerivation {
    pname = "dxvk-proton-custom-src";
    version = dxvkVersion;

    src = fetchgit {
      url = "https://github.com/doitsujin/dxvk";
      rev = dxvkRev;
      hash = dxvkHash;
      fetchSubmodules = true;
    };

    patches = dxvkPatchSeries;
    patchFlags = [ "-p1" "--fuzz=0" ];

    postPatch = ''
      find . -name '*.orig' -delete
    '';

    dontConfigure = true;
    dontBuild = true;

    installPhase = ''
      cp -r . "$out"
      chmod -R u+w "$out"
    '';
  };

  dxvk-proton-custom = stdenv.mkDerivation {
    pname = "dxvk-proton-custom";
    version = dxvkVersion;
    src = dxvk-proton-custom-src;

    nativeBuildInputs = with pkgs; [
      glslang
      meson
      ninja
      pkg-config
      python3
      pkgsCross.mingwW64.buildPackages.gcc
      pkgsCross.mingw32.buildPackages.gcc
    ];

    dontConfigure = true;

    buildPhase = ''
      runHook preBuild

      patchShebangs subprojects
      substituteInPlace src/dxvk/meson.build \
        --replace-fail "dxvk_extra_deps = [ dependency('threads') ]" \
                       "dxvk_extra_deps = [ dependency('threads'), cpp.find_library('mcfgthread') ]"
      substituteInPlace src/vulkan/meson.build \
        --replace-fail "dependencies        : [ thread_dep ]," \
                       "dependencies        : [ thread_dep, cpp.find_library('mcfgthread') ],"
      substituteInPlace src/dxgi/meson.build \
        --replace-fail "dxgi_ld_args      = []" \
                       "dxgi_ld_args      = [ '-Wl,--whole-archive', '-lmcfgthread', '-Wl,--no-whole-archive' ]"

      export LIBRARY_PATH="${pkgs.pkgsCross.mingwW64.windows.mcfgthreads}/lib:${pkgs.pkgsCross.mingwW64.windows.pthreads}/lib"
      export LDFLAGS="-L${pkgs.pkgsCross.mingwW64.windows.mcfgthreads}/lib -L${pkgs.pkgsCross.mingwW64.windows.pthreads}/lib"
      meson setup --cross-file build-win64.txt \
        --buildtype release \
        --prefix "$out" \
        --bindir x64 \
        --libdir x64 \
        --strip \
        -Db_ndebug=if-release \
        -Dbuild_id=false \
        build.64
      ninja -C build.64 src/dxgi/dxgi.dll src/d3d11/d3d11.dll

      export LIBRARY_PATH="${pkgs.pkgsCross.mingw32.windows.mcfgthreads}/lib:${pkgs.pkgsCross.mingw32.windows.pthreads}/lib"
      export LDFLAGS="-L${pkgs.pkgsCross.mingw32.windows.mcfgthreads}/lib -L${pkgs.pkgsCross.mingw32.windows.pthreads}/lib"
      meson setup --cross-file build-win32.txt \
        --buildtype release \
        --prefix "$out" \
        --bindir x32 \
        --libdir x32 \
        --strip \
        -Db_ndebug=if-release \
        -Dbuild_id=false \
        build.32
      ninja -C build.32 src/dxgi/dxgi.dll src/d3d11/d3d11.dll

      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      install -Dm755 build.64/src/dxgi/dxgi.dll "$out/x64/dxgi.dll"
      install -Dm755 build.64/src/d3d11/d3d11.dll "$out/x64/d3d11.dll"
      install -Dm755 build.32/src/dxgi/dxgi.dll "$out/x32/dxgi.dll"
      install -Dm755 build.32/src/d3d11/d3d11.dll "$out/x32/d3d11.dll"
      printf '%s dxvk (%s)\n' '${dxvkRev}' '${dxvkVersion}' > "$out/version"
      runHook postInstall
    '';

    meta.platforms = [ "x86_64-linux" ];
  };

  # -- Final compat tool: GE-Proton binary + our patched files --------------
in stdenv.mkDerivation {
  pname   = toolName;
  version = toolVersion;
  src     = ge-proton-src;

  nativeBuildInputs = [ makeWrapper rsync unzip ];
  dontConfigure = true;
  dontBuild     = true;

  installPhase = ''
    runHook preInstall

    # The activation script in default.nix symlinks this package to:
    #   ~/.local/share/Steam/compatibilitytools.d/<pname> -> $out
    # so the compat tool files must live directly in $out/, not a subdirectory.
    mkdir -p "$out"
    cp -r . "$out/"

'' + pkgs.lib.optionalString overlayPatchedBinaries ''
    # Overlay our patched binaries on top of the GE-Proton tarball.
    copy_patched() {
      local rel="$1"
      if [ ! -e "${wine-proton-custom}/lib/wine/$rel" ]; then
        echo "missing patched artifact from wine-proton-custom: $rel" >&2
        return 1
      fi
      if [ ! -e "$out/files/lib/wine/$rel" ]; then
        echo "GE-Proton tarball does not contain expected artifact: $rel" >&2
        return 1
      fi
      cp "${wine-proton-custom}/lib/wine/$rel" "$out/files/lib/wine/$rel"
    }

    copy_patched x86_64-unix/winewayland.so
    copy_patched i386-unix/winewayland.so
    copy_patched x86_64-unix/winevulkan.so
    copy_patched i386-unix/winevulkan.so
    copy_patched x86_64-unix/win32u.so
    copy_patched i386-unix/win32u.so
    copy_patched x86_64-unix/ntdll.so
    copy_patched i386-unix/ntdll.so
    copy_patched x86_64-windows/winewayland.drv
    copy_patched i386-windows/winewayland.drv
    copy_patched x86_64-windows/dcomp.dll
    copy_patched i386-windows/dcomp.dll
    copy_patched x86_64-windows/dxgi.dll
    copy_patched i386-windows/dxgi.dll
    copy_patched x86_64-windows/winevulkan.dll
    copy_patched i386-windows/winevulkan.dll
    copy_patched x86_64-windows/win32u.dll
    copy_patched i386-windows/win32u.dll
    copy_patched x86_64-windows/ntdll.dll
    copy_patched i386-windows/ntdll.dll
    copy_patched x86_64-windows/explorer.exe
    copy_patched i386-windows/explorer.exe

    cp "${wine-proton-custom}/lib/wine/x86_64-windows/explorer.exe" \
      "$out/files/share/default_pfx/drive_c/windows/explorer.exe"
    cp "${wine-proton-custom}/lib/wine/x86_64-windows/explorer.exe" \
      "$out/files/share/default_pfx/drive_c/windows/system32/explorer.exe"
    cp "${wine-proton-custom}/lib/wine/i386-windows/explorer.exe" \
      "$out/files/share/default_pfx/drive_c/windows/syswow64/explorer.exe"
    cp "${wine-proton-custom}/lib/wine/x86_64-windows/ntdll.dll" \
      "$out/files/share/default_pfx/drive_c/windows/system32/ntdll.dll"
    cp "${wine-proton-custom}/lib/wine/i386-windows/ntdll.dll" \
      "$out/files/share/default_pfx/drive_c/windows/syswow64/ntdll.dll"
    copy_dxvk() {
      local src="$1"
      local rel="$2"
      if [ ! -e "$src" ]; then
        echo "missing patched DXVK artifact: $src" >&2
        return 1
      fi
      if [ ! -e "$out/files/lib/wine/dxvk/$rel" ]; then
        echo "GE-Proton tarball does not contain expected DXVK artifact: $rel" >&2
        return 1
      fi
      cp "$src" "$out/files/lib/wine/dxvk/$rel"
    }

    copy_dxvk "${dxvk-proton-custom}/x64/dxgi.dll" x86_64-windows/dxgi.dll
    copy_dxvk "${dxvk-proton-custom}/x32/dxgi.dll" i386-windows/dxgi.dll
    copy_dxvk "${dxvk-proton-custom}/x64/d3d11.dll" x86_64-windows/d3d11.dll
    copy_dxvk "${dxvk-proton-custom}/x32/d3d11.dll" i386-windows/d3d11.dll
    cp "${dxvk-proton-custom}/version" "$out/files/lib/wine/dxvk/version"

'' + ''
    # WineASIO is not part of GE-Proton. Nixpkgs currently packages WineASIO
    # 1.3.0 for 64-bit Wine; Rocksmith still needs the 32-bit driver, so keep
    # the existing known-working 32-bit pair as a packaged compat-tool payload
    # instead of mutating the Steam compat tool during activation.
    install -Dm644 "${wineasio64}/lib/wine/x86_64-windows/wineasio64.dll" \
      "$out/files/lib/wine/x86_64-windows/wineasio64.dll"
    install -Dm755 "${wineasio64}/lib/wine/x86_64-unix/wineasio64.dll.so" \
      "$out/files/lib/wine/x86_64-unix/wineasio64.dll.so"
    install -Dm644 "${wineasio32Files}/wineasio32.dll" \
      "$out/files/lib/wine/i386-windows/wineasio32.dll"
    install -Dm755 "${wineasio32Files}/wineasio32.dll.so" \
      "$out/files/lib/wine/i386-unix/wineasio32.dll.so"

    cat > "$out/compatibilitytool.vdf" <<EOF
"compatibilitytools"
{
  "compat_tools"
  {
    "${toolName}"
    {
      "install_path" "."
      "display_name" "proton-custom ${toolVersion} (Wayland SNI)"
      "from_oslist"  "windows"
      "to_oslist"    "linux"
    }
  }
}
EOF

    runHook postInstall
  '';

  meta = {
    description = "${toolVersion} with Wine Wayland, DComp, DXVK, SNI, and WineASIO patches";
    homepage    = "https://github.com/GloriousEggRoll/proton-ge-custom";
    platforms   = [ "x86_64-linux" ];
  };

  passthru = {
    wineSource = wine-proton-custom-src;
    wineArtifacts = wine-proton-custom;
    dxvkSource = dxvk-proton-custom-src;
    dxvkArtifacts = dxvk-proton-custom;
  };
}
