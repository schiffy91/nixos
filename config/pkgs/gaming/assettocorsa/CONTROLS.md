# Wheel control acceptance

Target: original AC with CM/CSP, Logitech PRO PS/PC wheel, pedals and
H-shifter. A predictable wheel-operated driving/session experience is the
goal; writing a binding or passing an isolated test does not complete it.

The shared binding records live in `logitech-pro-playstation.nix`; FRACTAL-NORTH selects that wheel profile in its `pkgs/default.nix`.
The writable calibrated preset on FRACTAL-NORTH lives in `~/Drive/media/Games/Assetto Corsa/Controls`.
The README contains the current button map. Do not restore the obsolete
L2/Share modifier map: Options must remain pause, Cross selects, and Circle
should consistently cancel in menus. Left dial selects, right dial adjusts.
The latter follows the interaction described in [GT7's update notes](https://www.gran-turismo.com/hk/gt7/news/00_5412323.html),
without claiming an exact factory PRO mapping.

## Current evidence

- Binding/preset preservation and composed-launch BTRC tests pass.
- Lua tests cover menu selection, limits, read-only values, pause/replay/menu
  isolation, Circle release before resume, optional car controls, pit strategy
  selection and display labels. CSP API doubles test adapter behavior; they
  are not a substitute for real physics or wheel input.
- The first Wheel Menu version loaded successfully in the real CSP session.
- The expanded menu and replacement-pause setting are built, tested and
  applied through `assetto-setup` with CM/Steam closed. Steam was restarted.
- The initial panel rendered in the actual Vegas/Ferrari race on the TV.
  The old AC pause menu was still tiny; the declarative settings now enable
  CSP's replacement pause menu too. After applying and restarting, the expanded
  app loaded successfully and the replacement pause menu was visually verified
  in the real game. The test session is paused for physical input verification.
- Physical inputs and the following workflows remain
  unverified unless explicitly recorded below.
- The cockpit/control expansion rendered with 24 entries in the real Ferrari
  session. This caught a stale SDK comment: the actual FOV state field is
  `firstPersonCameraFOV`, not `firstPersonFOV`. The corrected runtime is covered
  by the adapter fixture. Entry-generation failures now show an unavailable
  message and log once per distinct error; they cannot dispatch adjustments.
  Held-command cancellation, capability changes, locked mirrors and error
  recovery have regression coverage. Actual wheel-operated adjustments remain
  to be verified.

## Coverage and remaining implementation

| Capability | Current implementation | Required acceptance / remaining work |
|---|---|---|
| Steering, accelerator, brake, clutch | Saved axes; linear brake gamma | Verify endpoints, inversion, centering, rotation, soft lock, clutch engagement |
| Paddle shifts, gears 1–7/reverse | Saved/direct bindings | Exercise every gear in appropriate cars |
| Neutral, H-pattern switching | Explicit neutral command; Wheel Menu shifter mode | Verify actual gear/mode switching |
| Dual-clutch launch paddles | None | Identify reported axes and configure bite-point behavior |
| Handbrake | Circle, digital | Verify release after menus; optional analog accessory support |
| FFB and haptics | Saved FFB; native gain commands in Wheel Menu; driver/relay infrastructure | Verify/tune physical feedback and helper lifetime |
| Menu directions/select | CSP D-pad navigation + Cross | Navigate pre-race, pause, setup, results, confirmation dialogs |
| Pause/resume | Options; Circle release resumes pause or exits instant replay | Physical test; universal Back, particularly confirmation-dialog cancellation, remains incomplete |
| Start/retry/pits/setup/exit/advance | Visible game menus | Prove complete wheel-only flows; preserve confirmations |
| CM car/track/session selection and settings | Desktop mouse/keyboard | Wheel-accessible launcher workflow still missing |
| TC, ABS, brake bias, turbo | Capability-aware Wheel Menu | Verify car-specific limits and direction against real physics |
| TC2 | CSP custom hotkey API from Wheel Menu | Verify single-detent behavior and limits in a supporting car |
| Engine/fuel map | Explicit next-map command: select then press right dial | Verify real cycling; game-reported map remains source of truth |
| Starter, ignition/cutoff | Hold right dial on labelled starter/cutoff entries when CSP stalling is enabled | Verify supported physics; experimental stalling is not enabled implicitly |
| Engine braking | Wheel Menu | Verify supported car |
| KERS/DRS | L2/R2 | Verify supported car and activation restrictions |
| ERS recovery/delivery, MGU-H | Wheel Menu; named delivery programs | Verify supported hybrid car |
| Pit limiter | R3 | Verify on/off and speed behavior |
| Pit strategy, fuel, tyres, pressures, repairs, wings | Game-provided pit options; compound labels; locked options respected | Complete a real pit stop; verify quantities, no-change choices and strategy switching |
| Fuel/lap estimate | Read-only menu entry | Verify units and unknown estimate before first laps |
| Lap/gap/tyre information | Existing game apps | Provide discoverable wheel-accessible race information |
| Camera/look | Square; saved D-pad POV | Verify directions and suppression during menu navigation |
| Headlights/flash | Triangle/L3 | Verify car behavior |
| High/low beams | Wheel Menu | Verify supported car |
| Indicators/hazards/cancel | Wheel Menu | Verify supported car; assess access speed while driving |
| Wipers/off | Wheel Menu | Verify every supported speed, off, and capability filtering |
| Horn | Left dial press | Verify no accidental activation when turning dial |
| Seat/FOV/mirrors | Stationary Wheel Menu entries; native seat/FOV commands; mirror API preserves other parameters | Verify actual positions, saving and locked mirrors |
| HUD/app pages | Wheel Menu visibility and app-page controls | Verify visibility/recovery in actual game |
| Replay clip | Share | Verify saved clip |
| Replay playback/seek/speed/car/camera | CSP replay controller navigation enabled; Circle exits instant replay on release | Verify full replay UI operation and isolation from driving controls |
| Voice/chat/voting | No wheel assignments | Add practical access without intercepting driving controls |
| Audio volume/mute/output | Desktop tray | Consider scoped wheel access; no global button interception |
| Extra car switches/animations | None | Expose only car-supported, labelled controls |
| VR recenter | None | Deferred with PSVR2 work until the core rig works |

## Verification sequence

1. Check the physical axes and each button in CM with the final driver active.
2. Start a stationary practice session. Verify navigation, panel rendering,
   pause/Back, and camera/look isolation before driving.
3. Verify adjustments against the actual dashboard/telemetry in cars exposing
   each feature. Unsupported features must be absent, not plausible no-ops.
4. Drive a manual car and a paddle-shift car, then complete a pit stop.
5. Exercise retry, setup, results, replay and exit through the wheel.
6. Close/reopen through Steam; verify calibrated presets survive and no relay
   keeps the Steam session falsely active.

Do not mark the control work finished until the full workflow is supported
and observed. Keep incomplete features visible here rather than treating
unit-test success as acceptance.
