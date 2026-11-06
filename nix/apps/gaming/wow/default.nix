{ config, pkgs, lib, ... }:
let
  cfg = config.settings.apps.wow;
  enabled = config.settings.apps.enable
    && config.settings.apps.gaming.enable
    && config.settings.apps.battlenet.enable
    && cfg.enable
    && pkgs.stdenv.hostPlatform.isx86_64;
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  forever = "/home/${user}/Games/Battle.net/prefix/drive_c/Program Files (x86)/World of Warcraft/_forever_";
  lock = builtins.fromJSON (builtins.readFile ./addons.lock.json);  # wow-addons-lock refreshes this
  # The lock stores the zip's own sha256, so refreshing it needs nothing but a download.
  fetch = name: pin: pkgs.runCommand "wow-addon-${name}" {
    nativeBuildInputs = [ pkgs.unzip ];
  } ''
    mkdir -p "$out" && cd "$out"
    unzip -q ${pkgs.fetchurl { inherit (pin) url hash; }}  # a release zip holds one or more addon folders
  '';
  addons = pkgs.symlinkJoin {
    name = "wow-forever-addons";
    paths = lib.mapAttrsToList fetch lock;
  };
  # Refreshes addons.lock.json from each repo's newest release. nixosctl upgrade runs it.
  lockTool = pkgs.writeShellApplication {
    name = "wow-addons-lock";
    runtimeInputs = [ pkgs.curl pkgs.jq pkgs.nix pkgs.git pkgs.coreutils pkgs.gnused pkgs.gnugrep ];
    text = ''
      root="''${NIXOS_CONFIG_ROOT:-/etc/nixos}/nix/apps/gaming/wow"
      sources="$root/addons.json"
      flavours="bcc cata classic forever mists titan titanreforged wrath retail nolib"
      result='{}'
      for name in $(jq -r 'keys[]' "$sources"); do
        repo=$(jq -r --arg n "$name" '.[$n].repo' "$sources")
        train=$(jq -r --arg n "$name" '.[$n].train // ""' "$sources")
        want=$(jq -r --arg n "$name" '.[$n].flavour // ""' "$sources")
        if [ -n "$train" ]; then  # the feed carries only the newest ten, so ask git for every tag
          tags=$(git ls-remote --tags "https://github.com/$repo" "refs/tags/$train/*" \
            | sed 's|.*refs/tags/||' | grep -v '\^{}$' | grep -E "^$train/[^/]+$" | sort -V | tac)
        else
          tags=$(curl -fsSL "https://github.com/$repo/releases.atom" \
            | grep -oE 'releases/tag/[^"]+' | sed 's|releases/tag/||; s|%2F|/|g; s|%2B|+|g')
        fi
        [ -n "$tags" ] || { echo "$name: no release tags" >&2; exit 1; }
        href=""; tag=""
        for candidate_tag in $tags; do  # a release can carry no zip at all, so walk until one does
          assets=$(curl -fsSL "https://github.com/$repo/releases/expanded_assets/$candidate_tag" \
            | grep -oE "/$repo/releases/download/[^\"]+\.zip" | sort -u || true)
          for candidate in $assets; do
            file=$(basename "$candidate" .zip)
            suffix=''${file##*-}
            matches=no
            for flavour in $flavours; do [ "$suffix" = "$flavour" ] && matches=yes; done
            if [ -n "$want" ]; then [ "$suffix" = "$want" ] && href="$candidate"
            elif [ "$matches" = no ]; then href="$candidate"; fi
          done
          if [ -n "$href" ]; then tag="$candidate_tag"; break; fi
        done
        [ -n "$href" ] || { echo "$name: no asset for ''${want:-base} in any release" >&2; exit 1; }
        url="https://github.com$href"
        hash=$(nix store prefetch-file --json --hash-type sha256 "$url" | jq -r .hash)
        echo "$name $tag" >&2
        result=$(printf '%s' "$result" | jq --arg n "$name" --arg u "$url" --arg h "$hash" \
          '.[$n] = {url: $u, hash: $h}')
      done
      printf '%s\n' "$result" | jq -S . > "$root/addons.lock.json"
    '';
  };
in lib.mkIf enabled {
  environment.systemPackages = [ lockTool ];
  environment.etc."${config.settings.apps.nixosctl.upgradeHooksDir}/wow-addons".source = "${lockTool}/bin/wow-addons-lock";
  system.activationScripts.wowAddons = lib.stringAfter [ "users" ] ''
    if [ -d "${forever}" ]; then
      ${pkgs.coreutils}/bin/install -d -o ${user} -g ${group} "${forever}/Interface"
      target="${forever}/Interface/AddOns"
      if [ ! -L "$target" ]; then ${pkgs.coreutils}/bin/rm -rf "$target"; fi  # drop any hand-installed tree
      ${pkgs.coreutils}/bin/ln -sfn ${addons} "$target"
      ${pkgs.coreutils}/bin/chown -h ${user}:${group} "$target"
    fi
  '';
}
