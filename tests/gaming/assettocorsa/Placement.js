// Exercise the actual KWin script with event ordering and output removal.
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
function signal() {
    const handlers = new Set();
    return { connect: fn => handlers.add(fn), disconnect: fn => handlers.delete(fn),
        emit: () => [...handlers].forEach(fn => fn()) };
}
const timers = [];
class QTimer {
    constructor() { this.timeout = signal(); timers.push(this); }
    start() { this.active = true; }
    stop() { this.active = false; }
    tick() { if (this.active) this.timeout.emit(); }
}
function window(caption, resourceClass, output) {
    return { caption, resourceClass, output,
        frameGeometry: { x: -2000, y: 0, width: 1600, height: 900 },
        captionChanged: signal(), fullScreenChanged: signal(), frameGeometryChanged: signal(),
        interactiveMoveResizeStarted: signal(), closed: signal() };
}
const desk = { geometry: { x: 2195, y: 0, width: 2407, height: 1354 } };
const tv = { geometry: { x: 0, y: 83, width: 2194, height: 1234 } };
const manager = window('Content Manager (Lite mode)', 'steam_app_244210', tv);
const moves = [];
const workspace = { screens: [desk, tv], windowAdded: signal(),
    windowList: () => [manager],
    sendClientToScreen: (window, output) => { window.output = output; moves.push(output); } };
const context = vm.createContext({ workspace, QTimer });
vm.runInContext(fs.readFileSync(process.argv[2], 'utf8'), context);
const game = window('', 'steam_app_244210', desk);
context.followManager(game);
assert.equal(timers.length, 0);
game.caption = 'Assetto Corsa'; game.captionChanged.emit();
game.frameGeometryChanged.emit(); game.fullScreenChanged.emit();
assert.equal(moves.length, 0); // Do not race Wine's initial placement requests.
manager.output = desk; // The source output was captured when the race started.
timers[0].tick();
assert.equal(game.output, tv);
assert.equal(game.frameGeometry.x, 297);
assert.equal(game.frameGeometry.y, 250);
game.output = desk; game.frameGeometryChanged.emit(); game.fullScreenChanged.emit();
timers[0].tick();
assert.equal(game.output, desk); // Later manual moves are respected.
assert.equal(moves.length, 1);
context.followManager(window('Assetto Corsa Competizione', 'steam_app_805550', desk));
context.followManager(window('Assetto Corsa', 'unrelated-app', desk));
assert.equal(timers.length, 1);
const next = window('Assetto Corsa', 'steam_proton', tv);
context.followManager(next); timers[1].tick();
assert.equal(next.output, desk); // Re-read CM for each new race.
for (const event of ['interactiveMoveResizeStarted', 'closed']) {
    const cancelled = window('Assetto Corsa', 'steam_app_244210', tv);
    context.followManager(cancelled); cancelled[event].emit(); timers.at(-1).tick();
    assert.equal(cancelled.output, tv);
}
const pending = window('Assetto Corsa', 'steam_app_244210', tv);
context.followManager(pending); workspace.screens = [tv]; timers.at(-1).tick();
assert.equal(pending.output, tv); // Never send to a disconnected output.
workspace.windowList = () => [];
const before = timers.length;
context.followManager(window('Assetto Corsa', 'steam_app_244210', tv));
assert.equal(timers.length, before);
console.log('PASS: AC follows CM, startup geometry, manual moves, hotplug, app isolation');
