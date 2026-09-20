# proton-custom - GE-Proton11-7 plus five small Wine bug fixes.
#
# Builds the exact Wine revision GE-Proton11-7 uses, applies GE's complete
# wine-hotfixes set in protonprep order (the shipped tarball's wineserver
# protocol is built from that tree, so the overlaid DLLs must be too), then
# layers our active patch series on top. Only the modules those patches touch
# are rebuilt and overlaid, each as its matched PE/Unix pair: ntdll, win32u and
# winewayland.drv. Everything else, DXVK included, is GE's own binary.
#
# Patches live under ./patches/<topic>/, one subfolder per upstreamable topic.
# The default package applies the explicit activePatchSeries list below, not a
# broad glob. Patch files are in `git format-patch -s` style (Wine upstream
# convention), so their 0001/0002 names are series-local commit order only.
#
# Active series:
#   Bounded WM_CANCELMODE on keyboard leave
#   Layered surface alpha uploads (GE-Proton11 ships the pUpdateLayeredWindow hook)
#   Delay-load IAT protection for PE modules with read-only thunk pages
#   libc thread pointer for PsCreateSystemThread threads (no core dump per CEF renderer at exit)
#   Bounded desktop-driver readiness wait in win32u
#
# Dropped 2026-09-20 (the fallback GE already takes renders Battle.net correctly):
#   DComp/DXGI/winewayland GPU presentation path and the DXVK composition
#   swap-chain series. They emulated DirectComposition for CEF; stock GE fails
#   CreateSwapChainForComposition, CEF falls back to a window swap chain, and
#   that path works. Ours depended on window creation order and went black once
#   a game hid and re-showed the launcher.
#
# Dropped at the GE-Proton11-7 rebase, now covered upstream:
#   StatusNotifierItem systray (GE em-fixups 0001, incl. the wow64win thunk)
#   present waits on hidden windows (GE child-rendering 0066 bounds
#   vkWaitForPresentKHR; unbounded waits deadlocked the Battle.net launcher's
#   login-to-main window switch under Agent load)
#
# Dropped at the GE-Proton11-5 rebase, now covered upstream:
#   xdg_popup for transient windows (GE wine-wayland 0031)
#   pUpdateLayeredWindow hook (GE wine-wayland 0014)
#   D3DKMT shared GPU resources (win32u implements OpenResource/QueryResourceInfo)
# Dropped in the 2026-08 review as functional no-ops:
#   second init roundtrip rework (reverted GE wine-wayland 0142 without effect)
#   fully-zero-alpha layered surface fixup (win32u shape copy zeroes those pixels)
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
  toolVersion = "GE-Proton11-7";
  toolName    = "proton-custom-${toolVersion}";

  # false ships the pristine GE tarball (plus WineASIO) under the same tool
  # name, skipping the Wine/DXVK rebuilds entirely — for A/B verification of
  # whether the patch series is still needed on this GE base.
  overlayPatchedBinaries = true;

  # The exact Valve wine commit GE-Proton11-7 uses (from proton-ge-custom
  # git submodule `wine` at tag GE-Proton11-7).
  valveWineRev  = "46b29104e3741fe23bf5e2547196a253aab88c89";
  valveWineHash = "sha256-cXJaXVAO05SSedIodq5fkvv+OEnr9z4tldqDDqFAWyY=";

  geProtonRev  = "GE-Proton11-7";
  geProtonHash = "sha256-HEFoB0tQOnCo6Tz9WZ6SnhoZepFFZ+JWIVjYgP/i8gI=";

  wineasio64 = pkgs.wineasio;
  wineasio32Files = ../../rocksmith/assets;

  # GE-Proton binary tarball (DXVK, VKD3D-Proton, Proton scripts, mono, gecko).
  # Keep this pinned to the exact tool whose Wine tree is patched below; using
  # pkgs.proton-ge-bin.src here would silently follow nixpkgs updates.
  ge-proton-src = pkgs.fetchurl {
    url = "https://github.com/GloriousEggRoll/proton-ge-custom/releases/download/${toolVersion}/${toolVersion}-x86_64.tar.gz";
    hash = "sha256-xUSLdqIwOE4te8a+tcy5e6+34sO2xSfLA6GlRrvLAKA=";
  };

  activePatchSeries = [
    ./patches/wine-wayland-focus/0001-winewayland.drv-Bound-WM_CANCELMODE-on-keyboard-leav.patch
    ./patches/wine-wayland-layered-windows/0001-winewayland.drv-Fix-layered-surface-alpha-uploads.patch
    ./patches/ntdll-delay-load/0001-ntdll-Make-the-delay-load-IAT-writable-before-patchi.patch
    ./patches/ntdll-system-thread-signals/0001-ntdll-Record-the-libc-thread-pointer-for-system-thre.patch
    ./patches/win32u-load-driver-deadlock/0001-win32u-Bound-the-desktop-driver-readiness-wait.patch
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

      # GE's Wine patch sequence from patches/protonprep-valve-staging.sh, in
      # its order. The shipped tarball's wineserver, wined3d, user32 and the
      # rest are built from this tree, and several series here change the
      # wineserver protocol, ntuser.h and struct vulkan_funcs, so the DLLs we
      # overlay must come from the same sequence.
      #
      # The wine-staging `patchinstall.py` run is unavailable (its submodule
      # is not in the source archive) and the wineopenxr copy is skipped, so a
      # few later patches lose hunks in files we never build. A rejected hunk
      # is fatal only when it lands in an overlaid module or a shared header;
      # everything else is reported and tolerated.
      ge="$geProtonSrc/patches"
      relevant='^\+\+\+ b/(dlls/ntdll/|dlls/win32u/|dlls/winewayland\.drv/|server/|include/)'
      apply_ge() {
        local rej; rej="$(mktemp)"
        patch -p1 -s --no-backup-if-mismatch -r "$rej" < "$1" >/dev/null 2>&1 || true  # protonprep applies with default fuzz
        if [ -s "$rej" ]; then
          if grep -qE "$relevant" "$rej"; then
            echo "GE patch hunk touching an overlaid module failed: ''${1#$ge/}" >&2
            cat "$rej" >&2
            exit 1
          fi
          echo "GE patch partially applied (rejects only outside overlaid modules): ''${1#$ge/}"
        fi
        rm -f "$rej"
      }
      apply_ge_dir() { for p in $(ls "$1"/*.patch | sort); do apply_ge "$p"; done; }
      sni="$ge/wine-hotfixes/em-fixups/0001-winewayland-add-SNI-tray-icons-and-native-context-me.patch"
      apply_ge_dir "$ge/wine-hotfixes/wine-wayland"
      apply_ge "$sni"
      apply_ge_dir "$ge/wine-hotfixes/wineland-child-rendering"
      for p in $(ls "$ge"/wine-hotfixes/em-fixups/*.patch | sort); do [ "$p" = "$sni" ] || apply_ge "$p"; done
      for d in ntdll-Hide_Wine_Exports kernel32-Debugger ntdll-ext4-case-folder winex11-Window_Style \
               winex11-ime-check-thread-data winex11-Fixed-scancodes comctl32_animate_avi d3drm-starwars \
               windowscodecs-TIFF_Support mmsystem.dll16-MIDIHDR_Refcount; do
        apply_ge_dir "$ge/wine-hotfixes/wine-staging/$d"
      done
      for f in assettocorsa-hud pso2_hack vgsoh silence-starcitizen-unsupported-os eac_60101_timeout \
               layered-overlay-wine 0001-win32u-Avoid-zero-WM_ACTIVATEAPP-lparam-on-first-for \
               black-desert-keep-fullscreen-on-focus-loss maplestory-kernelbase-charprev-null \
               maplestory-spi-stickykeys-filterkeys ai-limit-dx12-compute-shader-fallback \
               max-payne-cpu-detection return-to-krondor-text-bitmap-readback nascar25-protector; do
        apply_ge "$ge/game-patches/$f.patch"
      done
      apply_ge_dir "$ge/wine-hotfixes/qcap-dshow-fixes"
      for f in urlmon-pump-thread-user-messages-during-synchronous-bind wineboot-create-sqm-machine-id \
               crypt32-pfx-record-machine-keyset-in-prov-info crypt32-pfx-use-the-container-key-spec \
               crypt32-reject-ncrypt-only-private-keys crypt32-wc3-modern-chain-engine-config \
               crypt32-wc3-trace-chain-engine-config crypt32-wc3-check-exclusive-flags-size \
               crypt32-wc3-accept-legacy-chain-engine-config crypt32-wc3-preserve-exclusive-root-and-test-layouts \
               version-GetFileVersionInfoByHandle-stub ws2_32-validate-connect-address \
               kernel32-refresh-power-status-asynchronously secur32-fallback-without-no-shuffle-extensions \
               wined3d-preserve-runtime-opengl-gpu-description winex11-use-x11-drawables-for-steam-opengl-overlay \
               winex11-keep-forza-background-windows-unmapped-on-wlroots win32u-share-selected-cursors-across-processes \
               win32u-limit-extra-swapchain-image-to-doom win32u-use-three-image-present-modes-for-hades-wayland \
               win32u-use-three-image-present-modes-for-path-of-exile ntdll-retry-native-view-allocation-with-effective-range \
               ntdll-reserve-top-down-space-for-large-address-aware-wow64 ntdll-remove-redundant-packed-split-lock \
               ntdll-prefer-native-version-resource-heuristics ntdll-keep-builtin-amd-ags-ahead-of-version-heuristic \
               ole32-clipboard-stale-handle-1-tests ole32-clipboard-stale-handle-2-fix unity_crash_hotfix \
               registry_RRF_RT_REG_SZ-RRF_RT_REG_EXPAND_SZ NCryptDecrypt_implementation \
               0009-HACK-kernel32-Spoof-GetProcAddress-of-KiUserApcDispa icuuc-icuin-forwarder-dlls \
               0001-server-Dynamically-relocate-.exes-by-default-too 0002-ntdll-allow-disabling-executable-ASLR; do
        apply_ge "$ge/wine-hotfixes/pending/$f.patch"
      done
      for f in winealsa-override-channel-count 0001-fshack-Implement-AMD-FSR-upscaler-for-fullscreen-hac \
               0001-win32u-Implement-NtGdiDdDDIQueryAdapterInfo-cases 83-nv_low_latency_wine \
               build_failure_prevention-add-nls 0001-win32u-add-env-switch-to-disable-wm-decorations \
               wine_host_block_envvar winex11-mutter-cinnamon 0001-HACK-kernelbase-allow-overriding-dlls-for-DLSS-XeSS- \
               0002-HACK-ntdll-add-optiscaler-inection-hack 0001-ntdll-Read-QueryPerformanceCounter-from-the-TSC-in-us \
               0001-ntdll-Implement-IOCTL_SERIAL_GET_DTRRTS-for-serial-dev; do
        apply_ge "$ge/proton/$f.patch"
      done
      apply_ge_dir "$ge/ge-video-rework"
      apply_ge_dir "$ge/proton-ds5-haptic"

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
        dlls/ntdll/all \
        dlls/win32u/all \
        dlls/winewayland.drv/all

      cd "$TMPDIR/wine32"
      HOME=$TMPDIR "$source_dir/configure" \
        --with-wine64="$TMPDIR/wine64" \
        --without-x \
        --disable-tests
      make -j"$NIX_BUILD_CORES" \
        dlls/ntdll/all \
        dlls/win32u/all \
        dlls/winewayland.drv/all

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
    copy_patched x86_64-unix/win32u.so
    copy_patched i386-unix/win32u.so
    copy_patched x86_64-unix/ntdll.so
    copy_patched i386-unix/ntdll.so
    copy_patched x86_64-windows/winewayland.drv
    copy_patched i386-windows/winewayland.drv
    copy_patched x86_64-windows/win32u.dll
    copy_patched i386-windows/win32u.dll
    copy_patched x86_64-windows/ntdll.dll
    copy_patched i386-windows/ntdll.dll


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
      "display_name" "proton-custom ${toolVersion} (Wayland)"
      "from_oslist"  "windows"
      "to_oslist"    "linux"
    }
  }
}
EOF

    runHook postInstall
  '';

  meta = {
    description = "${toolVersion} with five Wine Wayland/ntdll/win32u fixes and WineASIO";
    homepage    = "https://github.com/GloriousEggRoll/proton-ge-custom";
    platforms   = [ "x86_64-linux" ];
  };

  passthru = {
    wineSource = wine-proton-custom-src;
    wineArtifacts = wine-proton-custom;
  };
}
