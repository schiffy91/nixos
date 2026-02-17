#!/usr/bin/env nix-shell
#!nix-shell -i python3 -p python3 alsa-utils

import subprocess, sys, struct, signal, re, math

def get_capture_devices():
    result = subprocess.run(['arecord', '-l'], capture_output=True, text=True)
    devices = []
    for line in result.stdout.splitlines():
        m = re.match(r'card (\d+): \S+ \[(.+?)\], device (\d+):', line)
        if m: devices.append((int(m.group(1)), int(m.group(3)), m.group(2)))
    return devices

def probe_format(card, dev):
    for channels in [8, 2, 1]:
        for fmt in ['S32_LE', 'S16_LE', 'S24_LE']:
            try:
                p = subprocess.run(['arecord', '-D', f'hw:{card},{dev}', '-f', fmt, '-r', '48000', '-c', str(channels), '-d', '1', '/dev/null'], capture_output=True, text=True, timeout=5)
                if p.returncode == 0: return fmt, channels
            except: pass
    return None, None

def monitor_device(card, dev, fmt, channels):
    proc = subprocess.Popen(['arecord', '-D', f'hw:{card},{dev}', '-f', fmt, '-r', '48000', '-c', str(channels), '-t', 'raw', '-'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    bps = 4 if '32' in fmt else (3 if '24' in fmt else 2)
    chunk = 48000 * channels * bps // 10

    w = 50
    print(f"\n  Monitoring hw:{card},{dev} ({fmt}, {channels}ch, 48kHz)")
    print(f"  Strum your guitar! Ctrl+C to stop.\n")
    for _ in range(channels): print()

    try:
        while True:
            data = proc.stdout.read(chunk)
            if not data: break
            peaks = [0.0] * channels
            n = len(data) // (bps * channels)
            for i in range(0, n, 4):
                for ch in range(channels):
                    off = (i * channels + ch) * bps
                    if off + bps > len(data): break
                    if bps == 4: val = abs(struct.unpack_from('<i', data, off)[0] / 2147483648.0)
                    elif bps == 2: val = abs(struct.unpack_from('<h', data, off)[0] / 32768.0)
                    else: val = abs(int.from_bytes(data[off:off+3], 'little', signed=True) / 8388608.0)
                    if val > peaks[ch]: peaks[ch] = val

            sys.stdout.write(f'\033[{channels}F')
            for ch in range(channels):
                db = 20 * math.log10(peaks[ch]) if peaks[ch] > 1e-10 else -96
                db = max(-96, db)
                fill = int(max(0, min(w, (db + 96) / 96 * w)))
                if db > -20: c = '\033[91m'
                elif db > -40: c = '\033[92m'
                elif db > -60: c = '\033[93m'
                else: c = '\033[90m'
                bar = c + '█' * fill + '\033[0m' + '░' * (w - fill)
                print(f"  Ch{ch+1:d} [{bar}] {db:6.1f} dB")
    except KeyboardInterrupt: pass
    finally:
        proc.kill()
        proc.wait()

def main():
    print("\n  === Audio Input Monitor ===\n")
    devices = get_capture_devices()
    if not devices: print("  No capture devices found!"); return

    print("  Available capture devices:\n")
    for i, (card, dev, name) in enumerate(devices):
        fmt, ch = probe_format(card, dev)
        status = f"{fmt}/{ch}ch" if fmt else "unavailable"
        print(f"    {i+1}) hw:{card},{dev}  {name} ({status})")

    print()
    try:
        idx = int(input("  Select device (number): ").strip()) - 1
        if idx < 0 or idx >= len(devices): print("  Invalid."); return
    except (ValueError, EOFError): return

    card, dev, name = devices[idx]
    fmt, channels = probe_format(card, dev)
    if not fmt: print(f"  Can't open hw:{card},{dev}"); return
    monitor_device(card, dev, fmt, channels)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, lambda *_: sys.exit(0))
    main()
