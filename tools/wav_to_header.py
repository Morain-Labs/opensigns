# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Zac Morain (Morain Labs)
"""Convert pitch call recordings into C headers the firmware can play.

Usage:
    python tools/wav_to_header.py audio/source/fb.wav [more.wav ...]

For each WAV, ffmpeg resamples it to 44100 Hz 16-bit stereo PCM and writes
firmware/src/audio/<name>.raw, then this script writes
firmware/src/audio/<name>_wav.h with the samples as a PROGMEM array named
<name>_pcm and its length as <name>_pcm_len. Needs ffmpeg on PATH.

To add a pitch: record a short WAV, run this on it, then add an entry to
PITCHES[] in firmware/src/main.cpp and include the new header.
"""

import array
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
OUT_DIR = REPO / "firmware" / "src" / "audio"
PER_LINE = 16


def convert(wav: Path) -> None:
    name = wav.stem
    raw = OUT_DIR / f"{name}.raw"
    result = subprocess.run(
        [
            "ffmpeg",
            "-v",
            "error",
            "-y",
            "-i",
            str(wav),
            "-ar",
            "44100",
            "-ac",
            "2",
            "-f",
            "s16le",
            "-acodec",
            "pcm_s16le",
            str(raw),
        ],
    )
    if result.returncode != 0:
        raw.unlink(missing_ok=True)
        raise SystemExit(f"{wav}: ffmpeg could not read this file as audio")

    samples = array.array("h")
    samples.frombytes(raw.read_bytes())
    if sys.byteorder != "little":
        samples.byteswap()
    if not samples:
        raw.unlink(missing_ok=True)
        raise SystemExit(f"{wav}: ffmpeg produced no audio")

    rows = [
        "    " + ", ".join(str(s) for s in samples[i : i + PER_LINE])
        for i in range(0, len(samples), PER_LINE)
    ]
    try:
        source = wav.resolve().relative_to(REPO).as_posix()
    except ValueError:
        source = wav.name

    header = "\n".join(
        [
            f"// {wav.name} converted to 44100Hz 16-bit stereo PCM",
            f"// Generated from {source}",
            "#pragma once",
            "#include <stdint.h>",
            "",
            f"static const int16_t {name}_pcm[] PROGMEM = {{",
            ",\n".join(rows),
            "};",
            f"static const int {name}_pcm_len = {len(samples)};",
            "",
        ]
    )
    (OUT_DIR / f"{name}_wav.h").write_text(header, encoding="ascii", newline="\n")
    print(f"{name}: {len(samples)} samples -> {OUT_DIR / (name + '_wav.h')}")


def main() -> None:
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    for arg in sys.argv[1:]:
        convert(Path(arg))


if __name__ == "__main__":
    main()
