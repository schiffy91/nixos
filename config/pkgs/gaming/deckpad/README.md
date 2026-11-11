# Deckpad: the Steam Deck as a Steam Controller

`deckpad` makes the Deck's built-in controls appear on another machine as a
2026 Steam Controller. Steam there sees both sticks, both trackpads, the four
back buttons, gyro and stick touch, and drives rumble and trackpad haptics,
all through Steam Input. It reaches the other machine three ways:

| Link | The other machine sees | It needs |
|---|---|---|
| Network (Wi-Fi, Ethernet, localhost) | A wired controller (`28de:1302`), through its kernel's virtual USB host controller | `deckpad attach` running; a paired key |
| USB-C cable | A wired controller (`28de:1302`) | Nothing. The Deck's port must be in device mode |
| Bluetooth | A Bluetooth controller (`28de:1303`) | Nothing. It pairs like any gamepad |

It is written in BTRC on the standard library's `USB`, `HID`, `Bluetooth`,
`Network`, `Crypto` and `HTTP` groups. Beyond libc it links libsodium (keys
and encryption on the network) and libdbus (BlueZ, and the tray).

## Using it

On the Deck, start the service with the **Deck as Controller** shortcut (Game
Mode) or the tray checkbox (Plasma). The Deck keeps its own controls until a
host attaches, and gets them back when the host leaves, the service stops, or
you hold **⋯** for two seconds.

A new host has to be let in once. Start **Deck as Controller: Pair** on the
Deck, which opens a two-minute pairing window, and then:

- **Network:** on the PC run `sudo deckpad pair STEAM-DECK-OLED.local`. Both
  sides show a six-digit code (`deckpad status` on the Deck); if they match,
  answer `y`. From then on the PC attaches by itself whenever the Deck serves.
- **Bluetooth:** pair "Steam Controller" from the PC's Bluetooth settings.
- **USB-C:** nothing. Whoever holds the cable holds the controller.

Machines whose keys are in `trustedKeys` never need the network step.

```
deckpad status                 links, who holds the controller, the pairing window, trusted hosts
deckpad pairing [SECONDS|off]  open or close the pairing window
deckpad trust KEYLINE          trust a host by its public key line
deckpad forget FINGERPRINT     stop trusting a host
deckpad key                    this machine's public key line
deckpad release                give the pad and the USB port back after a crash
```

## How it works

One `VirtualController` turns the Deck's pad into the Steam Controller's HID
reports. It implements the standard library's `IHIDDevice`, and each link is
a class that presents that device over one bus and decides who may use it:

| Layer | Class | Knows about |
|---|---|---|
| Input | `IPadSource` (`DeckPad`, `ScriptedPad`) | Where pad state comes from and where haptics go. `DeckPad` reads the Deck's controller (`28de:1205`) through `Library.USB`, taking its interfaces from the kernel while a host is attached. |
| Personality | `SteamController` | The Steam Controller's reports and feature commands. No transport. |
| Controller | `VirtualController` | One pad behind one personality, as an `IHIDDevice`: rumble timing, the battery report, the stop chord. |
| Link | `IControllerLink` | One way for a host to arrive: `NetworkLink`, `GadgetLink`, `BluetoothLink`. |
| Trust | `Pairing`, `MachineIdentity` | Trusted keys and the pairing window that all links share. |
| Service | `ControllerService` | Waits on every link at once. The first host to attach, on any link, holds the controller until it leaves; a link that cannot work on this machine is left out. |
| Control | `ControlInterface` | Status and pairing over HTTP on the loopback address, for the `deckpad` command and the tray. |

**Network.** The host connects and both sides prove an Ed25519 key over
`Library.Crypto`'s `SecureChannel`. A machine's key is its SSH host key, so
the line in `known_hosts` is the line to trust. The Deck serves a host whose
key it trusts, and takes in a new one only while the pairing window is open.
USB/IP then runs inside the encrypted channel: on the PC, `deckpad attach`
hands one end of a socket pair to the kernel's `vhci_hcd` and relays the other
through the channel, so the kernel sees a USB device and the network sees
ciphertext.

**USB-C cable.** `Library.USB`'s `USBGadget` puts the same USB device on the
Deck's own port. The Deck's port is a host port unless the BIOS option *USB
Dual Role Device* is set to *DRD*; that setting also stops the Deck booting
from a USB drive. Without it the link reports "no USB device controller" and
the other links carry on.

