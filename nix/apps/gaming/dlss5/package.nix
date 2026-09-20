# DLSS5VKLayer: a Vulkan layer that hands each presented frame to NVIDIA's DLSS 5
# neural-rendering model running in a Windows helper under Proton. Upstream is
# built as-is; nothing here patches it. The NGX DLL is never packaged.
{ lib, stdenv, fetchFromGitHub, meson, ninja, pkg-config, glibc, libx11, libxi, qt6, pkgsCross
, makeWrapper, bash, coreutils, gawk, gnused, gnugrep, util-linux, pciutils, curl, python3 }:
let
  version = "0.3.1";
  src = fetchFromGitHub {
    owner = "bmitch87";
    repo = "DLSS5VKLayer";
    rev = "117c9530834d37ef8dc7b371e36cc3fb507e6b6b";
    hash = "sha256-7MXJzNgnkOmMpqFsB4DsCIXFHU9dlD+GHJW7uGObtfA=";
  };
  helper = pkgsCross.mingwW64.callPackage ({ stdenv, meson, ninja, windows }: stdenv.mkDerivation {
    pname = "dlssnr-helper-exe";
    inherit version src;
    nativeBuildInputs = [ meson ninja ];
    buildInputs = [ windows.pthreads ];
    installPhase = ''
      install -Dm755 windows/dlssnr_helper.exe $out/lib/dlssnr/helper/dlssnr_helper.exe
    '';
  }) { };
in stdenv.mkDerivation {
  pname = "dlss5vklayer";
  inherit version src;
  nativeBuildInputs = [ meson ninja pkg-config makeWrapper qt6.wrapQtAppsHook ];
  buildInputs = [ libx11 libxi qt6.qtbase ];  # hotkeys dlopen X11 at runtime
  dontWrapQtApps = true;  # only the GUI is a Qt app; the hook would wrap the helper script too
  preConfigure = ''
    export NIX_LDFLAGS="-L${glibc}/lib $NIX_LDFLAGS -L${glibc.static}/lib"  # tools link -static; keep the dynamic libc first
  '';
  installPhase = ''
    runHook preInstall
    install -Dm755 layer_linux/libVkLayer_NV_dlssnr.so $out/lib/dlssnr/layer/libVkLayer_NV_dlssnr.so
    install -Dm755 tools/dlssnr-shmctl $out/lib/dlssnr/bin/dlssnr-shmctl
    install -Dm755 tools/runner_probe $out/lib/dlssnr/bin/runner_probe
    install -Dm755 gui/dlssnr_gui $out/bin/dlssnr-gui
    wrapQtApp $out/bin/dlssnr-gui
    install -Dm755 ${helper}/lib/dlssnr/helper/dlssnr_helper.exe $out/lib/dlssnr/helper/dlssnr_helper.exe
    install -Dm755 ../dlssnr-helper $out/bin/dlssnr-helper
    ln -s ../lib/dlssnr/bin/dlssnr-shmctl $out/bin/dlssnr-shmctl
    mkdir -p $out/share/vulkan/implicit_layer.d
    sed -e "s#./libVkLayer_NV_dlssnr.so#$out/lib/dlssnr/layer/libVkLayer_NV_dlssnr.so#" \
        -e 's#"implementation_version"#"library_arch": "64",\n    "implementation_version"#' \
      ../layer_linux/manifest/VK_LAYER_NV_dlssnr.json \
      > $out/share/vulkan/implicit_layer.d/VK_LAYER_NV_dlssnr.x86_64.json
    wrapProgram $out/bin/dlssnr-helper \
      --set DLSSNR_INSTALL_DIR $out/lib/dlssnr \
      --prefix PATH : ${lib.makeBinPath [ bash coreutils gawk gnused gnugrep util-linux pciutils curl python3 ]}
    runHook postInstall
  '';
  meta = {
    description = "DLSS 5 neural rendering for Vulkan and Proton games, out of process";
    homepage = "https://github.com/bmitch87/DLSS5VKLayer";
    license = lib.licenses.agpl3Only;
    platforms = [ "x86_64-linux" ];
  };
}
