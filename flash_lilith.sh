#!/bin/bash
# Flash Lilith firmware to LilyGo T-Deck Plus
# Usage: ./flash_lilith.sh [port]
# Default port: /dev/ttyUSB0

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD="$SCRIPT_DIR/build/esp32.esp32.esp32s3"
PORT="${1:-/dev/ttyUSB0}"

if [ ! -f "$BUILD/Lilith.ino.bin" ]; then
    echo "ERROR: Build artifacts not found. Compile first with Arduino IDE or arduino-cli."
    exit 1
fi

echo "=== Flashing Lilith to $PORT ==="
echo "  Firmware: $(stat -c%s $BUILD/Lilith.ino.bin) bytes"

python3 -m esptool --chip esp32s3 --port "$PORT" --baud 921600 \
    write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB \
    0x0     "$BUILD/Lilith.ino.bootloader.bin" \
    0x8000  "$BUILD/Lilith.ino.partitions.bin" \
    0xe000  "$BUILD/boot_app0.bin" \
    0x10000 "$BUILD/Lilith.ino.bin"

echo ""
echo "=== Done. Serial monitor: ==="
python3 -c "
import serial, time
s = serial.Serial('$PORT', 115200, timeout=3)
s.dtr = False; s.rts = True; time.sleep(0.1); s.rts = False
start = time.time()
while time.time() - start < 15:
    line = s.readline()
    if line:
        print(line.decode('utf-8', errors='replace').rstrip())
s.close()
" 2>/dev/null || true
