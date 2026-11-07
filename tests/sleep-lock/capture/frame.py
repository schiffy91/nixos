import json
from PIL import Image

path = "/tmp/sleep-capture.ppm"
with open(path, "rb") as source:
    assert source.readline() == b"P6\n"
    _, sequence, timestamp = source.readline().decode().split()
    source.seek(0)
    image = Image.open(source).convert("RGB")
colors = image.getcolors(image.width * image.height)
assert colors is not None
private = sum(count for count, color in colors if color in [(0x90, 0x20, 0x20), (0x20, 0x60, 0x90)])
print(json.dumps({"sequence": int(sequence), "timestamp": float(timestamp), "size": image.size, "private": private}))
