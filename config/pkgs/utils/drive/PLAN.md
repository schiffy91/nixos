# Drive: a Google Drive client for Linux, in BTRC

Status: plan for review, second draft (2026-11-13). Nothing here is built yet.

Changes from the first draft, after review:

- A not-downloaded file must fail to open when the client is not running, never read as zeros. That rules out the fanotify gate and brings back a thin FUSE layer (the "door").
- FUSE is wrapped once, properly, in the btrc standard library; the client is built on that.
- Content is fetched range by range from the first version.
- No database. The filesystem holds all state.
- Bulk readers: a small budget, then a dialog owned by the client asks before anything more downloads.
- Remote changes are pushed (through Pub/Sub, to be confirmed in M1); the change feed stays the record.

## What it should do

`~/Drive` behaves like Google Drive for desktop on the Mac:

1. The whole Drive tree is there after a restart with no network.
2. A folder marked **available offline** is fully downloaded, always present, and never evicted.
3. Opening a file that is not downloaded fetches what is read, as it is read, for any program.
4. Creating, editing, renaming, moving and deleting in `~/Drive` reaches Drive, and the reverse, within seconds while online. Changes made offline go up when the network returns.
5. A file changed on two machines is kept twice, never silently overwritten.
6. If the client is not running, a not-downloaded file cannot be opened. It never yields wrong data.
7. The Emulation library lives in place at `~/Drive/media/Games/Emulation` on every machine, so Semu needs no per-machine root.

The Mac keeps Google's own client. This is for FRACTAL-NORTH and the Deck.

## The main decision: a real tree behind a thin door

Two facts decide the shape.

**The tree is real files on the normal disk** (btrfs or ext4), in a store that only the client can reach:

- A downloaded file is an ordinary file.
- A not-downloaded file is a placeholder: a real file with the right name, size and time and no data blocks (a sparse file). A partly read file has data where ranges were fetched and holes elsewhere.
- Everything the client knows about a file is stored on the file (extended attributes), so the kernel's filesystem is the database.

**The only way into that store is the door**: a small FUSE server mounted at `~/Drive` that forwards every operation to the real tree.

- Opening a fully downloaded file is handed to the kernel (FUSE passthrough), so reads, writes and mmap run at native speed with the door out of the path.
- Reading a placeholder goes through the door, which fetches the missing range, stores it in the file, and answers.
- Because every change passes through the door, the door records intent at the moment it happens. Nothing has to be discovered afterwards by comparing snapshots.

Why FUSE after all: requirement 6. With fanotify the kernel lets every paused open through when the listener dies, and with no listener there is no gate at all, so a placeholder reads as zeros. The 7.2 kernel headers have no persistent form of a fanotify mark. FUSE is the one kernel mechanism where "no server" means "the open fails". What the first draft wanted to avoid was writing a filesystem; the door does not, it forwards to one.

Checked on 2026-11-13: both kernels (desktop 7.3.3, Deck 7.2.10-valve1) have `CONFIG_FUSE_PASSTHROUGH=y`; the FUSE protocol in the headers is 7.45 with `FOPEN_PASSTHROUGH`. Home is btrfs on both; the Deck's SD card is ext4. The Drive holds 368,436 files, 2.53 TiB, Google Docs not counted.

FUSE ("filesystem in userspace") is part of the Linux kernel: a program opens `/dev/fuse`, mounts a folder, and from then on the kernel sends it every operation on that folder as a message and waits for the reply.

### FUSE in the btrc standard library

The protocol goes into btrc as a library module, `Library.FileSystem.Userspace`, built before the client and usable by anything else:

- It speaks the kernel protocol directly (`/dev/fuse`, the structures in `linux/fuse.h`); no libfuse.
- A filesystem is a class that overrides typed operations (`lookup`, `read`, `rename`, ...) taking and returning BTRC values, with failures as typed errors that map to errno, never raw structs or integer codes in product code.
- Mount lifetime is an owned resource: dropping it unmounts. Requests are served on a worker pool; an operation can answer later without blocking a worker (needed for held reads).
- Passthrough is a first-class answer to `open`: "the kernel may use this file directly".
- Kernel cache control (invalidate an entry, an inode, a range) is part of the API.
- It ships with two reference filesystems that double as its tests: one in memory, and one that forwards to a real directory. The door is the second with the client's rules added.
- Tests live in btrc and run in a VM against the real kernel: the standard operations, passthrough, killing the server (every open must fail), and a file-operation stress run compared against the same operations on the bare directory.

### Two processes

