import sys


def pixels(path):
    with open(path, "rb") as handle:
        data = handle.read()
    fields = []
    index = 0
    while len(fields) < 4:
        while index < len(data) and data[index : index + 1].isspace():
            index += 1
        if data[index : index + 1] == b"#":
            while index < len(data) and data[index] != 0x0A:
                index += 1
            continue
        start = index
        while index < len(data) and not data[index : index + 1].isspace():
            index += 1
        fields.append(data[start:index])
    index += 1
    width, height = int(fields[1]), int(fields[2])
    return width, height, data[index : index + width * height * 3]


def main(before_path, after_path, agent):
    before_width, before_height, before = pixels(before_path)
    after_width, after_height, after = pixels(after_path)
    if (before_width, before_height) != (after_width, after_height):
        print(f"FAIL: the panel changed resolution: {before_width}x{before_height} -> {after_width}x{after_height}")
        return 1
    changed = sum(
        1
        for offset in range(0, min(len(before), len(after)), 3)
        if abs(before[offset] - after[offset]) > 12
        or abs(before[offset + 1] - after[offset + 1]) > 12
        or abs(before[offset + 2] - after[offset + 2]) > 12
    )
    print(f"agent={agent} changed_pixels={changed}")
    if changed < 40:
        print(
            f"FAIL: {agent} did not draw the keystroke: only {changed} pixels changed. "
            "Whatever owns the panel is not reading the keyboard, so a passphrase cannot be typed on it."
        )
        return 1
    print(f"PASS: {agent} drew the keystroke, so it owns the panel and reads the keyboard")
    print("panel-responds")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2], sys.argv[3]))
