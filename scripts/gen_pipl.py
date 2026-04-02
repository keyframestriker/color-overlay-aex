#!/usr/bin/env python3
"""
gen_pipl.py  —  Generates the PiPL binary resource for ColorOverlay.aex

Background
    Adobe After Effects identifies effect plug-ins through a "PiPL"
    (Plug-in Property List) binary resource embedded in the DLL.
    Normally this is produced by Adobe's proprietary PiPLtool.exe, but we
    can generate the exact same binary manually.

    The format was reverse-engineered from real .aex files shipped with
    AE 2025 (e.g. BallAction.aex / CC Ball Action) using Python's struct
    module to walk the PE resource directory.

Binary layout
    Header  (10 bytes)
        01 00 00 00 00 00  — fixed prologue (version = 1)
        NN NN NN NN        — property count (LE uint32)

    Per property  (16 bytes header + data, padded to 4-byte boundary)
        [0..3]  creator  — 4-char OSType, byte-reversed for LE storage
        [4..7]  key      — 4-char OSType, byte-reversed for LE storage
        [8..11] index    — always 0  (LE uint32)
        [12..15] size    — actual data size in bytes  (LE uint32)
        [16..]  data     — padded with null bytes to next 4-byte boundary
                           (OSType values inside data are ALSO byte-reversed)

    OSType byte-reversal rule
        Mac/big-endian   : 'kind' stored as  [k, i, n, d]
        Windows/LE PiPL  : 'kind' stored as  [d, n, i, k]  (reversed)
        Verified against: kind='eFKT' → stored `54 4b 46 65`
                          kind='AEgx' → stored `78 67 45 41`

Output
    src/ColorOverlay.pipl.bin  (referenced by src/ColorOverlay.rc)

Usage
    python scripts/gen_pipl.py          # run from repo root
"""

import os
import struct

# ─── Helpers ──────────────────────────────────────────────────────────────────

def rev4(s: str) -> bytes:
    """Return 4-char OSType string as byte-reversed LE bytes (PiPL storage)."""
    return bytes(reversed(s.encode("ascii")))


def pad4(b: bytes) -> bytes:
    """Pad byte string to next 4-byte boundary with null bytes."""
    r = len(b) % 4
    return b if r == 0 else b + b"\x00" * (4 - r)


def prop(key: str, data: bytes, creator: str = "8BIM") -> bytes:
    """Build a single PiPL property block."""
    header = rev4(creator) + rev4(key) + struct.pack("<II", 0, len(data))
    return header + pad4(data)


def pascal_str(s: str) -> bytes:
    """
    AE Pascal string format: length_byte + ASCII chars + null terminator,
    padded to 4-byte boundary.
    """
    return pad4(bytes([len(s)]) + s.encode("ascii") + b"\x00")


def cstr(s: str) -> bytes:
    """Null-terminated C string, padded to 4-byte boundary."""
    return pad4(s.encode("ascii") + b"\x00")


# ─── Properties ───────────────────────────────────────────────────────────────
#
# Values confirmed against BallAction.aex (CC Ball Action, ships with AE 2025):
#   kind  = 'eFKT'  — standard AE effect kind (all CC/built-in effects use this)
#   ePVR  = 2       — PiPL format version
#   eSVR  = {13,28} — AE SDK spec version (PF_PLUG_IN_VERSION=13, subvers=28)
#   eGLO  = 0x02000000 = PF_OutFlag_DEEP_COLOR_AWARE (1<<25)
#   eINF  is 2 bytes (size=2), not 4
#
# IMPORTANT: MATCH_NAME ("KFSK Color Overlay") must be globally unique.
# Change the prefix if you fork/redistribute this plug-in.

PROPS = [
    prop("kind", rev4("eFKT")),                       # AE effect kind

    prop("name", pascal_str("Color Overlay")),         # Effect panel display name

    prop("catg", pascal_str("Stylize")),               # Effects menu category

    prop("8664", cstr("EffectMain")),                  # Win64 code entry point

    prop("ePVR", struct.pack("<I", 2)),                # PiPL version = 2

    prop("eSVR", struct.pack("<HH", 13, 28)),          # Spec version {13, 28}

    prop("eVER", struct.pack("<I", 0x00010000)),       # Plugin version 1.0

    prop("eINF", struct.pack("<H", 0)),                # Info flags = 0  (2 bytes!)

    prop("eGLO", struct.pack("<I", 0x02000000)),       # Global flags: DEEP_COLOR_AWARE

    prop("eGL2", struct.pack("<I", 0x00000000)),       # Global flags2 = 0

    prop("eMNA", pascal_str("KFSK Color Overlay")),    # Match name — MUST be unique

    prop("aeFL", struct.pack("<I", 0)),                # Reserved
]

# ─── Assemble ─────────────────────────────────────────────────────────────────

header  = b"\x01\x00\x00\x00\x00\x00" + struct.pack("<I", len(PROPS))
pipl    = header + b"".join(PROPS)

# ─── Write ────────────────────────────────────────────────────────────────────

repo_root   = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
output_path = os.path.join(repo_root, "src", "ColorOverlay.pipl.bin")

with open(output_path, "wb") as f:
    f.write(pipl)

print(f"[gen_pipl] wrote {len(pipl)} bytes → {output_path}")
print(f"[gen_pipl] hex: {pipl.hex()}")