| Process | Does | Size and risk |
|---|---|---|
| **door** | FUSE server: forwards operations to the store, hands local files to the kernel, asks the engine for missing ranges, records intent | small, no network, no parsing of remote data; holds the one capability passthrough needs |
| **engine** | Drive API, transfers, sync decisions, policy, control socket | large, unprivileged; may crash and restart without `~/Drive` going away |

If the engine is down, local files work and placeholders fail to read with a clear error. If the door is down, nothing new opens under `~/Drive` until it restarts (about a second); files already open through passthrough keep working, so a running game keeps its ROM.

## No database

| What must be known | Where it lives |
|---|---|
| A file's Drive id, its parent's id, the version last agreed with Drive, its state | extended attributes on the file itself; they move with it on rename |
| Which ranges of a file are downloaded | the file's own extent map (data or hole), in whole 4 KiB blocks |
| Find a file by Drive id | `ids/<id>` in the store: a hard link to the same inode. Folders are found by walking the tree once when the engine starts (35,000 folders: under a second) |
| What is waiting to upload | `dirty/journal`: the door appends one line per change (made, changed, moved, removed) before the change happens; the engine reads it and keeps what is still to do in `dirty/work` |
| What was deleted locally | `trash/`: the door moves a deleted file here instead of unlinking it; the engine trashes it on Drive; it stays 30 days as an undo |
| Position in Drive's change feed | one small file, replaced atomically |

Why not SQLite: a database beside the tree is a second truth, and keeping the two consistent across a crash is the hard part of every sync client. Here each fact has one home. A full walk (`drive verify`) checks the invariants and repairs them.

Each volume that holds part of the tree has its own `ids/`, `dirty/` and `trash/`, because hard links cannot cross filesystems.

## Parts

```
door        FUSE protocol server; passthrough; range requests; intent recording
store       the real tree, ids/, dirty/, trash/; attribute and extent helpers
remote      Drive API v3: list, change feed, ranged download, resumable upload,
            rename/move, trash, token refresh
changes     when to read the change feed: a pushed message, or a timer as fallback
planner     pure function: (agreed version, local, remote) -> actions; all sync rules
engine      runs the planner's actions; transfer queue; retries; brakes
readers     who is asking for content: identity, budgets, consent
policy      pinned folders, eviction, placement on other volumes
control     local socket for the CLI and tray: status, pin, unpin, allow, pause, log
```

File states: `cloud` (nothing or part downloaded), `local` (complete, may be evicted when clean), `pinned` (complete, never evicted), `dirty` (changed locally, waiting to upload).

## Rules