**Bluetooth.** `Library.Bluetooth` serves the controller over the HID
profile. The adapter is discoverable and lets hosts pair only while the
pairing window is open; afterwards only bonded hosts are served. The state
report travels as report `0x45`, as the real controller sends it over
Bluetooth. Enabling this link gives the Deck's adapter the controller's
identity and turns off the Deck's own Bluetooth input devices, because
BlueZ's input plugin holds the channels a controller serves.

**Latency.** Nothing is queued. The service reads the pad, and in the same
loop turn each link sends the newest state; a link that is behind skips a
report instead of sending a stale one. Over the network that is one pad
report per 4 ms with Nagle off.

## Configuration

```nix
# The Deck
settings.apps.gaming.deckpad = {
  enable = true;
  role = "controller";
  usb = true;                 # needs the BIOS setting above
  bluetooth = true;           # see the Bluetooth note above
  trustedKeys = [ "ssh-ed25519 AAAA... FRACTAL-NORTH" ];
};
# The PC (role "host")
settings.apps.gaming.deckpad = {
  enable = true;
  trustedKeys = [ "ssh-ed25519 AAAA... STEAM-DECK-OLED" ];
};
```

`deckpad key` prints a machine's line. The controller role opens the port
(3240) through `settings.networking.ports.tcp`; `allow` narrows which
networks may connect at all. The host role loads `vhci-hcd` and keeps trying
`controller` (`STEAM-DECK-OLED.local`). Trusted keys live in
`/var/lib/deckpad`, which is persisted.

## What is verified

- `tests/gaming/deckpad` (in the gaming check farm): report parsing and
  encoding, descriptors, feature commands, haptic translation, rumble timing,
  the stop chord, the USB and Bluetooth views of the controller, trust and the
  pairing window, and link arbitration. The server test runs the real program
  and is a host for it over loopback: refused before pairing, paired while the
  window is open, attached through the encrypted tunnel, then enumeration,
  250 reports a second in order with a bound on their age, outputs,
  reconnecting, a second host turned away, and forgetting.
- `tests/gaming/deckpad/kernel.nix` (VMs, run by hand): each link against a
  real kernel. Two machines pair and the PC's `vhci_hcd` presents the
  controller, whose reports arrive on its hidraw node, and comes back by
  itself after the Deck's service restarts. The gadget is enumerated by a
  host controller wired to it in the same kernel, and a killed service leaves
  the port clean. Over two emulated Bluetooth controllers a host pairs during
  the window and reads the controller. And the Deck's own pad is emulated
  (`FakeDeck.btrc`: the pad's USB identity, three interfaces and state
  reports, which the kernel's `hid-steam` binds as it does the real one):
  the service takes it from the kernel, puts it in gamepad mode, its held
  buttons arrive on the PC as the controller's, reading it does not cancel
  transfers, and the kernel gets it back when the host leaves.
- The standard library's own tests cover the layers underneath: USB/IP, the
  gadget at high speed and SuperSpeed, Bluetooth pairing rules, the secure
  channel and OpenSSH key formats.

Not verified without hardware: the real pad's firmware (the emulated one
answers the same requests but is not it), the Deck's port in device mode, a
real Bluetooth radio, and how Steam treats the controller on each link. The report layouts follow SDL and `hid-steam`; the
Deck-to-controller mapping has only been shown working over Bluetooth, by the
project credited below. On Linux 6.18 `hid-generic` binds `28de:1302`: that
kernel's `hid-steam` does not know it yet, so there is no kernel gamepad
without Steam, which reads hidraw itself.

The network channel's construction (SIGMA-I over libsodium) has not been
audited.

## References

- [SDL's Steam Controller driver](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_steam_triton.c)
  and Linux's [`hid-steam`](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-steam.c)
  for the report layouts.
- [deck-as-controller](https://github.com/jmedina21/deck-as-controller) (MIT)
  for the Deck-to-controller mapping, the pad grab, the haptic translation and
  the Bluetooth HID approach.
- [SISR](https://github.com/Alia5/SISR) and VIIPER for USB/IP as a controller
  transport.
- The kernel's [USB/IP protocol](https://docs.kernel.org/usb/usbip_protocol.html)
  and [FunctionFS](https://docs.kernel.org/usb/functionfs.html).
