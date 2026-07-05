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
    "video_hvc1.mp4": None,
    "audio_aac.mp4": None,
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
    "video_hvc1.mp4": "iso-bmff",
    "audio_aac.mp4": "iso-bmff",
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


def mvhd_box() -> bytes:
    return fullbox(
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


def tkhd_box(track_id: int, duration: int, width: int = 0, height: int = 0) -> bytes:
    return fullbox(
        b"tkhd",
        be32(0)
        + be32(0)
        + be32(track_id)
        + be32(0)
        + be32(duration)
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
        + be32(width << 16)
        + be32(height << 16),
        flags=7,
    )


def hdlr_box(handler: bytes, name: bytes) -> bytes:
    return fullbox(b"hdlr", be32(0) + handler + (b"\x00" * 12) + name + b"\x00")


def make_video_hvc1_mp4() -> bytes:
    ftyp = box(b"ftyp", b"isom" + be32(512) + b"isomiso2hvc1mp41")
    vps = bytes.fromhex("40 01 0c 01 ff ff 01 60")
    sps = bytes.fromhex("42 01 01 01 60 00 00 03 00 90")
    pps = bytes.fromhex("44 01 c0 f1 83")
    hvcc_payload = (
        bytes([1, 1])
        + bytes.fromhex("60 00 00 00")
        + (b"\x00" * 6)
        + bytes([93])
        + be16(0xF000)
        + bytes([0xFC, 0xFD, 0xF8, 0xF8])
        + be16(0)
        + bytes([0x0F, 3])
        + bytes([0xA0]) + be16(1) + be16(len(vps)) + vps
        + bytes([0xA1]) + be16(1) + be16(len(sps)) + sps
        + bytes([0xA2]) + be16(1) + be16(len(pps)) + pps
    )
    hvcc = box(b"hvcC", hvcc_payload)
    hvc1_payload = (
        (b"\x00" * 6)
        + be16(1)
        + be16(0)
        + be16(0)
        + (b"\x00" * 12)
        + be16(1280)
        + be16(720)
        + be32(0x00480000)
        + be32(0x00480000)
        + be32(0)
        + be16(1)
        + bytes([0])
        + (b"\x00" * 31)
        + be16(24)
        + be16(0xFFFF)
        + hvcc
    )
    hvc1 = box(b"hvc1", hvc1_payload)
    stsd = fullbox(b"stsd", be32(1) + hvc1)
    stsz = fullbox(b"stsz", be32(0) + be32(1) + be32(4321))
    stbl = box(b"stbl", stsd + stsz)
    minf = box(b"minf", stbl)
    mdia = box(
        b"mdia",
        fullbox(b"mdhd", be32(0) + be32(0) + be32(90000) + be32(180000) + be16(0x55C4) + be16(0))
        + hdlr_box(b"vide", b"VideoHandler")
        + minf,
    )
    trak = box(b"trak", tkhd_box(1, 2000, 1280, 720) + mdia)
    return ftyp + box(b"moov", mvhd_box() + trak)


def descriptor(tag: int, payload: bytes) -> bytes:
    assert len(payload) < 128
    return bytes([tag, len(payload)]) + payload


def make_audio_aac_mp4() -> bytes:
    ftyp = box(b"ftyp", b"M4A " + be32(512) + b"M4A mp42isom")
    asc = bytes.fromhex("12 10")
    decoder_config = descriptor(
        0x04,
        bytes([0x40, 0x15])
        + b"\x00\x00\x00"
        + be32(128000)
        + be32(128000)
        + descriptor(0x05, asc),
    )
    es_descriptor = descriptor(0x03, be16(1) + bytes([0]) + decoder_config)
    esds = fullbox(b"esds", es_descriptor)
    mp4a_payload = (
        (b"\x00" * 6)
        + be16(1)
        + be16(0)
        + be16(0)
        + be32(0)
        + be16(2)
        + be16(16)
        + be16(0)
        + be16(0)
        + be32(44100 << 16)
        + esds
    )
    mp4a = box(b"mp4a", mp4a_payload)
    stsd = fullbox(b"stsd", be32(1) + mp4a)
    stsz = fullbox(b"stsz", be32(0) + be32(1) + be32(512))
    stbl = box(b"stbl", stsd + stsz)
    minf = box(b"minf", stbl)
    mdia = box(
        b"mdia",
        fullbox(b"mdhd", be32(0) + be32(0) + be32(44100) + be32(88200) + be16(0x55C4) + be16(0))
        + hdlr_box(b"soun", b"SoundHandler")
        + minf,
    )
    trak = box(b"trak", tkhd_box(1, 2000) + mdia)
    return ftyp + box(b"moov", mvhd_box() + trak)


FIXTURES["video_avc1.mp4"] = make_video_avc1_mp4()
FIXTURES["video_hvc1.mp4"] = make_video_hvc1_mp4()
FIXTURES["audio_aac.mp4"] = make_audio_aac_mp4()


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
            if "input" in actual:
                try:
                    assert actual["input"]["bytes"]["offset"] == 0
                    assert actual["input"]["bytes"]["length"] > 0
                    assert actual["input"]["bytes"]["hex"]
                except Exception as exc:
                    failures.append((name, "input byte preview", f"error: {exc}", actual))
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
                    assert track["codec"]["raw_header_bytes"]["offset"] > 0
                    assert track["codec"]["raw_header_bytes"]["length"] > 8
                    assert track["codec"]["sps_hex"].startswith("67 64 00 1f")
                    assert track["codec"]["pps_hex"].startswith("68 eb")
                    assert actual["container"]["structure"][0]["bytes"]["hex"].startswith("00 00 00")
                except Exception as exc:
                    failures.append((name, "parsed AVC track", f"error: {exc}", actual))
            if name == "video_hvc1.mp4":
                try:
                    track = actual["container"]["tracks"][0]
                    codec = track["codec"]
                    assert track["type"] == "video"
                    assert track["width"] == 1280
                    assert track["height"] == 720
                    assert codec["fourcc"] == "hvc1"
                    assert codec["description"] == "H.265/HEVC"
                    assert codec["profile"] == "Main"
                    assert codec["level"] == "Main tier 3.1"
                    assert codec["length_size"] == 4
                    assert codec["vps_count"] == 1
                    assert codec["sps_count"] == 1
                    assert codec["pps_count"] == 1
                    assert codec["bit_depth_luma"] == 8
                    assert codec["bit_depth_chroma"] == 8
                    assert codec["chroma_format"] == 1
                    assert codec["vps_hex"].startswith("40 01")
                    assert codec["raw_header_bytes"]["offset"] > 0
                    assert codec["raw_header_bytes"]["length"] > 8
                except Exception as exc:
                    failures.append((name, "parsed HEVC track", f"error: {exc}", actual))
            if name == "audio_aac.mp4":
                try:
                    track = actual["container"]["tracks"][0]
                    codec = track["codec"]
                    assert track["type"] == "audio"
                    assert track["sample_rate"] == 44100
                    assert track["channel_count"] == 2
                    assert codec["fourcc"] == "mp4a"
                    assert codec["description"] == "AAC"
                    assert codec["profile"] == "AAC LC"
                    assert codec["audio_object_type"] == 2
                    assert codec["asc_sample_rate"] == 44100
                    assert codec["channel_config"] == 2
                    assert codec["asc_hex"] == "12 10"
                    assert codec["raw_header_bytes"]["offset"] > 0
                    assert codec["raw_header_bytes"]["length"] > 8
                except Exception as exc:
                    failures.append((name, "parsed AAC track", f"error: {exc}", actual))

        if failures:
            for name, expected, actual, detail in failures:
                print(f"FAIL {name}: expected={expected} actual={actual} detail={detail}")
            return 1

    print(f"golden tests passed: {len(FIXTURES)} fixtures")
    return 0


if __name__ == "__main__":
    sys.exit(main())