- **Reading a placeholder:** the door asks for the range being read, rounded to chunks, with read-ahead when the reads are sequential. When the last hole is filled the file is checked against Drive's checksum and becomes `local`; later opens are passthrough. Offline, the read fails at once.
- **Local change:** the door marks the file dirty when a write begins. The engine uploads after the file is closed and quiet for a few seconds, and checks it is unchanged before and after.
- **Names that stay here:** unfinished and scratch files (`*.part`, `.~lock.*`, an emulator's `*.semu-partial`; the list is in the NixOS module) are never sent. A file is sent once it is renamed to a real name.
- **Remote change:** a new, empty file with the new facts takes the name; the old one is left untouched for whoever still has it open. Emptying a file in place would hand a program that has it open zeros. A program still reading the old, partly fetched file gets "stale file" for the parts it had not read, never a hole.
- **Both changed:** both are kept; the local one is renamed with the machine name and date.
- **Delete:** local delete goes to `trash/`, then to Drive's trash, never a permanent delete. A remote delete moves the local file into `trash/`.
- **Rename and move:** one operation at the door, sent to Drive as a rename.
- **Brakes:** a batch that would delete more than 50 entries or a tenth of the tree, in either direction, waits for `drive resume`. A folder whose volume is away holds everything the Drive says until the volume is back (below).
- **Several entries of one name:** Drive allows them in one folder, a folder here does not. The names are a function of what the folder holds, so every machine arrives at the same ones: the entry with the smallest Drive id gets the name, the others get `name (2).ext`, `name (3).ext` in order.
- **Pinning:** `drive pin PATH` downloads everything under it and keeps new remote files downloaded. `drive unpin PATH` makes its files evictable.
- **Eviction:** clean, unpinned files get their blocks released, least recently opened first, when the cache budget or free-space floor is hit. A file is only emptied under a write lease, which the kernel grants only while nothing has the file open or mapped, so a running program never sees its file turn to zeros.
- **Adoption:** `drive adopt LOCAL-FOLDER DRIVE-FOLDER` makes local files the contents of the same not-downloaded files: same size and the checksum the Drive reports. On one filesystem the file is moved in (the same inode); across subvolumes its blocks are shared; only otherwise is it copied. Files that differ are named and left where they are. With `--upload-new`, files the Drive does not have are moved in and uploaded.
- **Placement:** every machine declares the same pinned folders by their Drive path; a machine may add where one of them is stored (the Deck: the SD card). It still appears in place under `~/Drive`. If the volume is absent the folder is "unavailable", not "deleted". The volume must be ext4 or btrfs (extended attributes, hard links, sparse files). A move across the boundary keeps the file's Drive id, so it stays a rename on Drive.
- **How placement works:** the door holds every folder it has seen as an open handle to a real folder and performs each operation relative to a handle, never by building paths. A placement is one substitution: looking up that folder's name returns a handle to the other location (`UserspaceForwarding.redirect` in btrc). Everything beneath follows from the handles. Each location carries its own `ids/`, `trash/` and `tmp/`; the journal stays in the main store. No mounts inside the store and no graph of our own; the kernel's directory tree is the graph.
- **As built:** in the main tree the folder is an empty stand-in. The location counts as present only if its tree carries the stand-in's Drive id, so an unmounted card, another card or a bare mount point all read as absent. Absent means: listed by name, entering it fails with "no such device or address", the door lets go of everything it held there so the volume can be removed, nothing is downloaded for it, and the engine stops reading the change feed until it is back, so nothing the Drive says about it is skipped. `drive place FOLDER` moves a folder onto its volume once; the folder on the volume must already exist, so nothing is ever written to a bare mount point. A placed folder and the folders it is under cannot be renamed, moved or removed on this machine ("device or resource busy").
- **Not built yet:** a move across the boundary is a copy and a delete (`mv` does it), so the Drive sees a new file and a trashed one rather than a rename. If another machine renames or trashes a placed folder or a folder above it, this machine holds until its declaration is changed.
- **Encryption:** the client does not encrypt. Files are as protected as the volume they sit on: the system disk is encrypted, the Deck's SD card today is not.
- **Google Docs, Sheets, Slides:** small link files that open in the browser.

## Readers: who may cause downloads

Listing a folder, however large, never downloads anything: names, sizes and dates are already on disk. Only reading a file's contents does. A script that walks the tree is harmless; a script that opens and reads every file would pull 2.5 TiB.

The other systems do not fully prevent this either. On macOS a program that reads every file in a streamed Drive folder downloads every file; only system services such as Spotlight and Time Machine are stopped from doing so. Windows shows "this app is downloading files" with a button to block it. This client is stricter than both: nothing reads past a small budget without the user's say.

- **Identity.** Each read request carries the process id. The engine resolves it to what the user would recognise: the application (executable and its systemd unit or app scope), and for an interpreter the script it is running and what launched it, so allowing one Python script does not allow every Python script. A process it cannot see (inside a container with its own process ids) is "unknown".
- **Three classes.** `allow` (apps the user approved), `deny` (they see names and sizes, content reads fail with a distinct error), `ask` (everything else, including unknown).
- **The deny list ships with the client** (`readers.deny`): indexers and desktop search, thumbnail and preview helpers, backup and other sync tools, virus scanners. The NixOS module adds to it; `drive readers block NAME` adds one by hand.
- **Budget for `ask`.** Each application may read a small amount of not-downloaded data per window (bytes and distinct files): enough to open the handful of files a person opens, far too little for a crawl. Range fetching keeps type sniffing and header reads cheap.
- **The dialog, as built.** `drive-ask` is a BTRC window (`Library.GUI`) started in the user's desktop session; Enter allows once, A always allows, D blocks, Escape leaves the question unanswered. The engine learns at once when there is no session to show it in, and stops the program there.
- **The dialog.** Past the budget, that application's reads are held and the client's own window asks: "**backup.py** (started from Konsole) wants to download 1,240 files, 38 GB, from Google Drive", with Allow once, Always allow and Block. It is a BTRC window owned by the client, not a desktop notification that can be missed. Held reads continue on Allow and fail on Block or after a minute with no answer.
- **Where no dialog can show** (Game Mode on the Deck, an SSH session, no display): the reads fail at the budget and `drive readers` shows what was stopped and lets it be allowed from the command line.
- **Memory.** Answers are remembered per application; `drive readers` lists and changes them; defaults are declared in the NixOS module.
- **Hint for cooperative tools.** Placeholders report zero allocated blocks, the conventional sign of an offline file.
- **Later:** previews for not-downloaded files from Drive's own thumbnails, so a file manager shows them without reading content.
- Pinned folders are never affected: everything in them is local.

## Remote changes: push, with the change feed as the record

Two different things are involved, and both are needed:

- **The change feed** answers "what changed since position N". It is the record: after sleep, a reboot or time offline, the client catches up from its saved position. This exists whatever triggers it.
- **The trigger** decides when to read the feed. A timer is polling. A message from Google is push.

Drive's classic push is a webhook: Google posts to a public HTTPS address with a valid certificate, which a desktop or a Deck behind a home router does not have. Google also delivers Drive events into a Cloud Pub/Sub topic (Workspace Events API), and a machine can wait on a topic with an outbound connection, so nothing has to be hosted.

- **Version 1: push through Pub/Sub.** The client subscribes to Drive events for the account, holds a long-lived pull open on the topic, and reads the change feed when a message arrives. The message only says "look now"; its content is never trusted over the feed.
- **Fallback: a timer.** If a pull cannot be held (no network path, quota, the events not covering something), the client reads the feed every 5 seconds while `~/Drive` is in use and every 60 seconds when idle. The same code path runs on wake and on reconnect.
- **To confirm in M1, before building on it:** that the events cover the whole of My Drive for this account type (the documentation found describes folders and shared drives, and labels Drive support a preview in one place and available in another), what the extra OAuth scope and Cloud project setup are, and that one message arrives within a couple of seconds of a change. If it does not hold up, the alternative is the webhook with a tiny relay we host.

Local changes need neither: the door sees them as they happen.

What Google's page on Drive events says (read 2026-11-13, not yet tried):

- A subscription watches one file, one folder, or one shared drive. A folder subscription can include everything beneath it. The page names no way to watch a whole My Drive, and says the root of a shared drive cannot be watched; whether My Drive's root folder can is not stated.
- It covers files created, moved, renamed, changed, trashed, restored and deleted, for personal Google accounts as well as Workspace ones.
- So the design to try is one subscription on My Drive's root folder with descendants included, or failing that one per top-level folder, with the timer covering anything the subscriptions miss.

Not confirmed. Checked on 2026-11-13 with the existing sign-in: it carries only the Drive permission, and the Workspace Events API is off in the Cloud project (number 963392265364) that owns the OAuth client.

The setup is scripted, with one step only a person can do:

1. `drive auth` prints a Google address; approving it in a browser gives the client a sign-in with the Drive and Pub/Sub permissions (and, for the setup, the permission to turn APIs on in the project).
2. `drive push prepare` turns on Pub/Sub and Workspace Events, makes the topic `drive-events`, lets Drive's service account publish to it, and makes this machine's subscription.
3. `drive push watch` asks Drive to publish a folder's events (My Drive's root unless told otherwise); `drive push pull` waits for them.

