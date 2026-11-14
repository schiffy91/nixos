"""The fake Drive's files as "sha256 path" lines, sorted by path, from its dump on stdin."""

import json
import sys

entries = {entry["id"]: entry for entry in json.load(sys.stdin)}


def numbered(name, number):
    stem, dot, extension = name.rpartition(".")
    if not dot or not stem:
        stem, extension = name, ""
    else:
        extension = "." + extension
    return name if number <= 1 else f"{stem} ({number}){extension}"


def live(entry):
    while entry is not None:
        if entry["trashed"]:
            return False
        entry = entries.get((entry.get("parents") or [None])[0])
    return True


# Several entries of one name in one folder: the smallest id gets the name, the others the first free numbered
# name, in order of name and id. The same rule as the client's, written a second time.
local = {}
folders = {}
for entry in entries.values():
    if live(entry):
        folders.setdefault((entry.get("parents") or [None])[0], []).append(entry)
for members in folders.values():
    taken = set()
    others = []
    previous = None
    for entry in sorted(members, key=lambda entry: f'{entry["name"]}/{entry["id"]}'.encode()):
        if entry["name"] != previous and entry["name"] not in taken:
            local[entry["id"]] = entry["name"]
            taken.add(entry["name"])
        else:
            others.append(entry)
        previous = entry["name"]
    for entry in others:
        number = 2
        while numbered(entry["name"], number) in taken:
            number += 1
        local[entry["id"]] = numbered(entry["name"], number)
        taken.add(local[entry["id"]])


def path(entry):
    parent = (entry.get("parents") or [None])[0]
    if parent in (None, "root") or parent not in entries:
        return local[entry["id"]]
    return path(entries[parent]) + "/" + local[entry["id"]]


lines = [f'{entry["sha256Checksum"]} {path(entry)}' for entry in entries.values() if "sha256Checksum" in entry and live(entry)]
print("\n".join(sorted(lines, key=lambda line: line.split(" ", 1)[1])))
