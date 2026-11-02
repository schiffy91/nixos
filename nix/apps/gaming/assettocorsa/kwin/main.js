// KWin owns output placement; Wine fullscreen selection does not follow CM.
function isAssetto(window) {
    return window.resourceClass === "steam_app_244210" || window.resourceClass === "steam_proton";
}

function followManager(window) {
    if (!isAssetto(window)) return;
    function whenNamed() {
        if (window.caption !== "Assetto Corsa") return;
        window.captionChanged.disconnect(whenNamed);
        const manager = workspace.windowList().find(candidate =>
            isAssetto(candidate) && candidate.caption.indexOf("Content Manager") === 0);
        if (!manager) return;
        const output = manager.output;
        // Wine applies several ConfigureRequests while creating the window.
        // Wait for them to settle before moving it, then relinquish placement.
        const settled = new QTimer();
        settled.interval = 250;
        settled.singleShot = true;
        function restart() { settled.start(); }
        function stop() {
            settled.stop();
            window.frameGeometryChanged.disconnect(restart);
            window.fullScreenChanged.disconnect(restart);
            window.interactiveMoveResizeStarted.disconnect(stop);
            window.closed.disconnect(stop);
        }
        settled.timeout.connect(function () {
            stop();
            if (workspace.screens.indexOf(output) === -1) return;
            workspace.sendClientToScreen(window, output);
            const frame = window.frameGeometry;
            const screen = output.geometry;
            window.frameGeometry = {
                x: screen.x + Math.max(0, (screen.width - frame.width) / 2),
                y: screen.y + Math.max(0, (screen.height - frame.height) / 2),
                width: frame.width, height: frame.height
            };
        });
        window.frameGeometryChanged.connect(restart);
        window.fullScreenChanged.connect(restart);
        window.interactiveMoveResizeStarted.connect(stop);
        window.closed.connect(stop);
        restart();
    }
    window.captionChanged.connect(whenNamed);
    whenNamed();
}

workspace.windowAdded.connect(followManager);
workspace.windowList().forEach(followManager);
