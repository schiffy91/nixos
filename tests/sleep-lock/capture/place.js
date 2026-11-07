// Fixture placement after output hotplug/layout changes. Wayland clients cannot
// control their global position; use KWin's supported per-output operation.
const prefix = "Sleep capture target: ";
let physicalWindow = null;
for (const window of workspace.windowList()) {
    if (!window.caption.startsWith(prefix)) {
        continue;
    }
    const name = window.caption.substring(prefix.length);
    const output = workspace.screens.find(candidate => candidate.name === name);
    if (!output) {
        throw new Error("Fixture target output is absent: " + name);
    }
    workspace.sendClientToScreen(window, output);
    window.fullScreen = true;
    if (name !== "Virtual-Sleep capture test") {
        physicalWindow = window;
    }
    print("CAPTURE_PLACED " + window.caption + " on " + window.output.name);
}
if (!physicalWindow) {
    throw new Error("Fixture physical-output window is absent");
}
// The fixture QLineEdit already owns widget focus. Select its top-level window
// after unlocked hotplug so fresh keyboard input cannot land on Plasma instead.
workspace.activeWindow = physicalWindow;
print("CAPTURE_FOCUSED " + physicalWindow.caption + " on " + physicalWindow.output.name);