**Result, 2026-11-13, against the real account:** push works, for folders.

- After the sign-in was approved, `drive push prepare` turned on both services, made the topic and this machine's subscription, with nothing done by hand.
- My Drive's root is refused as something to watch ("the provided target resource is invalid"). An ordinary folder is accepted, with everything beneath it included.
- In a scratch folder (`drive-client-scratch`), six changes produced six events: file created, content changed, renamed, a folder created inside, a file created inside that folder, and trashed. Nothing was missed.
- An event reached this machine 4 to 10 seconds after the change (three measurements: 4.0, 10.1 and 10.2 s).
- A subscription lasts seven days and then has to be renewed.

**Checked end to end, 2026-11-13, real account:** `drive push listen` beside the real engine, whose own timer was set to 60 s. Three files made on the Drive (by a script standing in for another machine) were in the local tree 3.3, 7.7 and 7.8 s later. The first attempt failed because the listener looked for a subscription named after a machine called "machine"; the name now comes from the system's hostname, and the NixOS unit passes it.

So the trigger is: one subscription per folder directly in My Drive, each with its descendants, renewed before it expires; a listener waits on the topic and tells the engine to read the change feed. Files that sit directly in My Drive, and anything a subscription misses, are covered by a slow timer (once a minute), which is also what runs when the listener cannot reach Pub/Sub.

## Where things live

- Product: `config/pkgs/utils/drive/` in this repository, like `nixosctl` and `deckpad`. FUSE lives in the btrc standard library (above). Extended attributes and extent queries go into btrc's `Library.FileSystem` as well; nothing kernel-facing stays product-local.
- Transfers use `curl`, as the standard library's HTTP client does, driven directly for ranged downloads and resumable uploads, batched so connections are reused. A libcurl binding only if measured throughput calls for it.
- Store: a persisted folder outside the home directory (`/var/lib/drive/<user>/`), not reachable except through the door.
- Credentials: `~/.config/drive/`, mode 0600, persisted, never in the repository. First version imports the OAuth client and refresh token from the existing `rclone.conf`; `drive auth login` comes later.
- NixOS: `settings.apps.utils.drive` (enable, pinned folders, placements, cache budget, reader defaults). It replaces the rclone mount and the `offline` option at cutover.

