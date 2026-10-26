# OpenSkyhawk — post-build step for production STM32 images (#362). PlatformIO runs it after linking:
#   1. stamp: fill in the identity block's imageLength and imageCrc inside firmware.elf
#   2. make firmware.bin from the stamped .elf with gaps filled 0xFF — what an ST-Link flash leaves —
#      so the .bin, the .elf and the board's flash all give the same CRC.
# The block is placed by stm32f103.ld at the app's start + 0x200; its layout is FirmwareImage.h.
# imageCrc = CRC-32/MPEG-2 over imageLength bytes (32-bit little-endian words), the imageCrc word skipped.
import os
import pathlib
import struct
import subprocess

IDENTITY_OFFSET = 0x200
MAGIC = b"OSKY"
OFF_LENGTH, OFF_CRC = 8, 12


def crc32_mpeg2(data, skip_word=None):
    c = 0xFFFFFFFF
    for i in range(0, len(data), 4):
        if i == skip_word:
            continue
        c ^= int.from_bytes(data[i:i + 4], "little")
        for _ in range(32):
            c = ((c << 1) ^ 0x04C11DB7) & 0xFFFFFFFF if c & 0x80000000 else (c << 1) & 0xFFFFFFFF
    return c


def section_file_offset(elf, name):
    """File offset of an ELF32 section by name — enough of the format to patch bytes in place."""
    b = pathlib.Path(elf).read_bytes()
    shoff, = struct.unpack_from("<I", b, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", b, 0x2E)
    def sh(i):
        return struct.unpack_from("<IIIIII", b, shoff + i * shentsize)   # name, type, flags, addr, offset, size
    strtab = sh(shstrndx)[4]
    for i in range(shnum):
        n, _, _, addr, off, size = sh(i)
        end = b.index(b"\0", strtab + n)
        if b[strtab + n:end].decode() == name:
            return off, addr, size
    return None


def stamp_image(img):
    """Pad a flat image to whole words, write its length into the block, and compute its CRC.
    The CRC covers the stamped length, so the length goes in first; only the CRC word is skipped."""
    length = (len(img) + 3) & ~3
    img = bytearray(img + b"\xFF" * (length - len(img)))
    img[IDENTITY_OFFSET + OFF_LENGTH:IDENTITY_OFFSET + OFF_LENGTH + 4] = struct.pack("<I", length)
    crc = crc32_mpeg2(img, skip_word=IDENTITY_OFFSET + OFF_CRC)
    img[IDENTITY_OFFSET + OFF_CRC:IDENTITY_OFFSET + OFF_CRC + 4] = struct.pack("<I", crc)
    return img, length, crc


def flat_image(objcopy, elf, out):
    subprocess.check_call([objcopy, "-O", "binary", "--gap-fill", "0xFF", elf, out])
    return pathlib.Path(out).read_bytes()


def stamp(target, source, env):
    elf = str(target[0])
    objcopy = env.subst("$OBJCOPY")
    sec = section_file_offset(elf, ".isr_vector")
    img = flat_image(objcopy, elf, elf + ".flat")
    os.remove(elf + ".flat")
    if not sec or img[IDENTITY_OFFSET:IDENTITY_OFFSET + 4] != MAGIC:
        print("osh_image: no identity block in this image — not stamped")
        return
    img, length, crc = stamp_image(img)
    with open(elf, "r+b") as f:
        f.seek(sec[0] + IDENTITY_OFFSET + OFF_LENGTH)
        f.write(struct.pack("<II", length, crc))
    node, kind, ma, mi, pa = img[IDENTITY_OFFSET + 16:IDENTITY_OFFSET + 21]
    print(f"osh_image: node {node} · kind {kind} · v{ma}.{mi}.{pa} · {length} B · CRC {crc:08X}")


def rebuild_bin(target, source, env):
    flat_image(env.subst("$OBJCOPY"), str(source[0]), str(target[0]))


try:
    Import("env")  # noqa: F821 — provided by PlatformIO; absent when the tests import this file
except NameError:
    env = None
if env is not None:
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", stamp)
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", rebuild_bin)
