// Steam can request a keyboard wider than the scaled Plasma work area.
// Preserve its requested height: reducing it clips Steam's fixed-height rows.
function watchSteamKeyboard(window) {
    let fitting = false;
    function fit() {
        if (fitting || window.resourceClass !== "steam" ||
            window.caption !== "Steam Input On-screen Keyboard") return;
        const area = workspace.clientArea(KWin.WorkArea, window);
        const frame = window.frameGeometry;
        const width = Math.min(frame.width, area.width);
        const height = Math.min(frame.height, area.height);
        const x = Math.max(area.x, Math.min(frame.x, area.x + area.width - width));
        const y = Math.max(area.y, Math.min(frame.y, area.y + area.height - height));
        // Fractional scaling can round geometry by a fraction of a logical pixel.
        if (Math.abs(frame.x - x) < 1 && Math.abs(frame.y - y) < 1 &&
            Math.abs(frame.width - width) < 1 && Math.abs(frame.height - height) < 1) return;
        fitting = true;
        window.frameGeometry = {x, y, width, height};
        fitting = false;
    }
    window.captionChanged.connect(fit);
    window.frameGeometryChanged.connect(fit);
    fit();
}
workspace.windowAdded.connect(watchSteamKeyboard);
workspace.windowList().forEach(watchSteamKeyboard);