## Tests

- **Fake Drive:** a BTRC HTTP server implementing the endpoints used, with fault injection (rate limits, server errors, dropped transfers, duplicate names).
- **Planner contracts:** table-driven cases for every rule; no root, no network.
- **VM end-to-end** (graph nodes, required): offline boot shows the tree; a read fetches only its range; offline read fails cleanly; **door killed: a placeholder cannot be opened and never yields zeros**; engine killed: local files work, placeholders fail; a game-like workload on a pinned folder across an engine crash; crash during a range fetch and during an upload; conflict; delete to trash both ways; delete brake; reader budget, consent and deny; eviction under disk pressure; placement volume absent and present; adoption without transfers; reboot under the erase-on-boot root; `drive verify` repairs a damaged `ids/`.
- **Two-machine convergence:** two clients against one fake Drive, random operations, both trees must end identical with no file lost.
- **Mutation checks:** removing a brake, the conflict rule or the reader budget must make a test fail.
- **Real Drive:** a scratch folder in the real account, run by hand before each cutover step.

## Milestones

Each ends with its tests passing and a short note of what was proved.

- **M0 FUSE in btrc.** `Library.FileSystem.Userspace` with its two reference filesystems and VM tests, merged in the btrc repository. On both kernels and both filesystems, with the forwarding filesystem: passthrough for a local file, ranges of a placeholder, kill the server and confirm opens fail. Measure metadata speed on a 370,000-file tree (the ES-DE scan), passthrough speed against the bare disk, and how long the tree takes to build. Ends with the numbers and a go or no-go, written into this file.
- **M1 Remote.** API client, token refresh, full listing, change feed, ranged download; fake Drive; the Pub/Sub push path confirmed against the real account or replaced.
- **M2 Read-only client.** Build the store with placeholders, keep it current from the change feed, range fetching, offline boot, reader policy with its dialog. Useful by itself: browse and open anything in Drive.
- **M3 Writes.** Dirty tracking, uploads, renames, trash, conflicts, brakes. Real-Drive writes only in the scratch folder.
- **M4 Pinning.** Pin, unpin, eviction, placement, and adoption of an existing local copy by matching size, time and checksum so nothing is transferred.
- **M5 Cutover.** NixOS module; adopt `~/Games/Emulation` on the desktop and the SD-card library on the Deck; point Semu at `~/Drive/media/Games/Emulation` and drop its root override; remove the rclone mount, the bisync timer and the launcher hooks. The old sync keeps running until this step, and the two never write the same folder at the same time.
- **M6 Everyday use.** Full CLI, tray status, pin and unpin from Dolphin's context menu, `drive auth login`.
- **M7 Hardening.** Convergence soak, thumbnails for placeholders.

## Encrypting the Deck's SD card (before the Deck's cutover)

Separate from the client, but it decides where the Deck's placement points, so it goes first. Not started; the card could not be inspected when this was written (the Deck was off).

What exists: `settings.disk.encryption.opportunisticUnlock` is on for the Deck. After boot it tries the typed boot password on other attached LUKS volumes and opens the ones that match. It does not mount anything, does not run when a card is inserted later, and does not run on a boot that needed no typed password.

Shape:

- **Declared, not hand-made.** The Deck's host file already names the card (`sdCard.label`). It gains `encrypted` and `mountPoint = "/sd"`; from that the system generates the unlock entry, the mount, and the paths Semu and the sync use.
- **Two ways to open.** The boot password, so the existing unlock opens it at boot and any of the machines can open it by hand; and a key file on the Deck's encrypted system disk, which needs no typing.
- **One command prepares the card** (`nixosctl`, never by hand): it checks, formats, adds both ways to open, saves the header, and refuses to start unless the checks below pass. Formatting waits for an explicit yes at that moment.

Every concern, what handles it, and what proves it:

