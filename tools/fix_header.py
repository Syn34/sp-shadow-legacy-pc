#!/usr/bin/env python3
"""Restore the secure-area CRC in a rebuilt ROM header and recompute the header CRC.

The secure-area CRC (header 0x6C) is a CRC over the *encrypted* first 16 KiB of
ARM9 code. Computing it needs the KEY1 table from the DS ARM7 BIOS, which this
project does not ship. When the secure area is byte-identical to the original
game, the original CRC is still valid, so we restore it. If the secure area was
changed (non-matching build), the field is left at 0; emulators and flashcarts
do not check it.

Usage: fix_header.py <rom.nds>
"""
import hashlib
import struct
import sys

# Known-good values for Spyro: Shadow Legacy (USA), ASSE
ORIGINAL = {
    "secure_area_sha1": "6aa74ae30d472e4b77a649c29e47541b04bcd8e0",
    "secure_area_crc": 0x809C,
}


def crc16(data, crc=0xFFFF):
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def main():
    path = sys.argv[1]
    rom = bytearray(open(path, "rb").read())
    arm9_off = struct.unpack_from("<I", rom, 0x20)[0]
    secure = bytes(rom[arm9_off:arm9_off + 0x4000])
    if hashlib.sha1(secure).hexdigest() == ORIGINAL["secure_area_sha1"]:
        struct.pack_into("<H", rom, 0x6C, ORIGINAL["secure_area_crc"])
        note = "secure area unchanged, original secure CRC restored"
    else:
        note = "secure area modified, secure CRC left as-is"
    struct.pack_into("<H", rom, 0x15E, crc16(rom[:0x15E]))
    open(path, "wb").write(rom)
    print(f"fix_header: {note}")


if __name__ == "__main__":
    main()
