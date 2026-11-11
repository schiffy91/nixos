{ pkgs, inputs }:
let
  addon = pkgs.runCommand "wow-addon-fixture" {} ''
    mkdir -p "$out/Details" "$out/Questie"
    printf new > "$out/Details/version"
  '';
  bundle = import ../../../config/pkgs/gaming/wow/bundle.nix { inherit pkgs; addons = [ addon ]; };
  duplicate = import ../../../config/pkgs/gaming/wow/bundle.nix { inherit pkgs; addons = [ addon addon ]; };
  legacy = pkgs.runCommand "wow-forever-addons" {} ''
    mkdir -p "$out/Details"
    printf old > "$out/Details/version"
  '';
  options = import ./options.nix { inherit pkgs inputs; };
  tools = import ../../../config/pkgs/gaming/wow/package.nix { inherit pkgs inputs; };
  programs = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "wow-lock-contract-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../../../config/pkgs/gaming/wow ../../../ci/Main.btrc ../../../ci/lib ../../../ci/native.h ../../../ci/btrc.toml ../../../ci/btrc.lock ../../lib ./LockTests.btrc ./RefreshTests.btrc ./RefreshFixture.btrc ./InstallTests.btrc ./SettingsTests.btrc ./btrc.toml ./btrc.lock ];
    };
    programs = {
      wow-addons-lock = "config/pkgs/gaming/wow/Main.btrc";
      wow-lock-tests = "tests/gaming/wow/LockTests.btrc";
      wow-refresh-tests = "tests/gaming/wow/RefreshTests.btrc";
      wow-refresh-fixture = "tests/gaming/wow/RefreshFixture.btrc";
      wow-install-tests = "tests/gaming/wow/InstallTests.btrc";
      wow-settings-tests = "tests/gaming/wow/SettingsTests.btrc";
    };
  };
in assert options; pkgs.runCommand "wow-contract-check" { nativeBuildInputs = [ pkgs.coreutils ]; } ''
  ${pkgs.lua5_1}/bin/lua ${./DisplayLayoutTests.lua} ${../../../config/pkgs/gaming/wow/display-layout.lua}
  ${pkgs.lua5_1}/bin/lua ${./LayoutGeometryTests.lua} ${../../../config/pkgs/gaming/wow/settings/SavedVariables/FlareUI.lua}
  ${programs}/bin/wow-lock-tests
  ${programs}/bin/wow-refresh-tests ${programs}/bin/wow-addons-lock ${programs}/bin/wow-refresh-fixture
  ${programs}/bin/wow-install-tests ${tools}/bin/wow-addons-install ${bundle} ${legacy}
  ${programs}/bin/wow-settings-tests ${tools}/bin/wow-settings
  if out="$TMPDIR/duplicate" bash -e ${pkgs.writeText "duplicate-addons.sh" duplicate.buildCommand}; then
    echo "Duplicate addon directories must fail the bundle build" >&2
    exit 1
  fi
  touch "$out"
''
