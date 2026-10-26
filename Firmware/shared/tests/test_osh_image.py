"""Tests for Firmware/shared/osh_image.py — the post-build stamp (#362).

The bootloader (#363) and the uploader (#366) recompute imageCrc on their side, so the algorithm
and the field offsets here must match FirmwareImage.h exactly. The header's test vectors and
offsets are read from the header itself, so a change on either side fails here.
"""
import importlib.util
import pathlib
import re
import struct

HERE = pathlib.Path(__file__).resolve().parent
HEADER = (HERE.parent.parent / "Libraries" / "FirmwareImage" / "FirmwareImage.h").read_text()

spec = importlib.util.spec_from_file_location("osh_image", HERE.parent / "osh_image.py")
osh = importlib.util.module_from_spec(spec)
spec.loader.exec_module(osh)


def header_const(name):
    m = re.search(rf"\b{name}\s*=\s*(0x[0-9A-Fa-f]+|\d+)", HEADER)
    assert m, f"{name} not found in FirmwareImage.h"
    return int(m.group(1), 0)


def test_crc_matches_header_vectors():
    assert osh.crc32_mpeg2(struct.pack("<I", 0x12345678)) == header_const("CRC_VECTOR_WORD_12345678")
    assert osh.crc32_mpeg2(bytes(range(16))) == header_const("CRC_VECTOR_BYTES_00_0F")


def test_layout_matches_header():
    assert osh.IDENTITY_OFFSET == header_const("IDENTITY_OFFSET")
    assert osh.OFF_LENGTH == header_const("OFF_LENGTH")
    assert osh.OFF_CRC == header_const("OFF_CRC")
    assert struct.unpack("<I", osh.MAGIC)[0] == header_const("MAGIC")


def fake_image(size):
    img = bytearray(b"\xA5" * size)
    blk = osh.IDENTITY_OFFSET
    img[blk:blk + 16] = osh.MAGIC + struct.pack("<HHII", 1, 32, 0xFFFFFFFF, 0xFFFFFFFF)
    return bytes(img)


def test_stamp_pads_to_words_with_ff():
    img, length, _ = osh.stamp_image(fake_image(0x403))
    assert length == 0x404 and len(img) == length and img[-1] == 0xFF


def test_stamped_image_verifies_like_the_bootloader():
    # What the bootloader does: read the length and CRC from the block, recompute over that many
    # bytes with the CRC word skipped. The stamped length is inside the CRC.
    img, length, crc = osh.stamp_image(fake_image(0x800))
    blk = osh.IDENTITY_OFFSET
    got_len, got_crc = struct.unpack_from("<II", img, blk + osh.OFF_LENGTH)
    assert (got_len, got_crc) == (length, crc)
    assert osh.crc32_mpeg2(img[:got_len], skip_word=blk + osh.OFF_CRC) == got_crc


def test_any_changed_byte_changes_the_crc():
    img, _, crc = osh.stamp_image(fake_image(0x800))
    for pos in (0, osh.IDENTITY_OFFSET + osh.OFF_LENGTH, 0x7FF):
        bad = bytearray(img)
        bad[pos] ^= 0x01
        assert osh.crc32_mpeg2(bad, skip_word=osh.IDENTITY_OFFSET + osh.OFF_CRC) != crc
