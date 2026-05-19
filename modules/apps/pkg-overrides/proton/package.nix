# scwhine-proton — GE-Proton10-34 with winewayland.drv + CEF hack patches.
#
# Builds the EXACT wine GE-Proton10-34 uses (ValveSoftware/wine at commit
# 1729f00) with the full 504-patch GE wine-wayland hotfix series, then
# layers our 4 scwhine patches on top.  Replaces only the changed binaries
# (winewayland.drv unix lib + kernelbase.dll PE/unix) in the GE-Proton
# binary tarball; everything else (DXVK, VKD3D, Proton scripts) stays as-is.
#
# Patches (in order):
#   1. Non-blocking second Wayland roundtrip — fixes CEF IPC deadlock that
#      caused KDE to grey-out the launcher window after 4 seconds.
#   2. pUpdateLayeredWindow hook + ensure_window_surface_contents after
#      WM_WAYLAND_CONFIGURE — fixes blank layered windows.
#   3. Append `--disable-direct-composition` to Battle.net.exe CEF command
#      line so CEF stops trying to use stub'd dcomp.dll.
#   4. SNI StatusNotifierItem systray via libdbus — tray icon integration
#      for KDE Plasma 6, GNOME+AppIndicator, Waybar; no XWayland.
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

  # ── Source: Valve wine + 504 GE wayland patches + our 4 scwhine patches ──
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

      # Apply our 4 scwhine patches
      for p in ${./patches}/*.patch; do
        patch -p1 < "$p"
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
      # Generate wine/vulkan.h (required by winewayland.drv/vulkan.c)
      python3 dlls/winevulkan/make_vulkan \
        -x ${pkgs.vulkan-headers}/share/vulkan/registry/vk.xml \
        -X ${pkgs.vulkan-headers}/share/vulkan/registry/video.xml
    '';

    configureFlags = [
      "--enable-archs=x86_64,i386"
      "--without-x"
      "--without-freetype"
      "--disable-tests"
    ];

    # Only build what we need
    buildFlags = [
      "dlls/winewayland.drv/winewayland.so"
      "dlls/kernelbase/all"
    ];

    installPhase = ''
      mkdir -p "$out/lib/wine/x86_64-unix" "$out/lib/wine/x86_64-windows" "$out/lib/wine/i386-windows"
      cp dlls/winewayland.drv/winewayland.so          "$out/lib/wine/x86_64-unix/"
      cp dlls/kernelbase/kernelbase.dll.so            "$out/lib/wine/x86_64-unix/"
      cp dlls/kernelbase/x86_64-windows/kernelbase.dll "$out/lib/wine/x86_64-windows/"
      cp dlls/kernelbase/i386-windows/kernelbase.dll   "$out/lib/wine/i386-windows/"
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

  installPhase = ''
    runHook preInstall

    # Steam's programs.steam.extraCompatPackages creates a symlink:
    #   ~/.local/share/Steam/compatibilitytools.d/<pname> -> $out
    # so the compat tool files must live directly in $out/, not a subdirectory.
    mkdir -p "$out"
    cp -r . "$out/"

    # Overlay our patched binaries
    cp "${wine-scwhine}/lib/wine/x86_64-unix/winewayland.so" \
       "$out/files/lib/wine/x86_64-unix/winewayland.so"
    cp "${wine-scwhine}/lib/wine/x86_64-unix/kernelbase.dll.so" \
       "$out/files/lib/wine/x86_64-unix/kernelbase.dll.so"
    cp "${wine-scwhine}/lib/wine/x86_64-windows/kernelbase.dll" \
       "$out/files/lib/wine/x86_64-windows/kernelbase.dll"
    cp "${wine-scwhine}/lib/wine/i386-windows/kernelbase.dll" \
       "$out/files/lib/wine/i386-windows/kernelbase.dll"

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
