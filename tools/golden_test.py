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
    "video_avc1.mp4": None,
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
    "video_avc1.mp4": "iso-bmff",
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


def be16(value: int) -> bytes:
    return value.to_bytes(2, "big")


def be32(value: int) -> bytes:
    return value.to_bytes(4, "big")


def box(kind: bytes, payload: bytes) -> bytes:
    return be32(8 + len(payload)) + kind + payload


def fullbox(kind: bytes, payload: bytes, version: int = 0, flags: int = 0) -> bytes:
    return box(kind, bytes([version]) + flags.to_bytes(3, "big") + payload)


def make_video_avc1_mp4() -> bytes:
    ftyp = box(b"ftyp", b"isom" + be32(512) + b"isomiso2avc1mp41")
    mvhd = fullbox(
        b"mvhd",
        be32(0)
        + be32(0)
        + be32(1000)
        + be32(2000)
        + be32(0x00010000)
        + be16(0x0100)
        + be16(0)
        + (b"\x00" * 8)
        + be32(0x00010000)
        + be32(0)
        + be32(0)
        + be32(0)
        + be32(0x00010000)
        + be32(0)
        + be32(0)
        + be32(0)
        + be32(0x40000000)
        + (b"\x00" * 24)
        + be32(2),
    )
    tkhd = fullbox(
        b"tkhd",
        be32(0)
        + be32(0)
        + be32(1)
        + be32(0)
        + be32(2000)
        + (b"\x00" * 8)
        + be16(0)
        + be16(0)
        + be16(0)
        + be16(0)
        + be32(0x00010000)
        + be32(0)
        + be32(0)
        + be32(0)
        + be32(0x00010000)
        + be32(0)
        + be32(0)
        + be32(0)
        + be32(0x40000000)
        + be32(640 << 16)
        + be32(360 << 16),
        flags=7,
    )
    mdhd = fullbox(b"mdhd", be32(0) + be32(0) + be32(90000) + be32(180000) + be16(0x55C4) + be16(0))
    hdlr = fullbox(b"hdlr", be32(0) + b"vide" + (b"\x00" * 12) + b"VideoHandler\x00")

    sps = bytes.fromhex("6764001facd940a02ff970110000030001000003003c8f162d96")
    pps = bytes.fromhex("68ebe3cb22c0")
    avcc = box(
        b"avcC",
        bytes([1, 100, 0, 31, 0xFF, 0xE1])
        + be16(len(sps))
        + sps
        + bytes([1])
        + be16(len(pps))
        + pps,
    )
    avc1_payload = (
        (b"\x00" * 6)
        + be16(1)
        + be16(0)
        + be16(0)
        + (b"\x00" * 12)
        + be16(640)
        + be16(360)
        + be32(0x00480000)
        + be32(0x00480000)
        + be32(0)
        + be16(1)
        + bytes([0])
        + (b"\x00" * 31)
        + be16(24)
        + be16(0xFFFF)
        + avcc
    )
    avc1 = box(b"avc1", avc1_payload)
    stsd = fullbox(b"stsd", be32(1) + avc1)
    stsz = fullbox(b"stsz", be32(0) + be32(1) + be32(1234))
    stbl = box(b"stbl", stsd + stsz)
    minf = box(b"minf", stbl)
    mdia = box(b"mdia", mdhd + hdlr + minf)
    trak = box(b"trak", tkhd + mdia)
    moov = box(b"moov", mvhd + trak)
    return ftyp + moov


FIXTURES["video_avc1.mp4"] = make_video_avc1_mp4()


def ensure_binary() -> None:
    if BIN.exists():
        return
    subprocess.run(["cmake", "-S", str(ROOT), "-B", str(ROOT / "build")], check=True)
    subprocess.run(["cmake", "--build", str(ROOT / "build")], check=True)


def run_detector(path: Path) -> dict:
    result = subprocess.run([str(BIN), str(path)], check=True, text=True, capture_output=True)
    return json.loads(result.stdout)


def detection_format(payload: dict) -> str:
    if "detection" in payload:
        return payload["detection"]["format"]
    return payload["format"]


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
            actual_format = detection_format(actual)
            if actual_format != expected:
                failures.append((name, expected, actual_format, actual))
            if name == "video_avc1.mp4":
                try:
                    track = actual["container"]["tracks"][0]
                    assert track["type"] == "video"
                    assert track["width"] == 640
                    assert track["height"] == 360
                    assert track["sample_count"] == 1
                    assert track["codec"]["fourcc"] == "avc1"
                    assert track["codec"]["description"] == "H.264/AVC"
                    assert track["codec"]["profile"]
                    assert track["codec"]["sps_count"] == 1
                    assert track["codec"]["pps_count"] == 1
                    assert track["codec"]["raw_header_hex"].startswith("01 64 00 1f")
                    assert track["codec"]["sps_hex"].startswith("67 64 00 1f")
                    assert track["codec"]["pps_hex"].startswith("68 eb")
                    assert actual["container"]["structure"][0]["bytes"]["hex"].startswith("00 00 00")
                except Exception as exc:
                    failures.append((name, "parsed AVC track", f"error: {exc}", actual))

        if failures:
            for name, expected, actual, detail in failures:
                print(f"FAIL {name}: expected={expected} actual={actual} detail={detail}")
            return 1

    print(f"golden tests passed: {len(FIXTURES)} fixtures")
    return 0


if __name__ == "__main__":
    sys.exit(main())
