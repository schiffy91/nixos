{ lib, config, ... }: with lib.types; {
  options.settings.apps.utils.drive = {
    enable = lib.mkOption { type = bool; default = false; description = "Serve Google Drive at the mount point with the BTRC client: every file in place, downloaded when opened, pinned folders kept whole."; };
    mountPoint = lib.mkOption { type = strMatching "/.+"; default = "${config.settings.users.admin.homeDirectory}/Drive"; description = "Where the Drive appears."; };
    store = lib.mkOption { type = strMatching "/.+"; default = "/var/lib/drive/${config.settings.users.admin.username}"; description = "The real tree behind the mount. Persisted; reached only through the mount."; };
    pinned = lib.mkOption { type = listOf str; default = []; description = "Folders kept downloaded, by their path in the Drive. Declare the same ones on every machine."; };
    placements = lib.mkOption { type = attrsOf (strMatching "/.+"); default = {}; description = "Folders stored on another volume, by their path in the Drive, each with the folder on that volume that holds it. They still appear in place; while the volume is away they are unavailable."; };
    placementDevices = lib.mkOption { type = attrsOf (strMatching "/.+"); default = {}; description = "For a placement's folder on its volume, a device path that exists exactly while the volume is attached (a /dev/disk/by-label link). Without it the client notices a pulled card only when reads start to fail."; };
    cacheBytes = lib.mkOption { type = ints.unsigned; default = 0; description = "How much content outside pinned folders may stay downloaded; the least recently opened files are given back beyond it. 0 keeps everything."; };
    freeFloorBytes = lib.mkOption { type = ints.unsigned; default = 10 * 1024 * 1024 * 1024; description = "Give content outside pinned folders back when the store's disk has less than this free."; };
    readers.deny = lib.mkOption { type = listOf str; default = []; description = "Programs that may list the Drive but never cause a download, beside the list the client ships."; };
    readers.budgetFiles = lib.mkOption { type = ints.unsigned; default = 20; description = "Not-downloaded files a program may open per window before someone is asked. 0 never asks."; };
    ignore = lib.mkOption { type = listOf str; default = []; description = "Names of files and folders that stay on this machine and are never sent, beside the client's own list of what programs call their unfinished and scratch files. `*` may start or end a name. A file is sent once it is renamed to a name that is not listed."; };
    quietSeconds = lib.mkOption { type = ints.positive; default = 5; description = "How long a changed file must be left alone before it is uploaded."; };
    followSeconds = lib.mkOption { type = ints.positive; default = 60; description = "How often the Drive's change feed is read when nothing announces a change."; };
    push = lib.mkOption { type = bool; default = false; description = "Wait on the account's Pub/Sub topic and read the change feed as soon as the Drive announces a change. Needs `drive auth` and `drive push prepare` once."; };
    api = lib.mkOption { type = nullOr str; default = null; description = "Another address for the Drive API. Tests point this at the fake."; };
  };
}
