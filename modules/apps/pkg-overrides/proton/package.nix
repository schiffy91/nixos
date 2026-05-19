# scwhine-proton — GE-Proton10-34 with winewayland.drv + CEF accel patches.
#
# Builds the EXACT wine GE-Proton10-34 uses (ValveSoftware/wine at commit
# 1729f00) with the full 504-patch GE wine-wayland hotfix series, then
# layers our scwhine patch series on top.  Replaces only the changed
# binaries (winewayland.drv unix lib + win32u.so + dcomp.dll + dxgi.dll)
# in the GE-Proton binary tarball; everything else stays as-is.
#
# Patches live under ./patches/<NN-bug-name>/*.patch, one subfolder per
# bug or feature, applied in lexical subfolder order.  Each subfolder
# carries a README explaining the bug, root cause, and fix.  Patch files
# are in `git format-patch` style (Wine upstream convention) so they can
# be submitted directly to wine-devel.
#
# Current series:
#   01  CEF startup deadlock (non-blocking second Wayland roundtrip)
#   02  Blank layered windows (pUpdateLayeredWindow + configure refresh)
#   03  SNI StatusNotifierItem systray via libdbus
#   04  Real DirectComposition impl (CEF accelerated rendering)
#   05  dxgi.CreateSwapChainForComposition HWND_MESSAGE fallback
#   06  winewayland.drv DXVK → wl_subsurface presenter bridge
#   07  win32u: load_desktop_driver cross-process deadlock
{ stdenv
, pkgs
, fetchgit
, fetchFromGitHub
, autoPatchelfHook
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

  # GE-Proton binary tarball (DXVK, VKD3D-Proton, Proton scripts, mono, gecko)
  ge-proton-src = pkgs.proton-ge-bin.src;

  # ── Source: Valve wine + 504 GE wayland patches + our 6 scwhine patches ──
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

      # Apply scwhine patch series: one subfolder per bug, lexical order.
      # Within a subfolder, patch files apply in lexical order too (mostly a
      # single 0001-*.patch but supports multi-patch series per bug).
      for p in $(ls -d ${./patches}/*/ | sort); do
        for q in $(ls "$p"*.patch 2>/dev/null | sort); do
          echo "scwhine: applying $q"
          patch -p1 < "$q"
        done
      done
    '';
  };

  # ── Build: configure wine, build only winewayland.so + kernelbase ────────
  wine-scwhine = stdenv.mkDerivation {
    pname = "wine-scwhine";
    version = toolVersion;
    src = wine-scwhine-src;

    nativeBuildInputs = with pkgs; [
      autoconf automake bison flex perl python3 pkg-config
      wayland-scanner
      pkgsCross.mingwW64.buildPackages.gcc
      pkgsCross.mingw32.buildPackages.gcc
    ];

    buildInputs = with pkgs; [
      wayland dbus libxkbcommon mesa libGL
      vulkan-headers vulkan-loader
      xorg.libX11 freetype fontconfig
    ];

    postPatch = ''
      # make_vulkan writes a cache under HOME; the nix builder's /homeless-shelter
      # is read-only, so point HOME at $TMPDIR before running it.
      HOME=$TMPDIR python3 dlls/winevulkan/make_vulkan \
        -x ${pkgs.vulkan-headers}/share/vulkan/registry/vk.xml \
        -X ${pkgs.vulkan-headers}/share/vulkan/registry/video.xml

      # Wine source ships with autogen.sh, not a pre-generated ./configure —
      # run it to produce ./configure from configure.ac.
      ./tools/make_requests || true
      HOME=$TMPDIR autoreconf -fi
    '';

    configureFlags = [
      "--enable-archs=x86_64,i386"
      "--without-x"
      "--without-freetype"
      "--disable-tests"
    ];

    # Only build the binaries our patches actually touch. The base GE-Proton
    # already ships a working kernelbase.dll(.so), and our patches don't
    # modify kernelbase — skip rebuilding it.
    buildFlags = [
      "dlls/winewayland.drv/winewayland.so"
      "dlls/dcomp/all"
      "dlls/dxgi/all"
    ];

    installPhase = ''
      mkdir -p "$out/lib/wine/x86_64-unix" "$out/lib/wine/x86_64-windows" "$out/lib/wine/i386-windows"
      cp dlls/winewayland.drv/winewayland.so           "$out/lib/wine/x86_64-unix/"
      cp dlls/dcomp/x86_64-windows/dcomp.dll           "$out/lib/wine/x86_64-windows/"
      cp dlls/dcomp/i386-windows/dcomp.dll             "$out/lib/wine/i386-windows/"
      cp dlls/dxgi/x86_64-windows/dxgi.dll             "$out/lib/wine/x86_64-windows/"
      cp dlls/dxgi/i386-windows/dxgi.dll               "$out/lib/wine/i386-windows/"
    '';

    meta.platforms = [ "x86_64-linux" ];
  };

  # ── Final compat tool: GE-Proton binary + our patched files ──────────────
in stdenv.mkDerivation {
  pname   = toolName;
  version = toolVersion;
  src     = ge-proton-src;

  nativeBuildInputs = [ autoPatchelfHook makeWrapper rsync unzip ];
  dontConfigure = true;
  dontBuild     = true;

  # GE-Proton ships its own libraries inside files/lib/... that link against
  # runtime libs (X11, alsa, pulse, gstreamer) supplied by Proton's
  # pressure-vessel sandbox at runtime, not by the host. Don't fail the build
  # over those missing deps — they're satisfied at game-launch time.
  autoPatchelfIgnoreMissingDeps = true;

  installPhase = ''
    runHook preInstall

    # Steam's programs.steam.extraCompatPackages creates a symlink:
    #   ~/.local/share/Steam/compatibilitytools.d/<pname> -> $out
    # so the compat tool files must live directly in $out/, not a subdirectory.
    mkdir -p "$out"
    cp -r . "$out/"

    # Overlay our patched binaries on top of the GE-Proton tarball.
    cp "${wine-scwhine}/lib/wine/x86_64-unix/winewayland.so" \
       "$out/files/lib/wine/x86_64-unix/winewayland.so"
    cp "${wine-scwhine}/lib/wine/x86_64-windows/dcomp.dll" \
       "$out/files/lib/wine/x86_64-windows/dcomp.dll"
    cp "${wine-scwhine}/lib/wine/i386-windows/dcomp.dll" \
       "$out/files/lib/wine/i386-windows/dcomp.dll"
    cp "${wine-scwhine}/lib/wine/x86_64-windows/dxgi.dll" \
       "$out/files/lib/wine/x86_64-windows/dxgi.dll"
    cp "${wine-scwhine}/lib/wine/i386-windows/dxgi.dll" \
       "$out/files/lib/wine/i386-windows/dxgi.dll"

    cat > "$out/compatibilitytool.vdf" <<EOF
"compatibilitytools"
{
  "compat_tools"
  {
    "${toolName}"
    {
      "install_path" "."
      "display_name" "scwhine GE-Proton10-34 (Wayland+SNI)"
      "from_oslist"  "windows"
      "to_oslist"    "linux"
    }
  }
}
EOF

    runHook postInstall
  '';

  meta = {
    description = "GE-Proton10-34 with scwhine winewayland + CEF fixes (Battle.net Wayland)";
    homepage    = "https://github.com/GloriousEggRoll/proton-ge-custom";
    platforms   = [ "x86_64-linux" ];
  };
}