| Concern | Handled by | Proved by |
|---|---|---|
| The existing unlock opens but does not mount | a declared mount of the opened card at `/sd` | VM: boot with the card, `/sd` is there |
| A card inserted after boot is not opened | the key file: the unlock entry is tied to the device appearing, not to boot | VM: attach the card after boot, `/sd` appears |
| A boot with no typed password opens nothing | the key file does not depend on the password | VM: boot on a host that unlocks without typing |
| A missing card must not delay or fail boot | the entry and the mount tolerate absence with a short wait | VM: boot with no card, time to login unchanged |
| The card is pulled while running, then put back | the mount goes away; Semu and the sync see "unavailable" (the marker guard today, placement later); reinsert reopens | VM: detach and reattach under a running sync |
| Suspend and resume | the card stays open in memory, like the system disk | VM: suspend, resume, read a file |
| The key file must survive the erase-on-boot root | it is stored in a persisted path, root only | the persistence-ownership check, and a VM reboot |
| The Deck is reinstalled and the key file is lost | the password still opens the card; the command adds a new key file | VM: remove the key file, reopen with the password, re-add |
| Formatting wipes the card | the command lists everything on the card first and stops if anything but the Emulation library is there; it also requires the desktop's copy to match the card (count, sizes, sampled checksums) | run on the Deck before formatting; refusal covered in the VM |
| A Steam library on the card | found by the listing above; its path is re-added after the refill | checked on the Deck |
| The sync runs against a half-filled card | the sync timer is stopped for the operation; the marker guard blocks it on an empty card anyway | VM: sync refuses while the marker is absent |
| Refill re-reads everything if timestamps differ | refill from the desktop keeping timestamps; verify count and sizes | first sync afterwards reports nothing to transfer |
| Game Mode may offer to format a card it no longer recognises | checked immediately after formatting, before the refill, while nothing on the card is unique; if it prompts, suppress it through Jovian or Steam settings, and if that is not possible, stop and report | on the Deck |
| A damaged header loses the card | header copy kept on the Deck's system disk and on the desktop; password in 1Password | restore the header in the VM |
| Read speed | measured with the same read test before and after; numbers recorded here | on the Deck |
| Paths that name the old location | Semu's root, the sync's folder and later the client's placement all come from `sdCard.mountPoint` | package-groups check |

Not decided by the plan: whether a measured slowdown is acceptable. The numbers are reported and you choose.

**Built so far (2026-11-13), nothing done to the real card:** `settings.disk.sdCard` (`enable`, `label`, `mountPoint`, `keyFile`) and `config/system/nixos/sd-card.nix`. The Deck's host file does not turn it on yet; that is the first step of the Deck's cutover.

- The card is opened with the key file by a unit that is asked for when the card's device appears, at boot or later, never by boot itself. The mount follows the opened container and is bound to the card's own device, with a lazy unmount, so a pulled card comes off `/sd` at once even while a program holds a file on it; the container closes when its last user lets go.
- The container is given its label last when a card is prepared: a labelled card is opened the moment it is seen.
- The Drive client is told the card's device (`placementDevices`), so it lets go as the card leaves rather than when reads start failing, and it flushes the card before it records how far it has read the change feed, so a pulled card is never behind what the client believes.
- VM scenario `sd-card` (required graph node): a blank card in the slot neither delays nor fails the boot; a prepared card opens and mounts by itself and is there after a restart with nothing typed; pulled while a program holds a file on it, it comes off and its container closes, and put back it returns; a Drive folder placed on it is unavailable while it is out and carries on when it is back; with the key file lost the password opens it and a new key file can be added; a wiped header is restored from its copy.
- `nixosctl sd-card prepare DEVICE` formats a card for the machine. It refuses a device that is in use anywhere but a desktop's automount, a card that holds anything but the library folder, and a card with a file the Drive does not have (it reads every file and compares it with the Drive's checksum, changing nothing). Then it asks "Format DEVICE?" and stops unless the answer is yes. It makes the key file, encrypts, makes the filesystem owned by the user, saves a copy of the header beside the key file, and names the card last. `nixosctl sd-card password DEVICE` adds the password as the second way in, `open` opens a card by hand with it, `key` gives a machine that lost its key file a new one, `status` says where things stand. The scenario runs all of these, the refusals included.
- Not covered: suspend and resume with the card in a VM. The test VM refuses to sleep (writing "mem" or "freeze" to `/sys/power/state` fails there), so this row of the table is left for the Deck itself, where the card's container stays open in memory like the system disk's. Everything else in the table that says "on the Deck" is also still to do.

## M0 results

In progress. Measured on FRACTAL-NORTH (kernel 7.3.3, btrfs on NVMe) on 2026-11-13 with the forwarding filesystem from `Library.FileSystem.Userspace`, run as root, passthrough on:

