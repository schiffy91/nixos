# scwhine-proton - GE-Proton10-34 with winewayland.drv cleanup patches.
#
# Builds the exact Wine and DXVK revisions GE-Proton10-34 uses, then layers
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
#   Blank layered windows (pUpdateLayeredWindow only)
#   SNI StatusNotifierItem systray via libdbus (winewayland dock bridge)
#   DComp/DXGI/winewayland GPU presentation path
#   winevulkan, winewayland, and win32u PE/Unix pairs rebuilt from the same Wine source
#   DXVK Battle.net composition swap-chain profile
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
  toolVersion = "GE-Proton10-34";
  toolName    = "scwhine-${toolVersion}";

  # The exact Valve wine commit GE-Proton10-34 uses (from proton-ge-custom
  # git submodule `wine` at tag GE-Proton10-34).
  valveWineRev  = "1729f00e17e879f98f9df1f2bca86bc5d21a65df";
  valveWineHash = "sha256-fml7rvOve6xpqDMzMCcA6JJ4P6pOhAGOnHCn9eVM67E=";

  geProtonRev  = "GE-Proton10-34";
  geProtonHash = "sha256-vHwVEJGGzb9vRPu2XNu2pjuovcZRqfLyLRLe3IT2UoQ=";

  dxvkVersion = "v2.7.1-509-g1676dcaf";
  dxvkRev     = "1676dcaf342a9b13af86c0464ad46235687727a6";
  dxvkHash    = "sha256-wnxOLWRcXJyCsQ1xaFgnrmQrfpq+O1vgJh+f7sa0qZg=";

  # GE-Proton binary tarball (DXVK, VKD3D-Proton, Proton scripts, mono, gecko).
  # Keep this pinned to the exact tool whose Wine tree is patched below; using
  # pkgs.proton-ge-bin.src here would silently follow nixpkgs updates.
  ge-proton-src = pkgs.fetchurl {
    url = "https://github.com/GloriousEggRoll/proton-ge-custom/releases/download/${toolVersion}/${toolVersion}.tar.gz";
    hash = "sha256-UcWAtmqDPHOZj+APBxfurFcZdlQECi8u1RiePuaNdz0=";
  };

  activePatchSeries = [
    ./patches/wine-wayland-roundtrip/0001-winewayland.drv-Avoid-second-init-roundtrip.patch
    ./patches/wine-wayland-layered-windows/0001-winewayland.drv-Hook-UpdateLayeredWindow.patch
    ./patches/wine-wayland-layered-windows/0002-winewayland.drv-Set-layered-surface-alpha-bits.patch
    ./patches/wine-wayland-status-notifier/0001-winewayland.drv-Add-StatusNotifierItem-tray-support.patch
    ./patches/wine-wayland-status-notifier/0002-winewayland.drv-Polish-SNI-context-menu-callbacks.patch
    ./patches/wine-wayland-status-notifier/0003-explorer-Forward-docked-tray-icon-updates.patch
    ./patches/dcomp-wayland-gpu-present/0001-dcomp-Implement-D3D11-backed-desktop-composition.patch
    ./patches/dcomp-wayland-gpu-present/0002-dcomp-Clip-composition-host-windows-to-the-target-cl.patch
    ./patches/dcomp-wayland-gpu-present/0003-dcomp-Do-not-mark-composition-host-windows-transpare.patch
    ./patches/dcomp-wayland-gpu-present/0004-dcomp-Make-composition-host-windows-presentation-onl.patch
    ./patches/dcomp-wayland-gpu-present/0005-dcomp-Keep-composition-hosts-above-target.patch
    ./patches/dcomp-wayland-gpu-present/0006-dxgi-Create-a-hidden-swap-chain-for-composition.patch
    ./patches/dcomp-wayland-gpu-present/0007-dcomp-Present-IDCompositionSurface-content.patch
    ./patches/dcomp-wayland-gpu-present/0008-dcomp-Handle-incremental-surface-draws.patch
    ./patches/dcomp-wayland-gpu-present/0009-dcomp-Clip-composition-hosts-with-window-regions.patch
    ./patches/dcomp-wayland-gpu-present/0010-dcomp-Host-placed-composition-swap-chain-visuals.patch
    ./patches/dcomp-wayland-gpu-present/0011-dcomp-Avoid-redundant-composition-host-updates.patch
    ./patches/dcomp-wayland-gpu-present/0012-dcomp-Implement-GPU-backed-surface-factories.patch
    ./patches/dcomp-wayland-gpu-present/0013-dcomp-Unbind-removed-composition-hosts.patch
    ./patches/dcomp-wayland-gpu-present/0014-dcomp-Implement-virtual-surfaces-and-scrolling.patch
    ./patches/dcomp-wayland-gpu-present/0015-dcomp-Unbind-composition-targets-before-destroying-.patch
    ./patches/win32u-load-driver-deadlock/0001-win32u-Bound-desktop-driver-readiness-wait.patch
    ./patches/win32u-shared-gpu-resource/0001-win32u-Open-D3DKMT-shared-GPU-resources.patch
  ];

  dxvkPatchSeries = [
    ./patches/dxvk-battlenet-composition/0001-dxgi-Enable-dummy-composition-swapchain-for-Battle.n.patch
    ./patches/dxvk-battlenet-composition/0002-d3d11-Pace-composition-swap-chains-with-the-composi.patch
    ./patches/dxvk-battlenet-composition/0003-d3d11-Dither-composition-swap-chain-presents.patch
    ./patches/dxvk-battlenet-composition/0004-d3d11-Allow-limiting-shared-resource-tier.patch
  ];

  applyActivePatchSeries = pkgs.lib.concatMapStringsSep "\n" (patchFile: ''
      echo "scwhine: applying ${patchFile}"
      patch -p1 < ${patchFile}
  '') activePatchSeries;

  # -- Source: Valve wine + GE wayland patches + active cleanup series -------
  wine-scwhine-src = stdenv.mkDerivation {
    pname = "wine-scwhine-src";
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

    '';
  };

  # -- Build: configure wine and build only artifacts touched by active patches
  wine-scwhine = stdenv_32bit.mkDerivation {
    pname = "wine-scwhine";
    version = toolVersion;
    src = wine-scwhine-src;

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
      HOME=$TMPDIR python3 dlls/winevulkan/make_vulkan \
        -x ${pkgs.vulkan-headers}/share/vulkan/registry/vk.xml \
        -X ${pkgs.vulkan-headers}/share/vulkan/registry/video.xml

      # Wine source ships with autogen.sh, not a pre-generated ./configure -
      # run it to produce ./configure from configure.ac.
      ./tools/make_requests || true
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
      copy_required x86_64-windows/explorer.exe \
        "$wine64_build/programs/explorer/x86_64-windows/explorer.exe" \
        "$wine64_build/programs/explorer/explorer.exe"
      copy_required i386-windows/explorer.exe \
        "$wine32_build/programs/explorer/i386-windows/explorer.exe" \
        "$wine32_build/programs/explorer/explorer.exe"
    '';

    meta.platforms = [ "x86_64-linux" ];
  };

  dxvk-scwhine = stdenv.mkDerivation {
    pname = "dxvk-scwhine";
    version = dxvkVersion;

    src = fetchgit {
      url = "https://github.com/ValveSoftware/dxvk";
      rev = dxvkRev;
      hash = dxvkHash;
      fetchSubmodules = true;
    };

    patches = dxvkPatchSeries;

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

    # Overlay our patched binaries on top of the GE-Proton tarball.
    copy_patched() {
      local rel="$1"
      if [ ! -e "${wine-scwhine}/lib/wine/$rel" ]; then
        echo "missing patched artifact from wine-scwhine: $rel" >&2
        return 1
      fi
      if [ ! -e "$out/files/lib/wine/$rel" ]; then
        echo "GE-Proton tarball does not contain expected artifact: $rel" >&2
        return 1
      fi
      cp "${wine-scwhine}/lib/wine/$rel" "$out/files/lib/wine/$rel"
    }

    copy_patched x86_64-unix/winewayland.so
    copy_patched i386-unix/winewayland.so
    copy_patched x86_64-unix/winevulkan.so
    copy_patched i386-unix/winevulkan.so
    copy_patched x86_64-unix/win32u.so
    copy_patched i386-unix/win32u.so
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
    copy_patched x86_64-windows/explorer.exe
    copy_patched i386-windows/explorer.exe

    cp "${wine-scwhine}/lib/wine/x86_64-windows/explorer.exe" \
      "$out/files/share/default_pfx/drive_c/windows/explorer.exe"
    cp "${wine-scwhine}/lib/wine/x86_64-windows/explorer.exe" \
      "$out/files/share/default_pfx/drive_c/windows/system32/explorer.exe"
    cp "${wine-scwhine}/lib/wine/i386-windows/explorer.exe" \
      "$out/files/share/default_pfx/drive_c/windows/syswow64/explorer.exe"
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

    copy_dxvk "${dxvk-scwhine}/x64/dxgi.dll" x86_64-windows/dxgi.dll
    copy_dxvk "${dxvk-scwhine}/x32/dxgi.dll" i386-windows/dxgi.dll
    copy_dxvk "${dxvk-scwhine}/x64/d3d11.dll" x86_64-windows/d3d11.dll
    copy_dxvk "${dxvk-scwhine}/x32/d3d11.dll" i386-windows/d3d11.dll
    cp "${dxvk-scwhine}/version" "$out/files/lib/wine/dxvk/version"

    cat > "$out/compatibilitytool.vdf" <<EOF
"compatibilitytools"
{
  "compat_tools"
  {
    "${toolName}"
    {
      "install_path" "."
      "display_name" "scwhine GE-Proton10-34 (Wayland SNI)"
      "from_oslist"  "windows"
      "to_oslist"    "linux"
    }
  }
}
EOF

    runHook postInstall
  '';

  meta = {
    description = "GE-Proton10-34 with winewayland SNI cleanup patches (Battle.net Wayland)";
    homepage    = "https://github.com/GloriousEggRoll/proton-ge-custom";
    platforms   = [ "x86_64-linux" ];
  };

  passthru = {
    wineSource = wine-scwhine-src;
    wineArtifacts = wine-scwhine;
    dxvkArtifacts = dxvk-scwhine;
  };
}
