{ pkgs, lib, settings, ... }:
let
  dataDir = "/home/${settings.user.admin.username}/.local/share/nanoclaw";
  nanoclaw-setup = pkgs.writeShellScriptBin "nanoclaw-setup" ''
    set -e
    if [ ! -d "${dataDir}/.git" ]; then
      ${pkgs.git}/bin/git clone https://github.com/qwibitai/nanoclaw.git "${dataDir}"
    else
      cd "${dataDir}" && ${pkgs.git}/bin/git pull
    fi
    cd "${dataDir}"
    ${pkgs.nodejs_22}/bin/npm install
    ${pkgs.nodejs_22}/bin/npm run build
    docker build -t nanoclaw-agent:latest container/
    echo ""
    echo "Launching Claude Code for interactive setup (/setup)..."
    echo "Exit Claude Code when done to start the service."
    claude
    systemctl --user enable --now nanoclaw
    echo "NanoClaw running. Logs: journalctl --user -u nanoclaw -f"
  '';
in {
  home.packages = [ pkgs.nodejs_22 nanoclaw-setup ];
  systemd.user.services.nanoclaw = {
    Unit = {
      Description = "NanoClaw AI Assistant";
      After = [ "network-online.target" ];
      ConditionPathExists = "${dataDir}/dist";
    };
    Service = {
      Type = "simple";
      WorkingDirectory = dataDir;
      ExecStart = "${pkgs.nodejs_22}/bin/node dist/index.js";
      Restart = "on-failure";
      RestartSec = 30;
      Environment = [
        "PATH=${lib.makeBinPath [ pkgs.nodejs_22 pkgs.git ]}:/run/current-system/sw/bin"
        "NODE_ENV=production"
      ];
    };
    Install.WantedBy = [ "default.target" ];
  };
}
