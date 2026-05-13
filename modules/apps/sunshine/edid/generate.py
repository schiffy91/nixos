#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import struct, sys

MFG_ID = ((ord('L') - 64) << 10) | ((ord('L') - 64) << 5) | (ord('V') - 64)  # LLV
PRODUCT_CODE = 0x0090
SERIAL_NUMBER = 0x00000000
WEEK = 1
YEAR_OFFSET = 35  # 1990 + 35 = 2025
H_ACTIVE = 1280
V_ACTIVE = 800
REFRESH_HZ = 90
H_BLANK = 80
V_BLANK = 25
H_SYNC_OFFSET = 8
H_SYNC_WIDTH = 32
V_SYNC_OFFSET = 3
V_SYNC_WIDTH = 4
H_TOTAL = H_ACTIVE + H_BLANK
V_TOTAL = V_ACTIVE + V_BLANK
PIXEL_CLOCK_HZ = H_TOTAL * V_TOTAL * REFRESH_HZ
PIXEL_CLOCK_10K = round(PIXEL_CLOCK_HZ / 10_000)  # CVT-RBv2 → ~10098
H_SIZE_MM = 200  # Steam Deck OLED dims (~16:10 panel)
V_SIZE_MM = 125
SYNC_FLAGS = 0x1A  # digital separate, H+ V-

def detailed_timing(pclk_10k, hact, hbl, vact, vbl, ho, hw, vo, vw, hmm, vmm, flags):
    b = bytearray(18)
    b[0], b[1] = pclk_10k & 0xFF, (pclk_10k >> 8) & 0xFF
    b[2], b[3] = hact & 0xFF, hbl & 0xFF
    b[4] = ((hact >> 4) & 0xF0) | ((hbl >> 8) & 0x0F)
    b[5], b[6] = vact & 0xFF, vbl & 0xFF
    b[7] = ((vact >> 4) & 0xF0) | ((vbl >> 8) & 0x0F)
    b[8], b[9] = ho & 0xFF, hw & 0xFF
    b[10] = ((vo & 0xF) << 4) | (vw & 0xF)
    b[11] = (((ho >> 8) & 0x3) << 6) | (((hw >> 8) & 0x3) << 4) | (((vo >> 4) & 0x3) << 2) | ((vw >> 4) & 0x3)
    b[12], b[13] = hmm & 0xFF, vmm & 0xFF
    b[14] = ((hmm >> 4) & 0xF0) | ((vmm >> 8) & 0x0F)
    b[17] = flags
    return bytes(b)

def descriptor(tag, payload):
    b = bytearray(18)
    b[3] = tag
    b[5:5 + len(payload)] = payload[:13]
    for i in range(5 + len(payload), 18): b[i] = 0x20
    return bytes(b)

def checksum(block):
    return (256 - (sum(block) % 256)) % 256

def base_edid():
    e = bytearray(128)
    e[0:8] = bytes([0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00])
    struct.pack_into('>H', e, 8, MFG_ID)
    struct.pack_into('<H', e, 10, PRODUCT_CODE)
    struct.pack_into('<I', e, 12, SERIAL_NUMBER)
    e[16], e[17] = WEEK, YEAR_OFFSET
    e[18], e[19] = 1, 4  # EDID 1.4
    e[20] = 0xB5         # digital, 10bpc, DisplayPort
    e[21], e[22] = 20, 12
    e[23] = 0x78         # gamma 2.2
    e[24] = 0x3A         # active-off + RGB+YCrCb + preferred-timing-valid
    e[25:35] = bytes([0xEE, 0x95, 0xA3, 0x54, 0x4C, 0x99, 0x26, 0x0F, 0x50, 0x54])
    e[35] = 0x20  # established: 640x480@60 (CTA-861 VIC 1 conformance)
    for i in range(38, 54): e[i] = 0x01
    e[54:72] = detailed_timing(PIXEL_CLOCK_10K, H_ACTIVE, H_BLANK, V_ACTIVE, V_BLANK,
                               H_SYNC_OFFSET, H_SYNC_WIDTH, V_SYNC_OFFSET, V_SYNC_WIDTH,
                               H_SIZE_MM, V_SIZE_MM, SYNC_FLAGS)
    e[72:90] = descriptor(0xFC, b'LLV Streaming\n')
    e[90:108] = descriptor(0xFD, bytes([50, 120, 30, 150, 30, 0x01]) + b'\x0A' + b'\x20' * 6)
    e[108:126] = descriptor(0xFF, b'LLV-STREAM-1\n')
    e[126] = 1
    e[127] = checksum(e[:127])
    return bytes(e)

def cta_extension():
    c = bytearray(128)
    c[0], c[1] = 0x02, 0x03  # CTA-861, rev 3
    c[3] = 0x80              # underscan IT formats by default
    hdr_block = bytes([
        0xE0 | 6,            # extended tag, len 6
        0x06,                # HDR Static Metadata
        0b00000110,          # ET: traditional HDR gamma + SMPTE ST 2084 (PQ)
        0x01,                # SM: static metadata type 1
        138,                 # max content luminance ~1000 nits
        96,                  # max frame-average ~400 nits
        12,                  # min luminance
    ])
    colorimetry_block = bytes([
        0xE0 | 3,            # extended tag, len 3
        0x05,                # Colorimetry Data Block
        0b11000001,          # sRGB + BT2020YCC + BT2020RGB
        0x00,
    ])
    vcdb = bytes([
        0xE0 | 2,            # extended tag, len 2
        0x00,                # Video Capability Data Block
        0b01001000,          # QY=1, QS=1: selectable RGB/YCbCr quantization
    ])
    db = hdr_block + colorimetry_block + vcdb
    c[4:4 + len(db)] = db
    c[2] = 4 + len(db)       # DTD start offset (no DTDs follow → equals length end)
    c[127] = checksum(c[:127])
    return bytes(c)

if __name__ == '__main__':
    out = base_edid() + cta_extension()
    assert len(out) == 256
    sys.stdout.buffer.write(out)
