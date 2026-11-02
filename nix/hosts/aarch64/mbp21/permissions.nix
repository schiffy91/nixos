# Intent only. macOS consent is applied through System Settings, never TCC writes.
# App installation and TCC authorization are separate operations.
{
  schemaVersion = 1;
  enforcement = "system-settings";
  apps = {
    codex = {
      name = "Codex (displayed as ChatGPT)";
      path = "/Applications/ChatGPT.app";
      bundleId = "com.openai.codex";
      teamId = "2DC432GLL2";
      reason = "User explicitly requires full access for computer administration.";
      allow = [
        "fullDiskAccess"
        "deviceControlAndDataAccess"
        "screenAndSystemAudioRecording"
        "localNetwork"
        "microphone"
      ];
    };
    claudeDesktop = {
      name = "Claude Desktop";
      path = "/Applications/Claude.app";
      bundleId = "com.anthropic.claudefordesktop";
      teamId = "Q6L2SF6YDW";
      reason = "User explicitly requires full access for computer administration.";
      allow = [
        "fullDiskAccess"
        "deviceControlAndDataAccess"
        "screenAndSystemAudioRecording"
        "localNetwork"
        "microphone"
      ];
    };
    codexComputerUse = {
      name = "Codex Computer Use";
      path = "/Applications/ChatGPT.app/Contents/Resources/cua_node/lib/node_modules/@oai/sky/Codex Computer Use.app";
      bundleId = "com.openai.sky.CUAService";
      teamId = "2DC432GLL2";
      reason = "Retain the existing Codex computer-control workflow.";
      allow = [
        "deviceControlAndDataAccess"
        "screenAndSystemAudioRecording"
      ];
    };
    claudeCode = {
      name = "Claude Code";
      path = "~/.local/bin/claude";
      bundleId = "com.anthropic.claude-code";
      teamId = "Q6L2SF6YDW";
      reason = "User approved standalone Claude Code Full Disk Access for computer administration.";
      allow = [
        "fullDiskAccess"
        "deviceControlAndDataAccess"
        "screenAndSystemAudioRecording"
        "localNetwork"
        "microphone"
      ];
    };
    chromeRemoteDesktop = {
      name = "Chrome Remote Desktop";
      path = "/Library/PrivilegedHelperTools/ChromeRemoteDesktopHost.app";
      bundleId = "com.google.chromeremotedesktop.me2me-host";
      teamId = "EQHXZ8M8AV";
      reason = "User confirmed continued remote access.";
      allow = [
        "deviceControlAndDataAccess"
        "screenAndSystemAudioRecording"
        "remoteDesktop"
        "localNetwork"
      ];
    };
  };
  review = [
    "Preserve Terminal FDA for CLI administration; audit it independently."
    "Review obsolete test-binary microphone grants and DaisyDisk Calendar access."
    "Resolve duplicate Local Network and generic background-task identities."
  ];
}