| Question | Result |
|---|---|
| A killed server fails closed | yes: after `kill -9`, a new open and a listing return "transport endpoint is not connected", and a read on a file opened earlier without passthrough returns an I/O error |
| Passthrough speed | reading 1 GiB cold: 5.3 to 5.5 GB/s through the mount, 5.4 to 5.5 GB/s on the bare disk |
| Building the tree | 370,000 empty files on the bare disk: 4.2 s (placeholders also need a size and attributes each; measured in M2) |
| Scanning 370,000 files, names only | cold 0.9 s bare, 4.2 s through the mount; warm 0.1 s bare, 1.5 s through the mount |
| Scanning 370,000 files with `stat` | cold 2.7 s bare, 4.5 s through the mount; warm 0.4 s bare, 1.8 s through the mount |
| Server cost for that tree | 64 MB of memory and one descriptor per file the kernel remembers (373,743 here; the limit was 524,288) |
| File operations through the mount | create, append, rename, links, permissions, times, truncate, extended attributes, mmap writes and removes all behave as on the bare directory |

Findings that change the design:

- The server holds one descriptor for every file the kernel remembers. The door has to run with a descriptor limit above the size of the tree, or hold nodes as file handles instead. Decide in M2 with the real tree.
- BTRC work does not scale across threads (one lock guards all reference counts), so the door serves requests on one thread and answers slow ones later. The scan numbers above are from that one thread.

Measured since, on the same machine:

| Question | Result |
|---|---|
| Building the real tree (M2) | the whole of My Drive, 398,957 entries (363,989 files in 34,969 folders; 10,484 entries outside the tree were skipped): 1,110 s, nearly all of it Google's listing, 355 MB of memory at the peak. The placeholders take 436 KB beyond their folder entries |

**Decision: go**, taken 2026-11-13 on the numbers above. A dead server fails closed, files handed to the kernel read at the disk's own speed, and a scan of 370,000 files through the mount costs seconds, not minutes. Everything since was built on that, and each property is now also a scenario that runs on every change (`door`, `module`).

ext4 has been covered in VMs only: the `placement` and `sd-card` scenarios keep their placed folder on ext4, and the forwarding filesystem's own check in btrc compares it with ext4 over thousands of random operations. **Not measured: the Deck itself** (kernel 7.2.10-valve1, ext4 on its real SD card: scan and read speed). That is the first thing to do there before its cutover; if the scan through the mount is slow on the card, the remedy is longer cache times for names in the door, which only the door changes.

## What each milestone proved

Every line is a VM scenario under `tests/drive/` unless it says otherwise. Each scenario runs the real door and engine against the fake Drive.

