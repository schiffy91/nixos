# Deckpad: the Steam Deck as a Steam Controller

`deckpad` makes the Deck's built-in controls appear on another Linux machine as
a wired 2026 Steam Controller (`28de:1302`). Steam on that machine sees both
sticks, both trackpads, the four back buttons, gyro and stick touch, and drives
rumble and trackpad haptics, all through Steam Input. It is written in BTRC and
links nothing beyond libc; the tray switch adds libdbus.

## How it works

| Part | Runs on | Does |
|---|---|---|
| `deckpad serve` | Deck | Speaks USB/IP as a server. When a host imports the controller it unbinds `usbhid` from the Deck's pad (`28de:1205`), reads it over usbfs and answers as the Steam Controller. |
| `deckpad attach HOST --watch` | PC | Imports the controller and hands the connection to the kernel's `vhci_hcd`, which presents a USB device. This is `usbip attach` without the usbip tools. |
| `deckpad-toggle`, `deckpad-tray` | Deck | Start or stop the service: a Steam shortcut in Game Mode, a tray checkbox in Plasma. |

The Deck keeps its controls until a host attaches, and gets them back when the
host leaves, the service stops, or you hold **⋯** for two seconds. `deckpad
release` (also the unit's `ExecStopPost`) rebinds `usbhid` after a crash.

## Layers

How a host reaches the controller is an implementation detail of one class.
Everything above it is shared.

| Layer | Class | Knows about |
|---|---|---|
| Input | `IPadSource` (`DeckPad`, `ScriptedPad`) | Where pad state comes from and where haptics go. |
| Personality | `SteamController` | The Steam Controller's HID reports and feature commands. No transport. |
| Controller | `VirtualController` | One pad behind one personality: input reports out, output and feature reports in, rumble timing, the stop chord. One host holds it at a time. |
| USB framing | `UsbHidDevice` | USB descriptors and control transfers around the controller. Shared by every USB-shaped link. |
| Link | `IControllerLink` (`UsbIpLink`) | One way for a host to arrive. It asks its pairing before admitting anyone. |
| Pairing | `IPairing` (`AddressPairing`) | Whether a peer is trusted; how trust came about is not the link's concern. |
| Service | `ControllerService` | Waits on every open link at once; the first host to attach, on any link, gets the controller. |

The network is the only link so far. A USB gadget link would wrap the same
`UsbHidDevice` and trust whoever holds the cable; a Bluetooth link would talk to
`VirtualController` directly, since Bluetooth HID has no USB framing, and leave
pairing to the Bluetooth stack. On the receiving side only the network needs a
helper (`LanHost`, over `VhciPort`); USB and Bluetooth hosts need nothing.

## Configuration

```nix
settings.apps.gaming.deckpad = { enable = true; role = "controller"; };  # the Deck
settings.apps.gaming.deckpad.enable = true;                              # the PC (role "host")
```

The controller role opens the USB/IP port (3240) through
`settings.networking.ports.tcp`, so only trusted networks reach it. USB/IP has
no authentication or encryption: anyone who can reach the port while the
service runs can take the controls. `allow` narrows that to addresses or
networks. The host role loads `vhci-hcd` and keeps trying `controller`
(`STEAM-DECK-OLED.local`) every two seconds.

## What is verified

- `tests/gaming/deckpad` (in the gaming check farm): report parsing and
  encoding, descriptors, feature commands, haptic translation, rumble timing,
  the stop chord, and the real program driven as a USB/IP host over loopback.
- `tests/gaming/deckpad/kernel.nix` (a VM, run by hand): `vhci_hcd` attaches the
  served controller, the kernel's USB and HID stack enumerates it, state reports
  arrive on its hidraw node, and detaching returns the controls. On Linux 6.18
  `hid-generic` binds it: that kernel's `hid-steam` does not know `28de:1302`
  yet, so there is no kernel gamepad without Steam, which reads hidraw itself.

Not verified without hardware: reading the Deck's own pad over usbfs, and how
Steam treats the controller. The report layout over USB follows SDL and
`hid-steam`; the Deck-to-controller mapping has only been shown working over
Bluetooth, by the project credited below.

## References

- [SDL's Steam Controller driver](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_steam_triton.c)
  and Linux's [`hid-steam`](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-steam.c)
  for the report layouts.
- [deck-as-controller](https://github.com/jmedina21/deck-as-controller) (MIT)
  for the Deck-to-controller mapping, the usbfs grab and the haptic translation.
- The kernel's [USB/IP protocol](https://docs.kernel.org/usb/usbip_protocol.html).
