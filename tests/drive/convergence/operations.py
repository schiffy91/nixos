"""Random file operations on two mounts of the same Drive, then what each side must end with.

Usage: operations.py SEED COUNT MOUNT_A MOUNT_B EXPECTED_OUT

Each side works alone in its own folder (only-a-SEED/, only-b-SEED/) and both work in shared-SEED/. The private folders have one
writer each, so their final content is known exactly and is written to EXPECTED_OUT as "path sha256" lines; shared/
is where the two sides meet and only has to end the same on both.
"""

import hashlib
import os
import random
import sys
import time


def act(source, root, folder, state):
    names = [f"file{index}.txt" for index in range(6)]
    kind = source.choice(["write", "write", "write", "append", "rename", "remove", "mkdir"])
    name = source.choice(names)
    path = os.path.join(root, folder, name)
    try:
        if kind == "write":
            content = f"{folder} {name} {source.randrange(1_000_000)}\n".encode() * source.randrange(1, 200)
            with open(path, "wb") as file:
                file.write(content)
            state[name] = content
        elif kind == "append" and name in state:
            with open(path, "ab") as file:
                file.write(b"more\n")
            state[name] += b"more\n"
        elif kind == "rename" and name in state:
            target = source.choice(names)
            os.rename(path, os.path.join(root, folder, target))
            state[target] = state.pop(name)
        elif kind == "remove" and name in state:
            os.unlink(path)
            del state[name]
        elif kind == "mkdir":
            os.makedirs(os.path.join(root, folder, f"sub{source.randrange(3)}"), exist_ok=True)
    except OSError:
        pass  # the other side got there first; shared/ makes no promise about which operation wins


def main():
    seed, count, mount_a, mount_b, expected_out = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3], sys.argv[4], sys.argv[5]
    source = random.Random(seed)
    own_a, own_b, both = f"only-a-{seed}", f"only-b-{seed}", f"shared-{seed}"
    for root, folder in ((mount_a, own_a), (mount_b, own_b), (mount_a, both)):
        os.makedirs(os.path.join(root, folder), exist_ok=True)
    time.sleep(8)  # both sides know the three folders before the work starts
    private = {own_a: {}, own_b: {}}
    shared = {mount_a: {}, mount_b: {}}
    for _ in range(count):
        root = source.choice([mount_a, mount_b])
        if source.random() < 0.5:
            act(source, root, both, shared[root])
        else:
            folder = own_a if root == mount_a else own_b
            act(source, root, folder, private[folder])
        time.sleep(source.choice([0.05, 0.2, 0.6]))
    with open(expected_out, "a") as out:
        for folder, state in sorted(private.items()):
            for name, content in sorted(state.items()):
                out.write(f"{folder}/{name} {hashlib.sha256(content).hexdigest()}\n")
    print("done", count)


if __name__ == "__main__":
    main()