- **M1** `remote`: the API client against the fake (listing, change feed, ranges, resumable upload, token refresh, rate limits and server errors retried). Push confirmed on the real account (above).
- **M2** `store`, `door`, `readers`: placeholders take no room; a read fetches only its range; a completed file is checked against the Drive's checksum; with the engine killed downloaded files work and others fail to open; **with the door killed nothing opens and no read returns zeros**; after a restart with no network the tree is there. A program past its allowance is stopped, asked about once, and remembered; a denied program is refused; removing the allowance makes the test fail.
- **M3** `planner` (table of every rule, no VM), `writes`, `convergence`: new files and folders, changes, renames and moves reach the Drive as what they are; a write into a not-downloaded file fetches it first and an outright replacement does not; deletes become trash on both sides; a file changed on both sides is kept twice (with the rule off, one version is lost: the test fails); 5 deletions against a limit of 3 wait for `drive resume` in both directions (with the brake off they go through: the test fails); changes made while the engine was stopped go up afterwards. Two clients doing 360 random operations over three seeds end with identical trees, identical to the Drive's, with every file each wrote in its own folder present and unchanged.
- **M4** `pinning`, `placement`: a pinned folder is downloaded without being read, including what the Drive adds or changes later, and keeps the Drive's file times; content outside pinned folders is given back oldest first and **never while a program has the file open**; adoption moves the local file in (same inode) and downloads nothing. A placed folder is unavailable before its volume has ever been there, in place once it is, unavailable again when the volume is pulled while running (and the volume can then really be removed), and back with everything the Drive changed meanwhile.
- **M2, the window** `dialog`: in a VM with a real compositor the window appears with the program's name (read off the screen), each of its three answers does what it says, and with no desktop session the program is stopped at once and listed.
- **Crashes** `crashes`: the engine is killed while a read waits for its range (the read fails; what had arrived is the file's own bytes; after a restart the file reads whole), after the Drive took an upload and before the engine could note it (the file is on the Drive once, not twice), and under a program that keeps reading a pinned file and rewriting a save (every read and save works, and the last save reaches the Drive as one file).
- **Repair** in `pinning`: with an id name removed and a stray one added, `drive verify` reports one repaired and one removed, and the engine then finds the file by its id again.
- **The real host's configuration** `host-boot-FRACTAL-NORTH-VM-secure`: installed and booted in a VM with its erase-on-boot root, through two hibernations, the Drive is mounted for the user and the system state is "running".
- **M5 so far** `module`: the units the NixOS module generates mount the Drive for the user from boot with one capability, recover from either service being killed, and show the tree after a restart with no network.
- **Real account, 2026-11-13, by hand, in `drive-client-scratch` only:** through a test mount beside the rclone one, a new folder, nested folder and files arrived on the Drive with the right checksums and times; an append went up as the same file's next version; a rename and move kept the file and its time; a delete was a trash; a change made on the Drive replaced the local file and was fetched when read; a file changed on both sides was kept twice; a trash on the Drive moved the local file to the local trash. The whole of My Drive was mirrored twice (398,957 entries each time: 1,110 s and 866 s).

## The desktop's cutover (M5, FRACTAL-NORTH, 2026-11-13)

Done, in this order:

1. The whole of My Drive was mirrored into the real store, `/var/lib/drive/<user>`.
2. A block-sharing copy of the library (`cp --reflink`, 1.2 s, no space) was adopted into it: 42,270 files adopted, 13 already there, none different, none left behind, 9 min 41 s, nothing transferred. The original `~/Games/Emulation` was never touched and nothing uses it any more.
3. Before the switch, the build was compared with the running system: the only differences were the client's units in, rclone's out, and the reset-on-boot program rebuilt by the newer btrc. Because that program runs at every boot, the reset scenario and an install-and-boot of this host's configuration in a VM were run first; both pass.
4. `nixosctl update`. `~/Drive` is the client's mount, the library is pinned and all of it is here, Semu resolves its root to `~/Drive/media/Games/Emulation` with no override and re-points its links as each emulator is next started, a file written and deleted through the mount was on the Drive and then in its trash.

One thing went wrong, and it was mine: a test engine from before the switch was still running on the same store, so for a few minutes two engines each uploaded every new file and the Drive got two of each (five test files in the scratch folder, since trashed). The engine and the door now take a lock on their store and a second one refuses to start; the scenario checks it. Looking for the cause also turned up two real weaknesses, both fixed: the engine and the push listener kept their requests in one folder under the same numbered names, and a new file whose upload got no answer was simply sent again.

The Deck has had nothing done to it. Until its card is encrypted it keeps the timer sync, which is why the rclone module is still in the repository.

## The mutation checks

Each safety rule has a switch that takes it away, and a step in its scenario that shows the protected thing then goes wrong. A test that passed either way would prove nothing.

| Rule | Taken away with | What the scenario then sees |
|---|---|---|
| A not-downloaded file is never handed to the kernel as it is | door `--unguarded` | the read that failed returns zeros (`door`) |
| A program reads only so much without a yes | `--budget-files 0` | the same crawl downloads everything (`readers`) |
| A file changed on both sides is kept twice | `--overwrite-conflicts` | one of the two versions is gone, here and on the Drive (`writes`) |
| Many deletions wait for a yes, in both directions | `--delete-limit 0` | the same batch goes straight through (`writes`) |

## Known limits

- The engine does one thing at a time: a large upload or a 32 MiB piece of a pinned download delays another program's read of a not-downloaded file until it finishes.
- `trash/` is never emptied yet (the 30 days are not enforced).
- Eviction looks only at the main store, not at placed volumes.
- Google's change feed hands out about 50 changes a page for this account, one request each, so a store that is far behind catches up slowly: 411,000 changes (a day in which another sync tool rewrote every file's time) would have taken over an hour. The engine stays usable meanwhile, since it reads one page between looks at the door, but a fresh listing is quicker past roughly 100,000 changes; the client does not switch to one by itself yet.
- Root cannot look inside `~/Drive` (as with the rclone mount today).

## Not in version 1

Shared drives and "shared with me"; more than one account; editing Google Docs; local symlinks and special files (reported, not synced); Windows; replacing Google's client on the Mac.

## Risks to watch

- Metadata speed through the door on large folders (M0 measures; the kernel caches entries, and the door is the only writer so the cache can be long-lived).
- A door outage blocks new opens even in pinned folders; the door must stay small enough to be boring.
- Drive allows names a Linux folder cannot hold (duplicates in one folder, `/` in a name); these need a stable, reversible mapping.
- Save files that an emulator rewrites continuously: the quiet period must be long enough not to upload half-written cards.
- First adoption of 900 GB must be metadata-only; M4 proves it on a copy first.
- Extended attributes cost about one extra block per file on ext4 (roughly 350 MB for the SD-card library).
- API rate limits during the first full listing.
