#!/usr/bin/env python3

import json
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "build" / "media-analyzer-core"


FIXTURES = {
    "sample.mp4": b"\x00\x00\x00\x18ftypisom\x00\x00\x02\x00isomiso2",
    "sample.webm": b"\x1a\x45\xdf\xa3\x9f\x42\x86\x81\x01\x42\xf7\x81\x01",
    "sample.ts": b"\x47" + (b"\x00" * 187) + b"\x47" + (b"\x00" * 187) + b"\x47" + (b"\x00" * 187),
    "sample.wav": b"RIFF\x24\x00\x00\x00WAVEfmt ",
    "sample.ogg": b"OggS\x00\x02",
    "sample.flac": b"fLaC\x00\x00\x00\x22",
    "sample.flv": b"FLV\x01\x05\x00\x00\x00\x09",
    "sample.mp3": b"ID3\x04\x00\x00\x00\x00\x00\x00",
    "sample.aac": b"\xff\xf1\x50\x80\x00\x1f\xfc",
    "sample.h264": b"\x00\x00\x00\x01\x67\x64\x00\x1f",
    "playlist.m3u8": b"#EXTM3U\n#EXT-X-VERSION:3\n",
    "manifest.mpd": b'<?xml version="1.0"?><MPD xmlns="urn:mpeg:dash:schema:mpd:2011"></MPD>',
}


EXPECTED = {
    "sample.mp4": "iso-bmff",
    "sample.webm": "matroska/webm",
    "sample.ts": "mpeg-ts",
    "sample.wav": "wav",
    "sample.ogg": "ogg",
    "sample.flac": "flac",
    "sample.flv": "flv",
    "sample.mp3": "mp3",
    "sample.aac": "adts-aac",
    "sample.h264": "annex-b-bitstream",
    "playlist.m3u8": "hls-m3u8",
    "manifest.mpd": "dash-mpd",
}


def ensure_binary() -> None:
    if BIN.exists():
        return
    subprocess.run(["cmake", "-S", str(ROOT), "-B", str(ROOT / "build")], check=True)
    subprocess.run(["cmake", "--build", str(ROOT / "build")], check=True)


def run_detector(path: Path) -> dict:
    result = subprocess.run([str(BIN), str(path)], check=True, text=True, capture_output=True)
    return json.loads(result.stdout)


def main() -> int:
    ensure_binary()
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        failures = []
        for name, payload in FIXTURES.items():
            path = tmp / name
            path.write_bytes(payload)
            actual = run_detector(path)
            expected = EXPECTED[name]
            if actual["format"] != expected:
                failures.append((name, expected, actual["format"], actual))

        if failures:
            for name, expected, actual, detail in failures:
                print(f"FAIL {name}: expected={expected} actual={actual} detail={detail}")
            return 1

    print(f"golden tests passed: {len(FIXTURES)} fixtures")
    return 0


if __name__ == "__main__":
    sys.exit(main())
