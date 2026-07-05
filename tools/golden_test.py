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
    "sample.ogg": None,
    "sample.flac": None,
    "sample.flv": b"FLV\x01\x05\x00\x00\x00\x09",
    "sample.mp3": None,
    "sample.aac": b"\xff\xf1\x50\x80\x00\xff\xfc",
    "sample.h264": None,
    "sample.h265": None,
    "sample.av1": None,
    "sample.opushead": None,
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
    "sample.h264": "h264-annex-b",
    "sample.h265": "hevc-annex-b",
    "sample.av1": "av1-obu",
    "sample.opushead": "opus-head",
    "playlist.m3u8": "hls-m3u8",
    "manifest.mpd": "dash-mpd",
}


def be16(value: int) -> bytes:
    return value.to_bytes(2, "big")


def be32(value: int) -> bytes:
    return value.to_bytes(4, "big")


def le16(value: int) -> bytes:
    return value.to_bytes(2, "little")


def le32(value: int) -> bytes:
    return value.to_bytes(4, "little")


def le64(value: int) -> bytes:
    return value.to_bytes(8, "little")


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
    sample_payload = bytes(range(32))
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

    def build_moov(chunk_offset: int) -> bytes:
        stsd = fullbox(b"stsd", be32(1) + hvc1)
        stts = fullbox(b"stts", be32(1) + be32(1) + be32(3000))
        stsc = fullbox(b"stsc", be32(1) + be32(1) + be32(1) + be32(1))
        stsz = fullbox(b"stsz", be32(0) + be32(1) + be32(len(sample_payload)))
        stco = fullbox(b"stco", be32(1) + be32(chunk_offset))
        stss = fullbox(b"stss", be32(1) + be32(1))
        stbl = box(b"stbl", stsd + stts + stsc + stsz + stco + stss)
        minf = box(b"minf", stbl)
        mdia = box(
            b"mdia",
            fullbox(b"mdhd", be32(0) + be32(0) + be32(90000) + be32(180000) + be16(0x55C4) + be16(0))
            + hdlr_box(b"vide", b"VideoHandler")
            + minf,
        )
        trak = box(b"trak", tkhd_box(1, 2000, 1280, 720) + mdia)
        return box(b"moov", mvhd_box() + trak)

    mdat = box(b"mdat", sample_payload)
    moov = build_moov(0)
    moov = build_moov(len(ftyp) + len(moov) + 8)
    return ftyp + moov + mdat


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


class BitWriter:
    def __init__(self) -> None:
        self.bits: list[int] = []

    def write(self, value: int, count: int) -> None:
        for shift in range(count - 1, -1, -1):
            self.bits.append((value >> shift) & 1)

    def bytes(self) -> bytes:
        out = bytearray()
        for index in range(0, len(self.bits), 8):
            byte = 0
            for bit in self.bits[index:index + 8]:
                byte = (byte << 1) | bit
            byte <<= max(0, 8 - len(self.bits[index:index + 8]))
            out.append(byte)
        return bytes(out)


def opus_head() -> bytes:
    return b"OpusHead" + bytes([1, 2]) + le16(312) + le32(48000) + le16(0) + bytes([0])


def make_ogg_opus() -> bytes:
    payload = opus_head()
    return (
        b"OggS"
        + bytes([0, 2])
        + le64(0)
        + le32(0x12345678)
        + le32(0)
        + le32(0)
        + bytes([1, len(payload)])
        + payload
    )


def make_flac_stream() -> bytes:
    sample_rate = 44100
    channels = 2
    bits_per_sample = 16
    total_samples = 44100
    packed = (
        (sample_rate << 44)
        | ((channels - 1) << 41)
        | ((bits_per_sample - 1) << 36)
        | total_samples
    ).to_bytes(8, "big")
    streaminfo = (
        be16(4096)
        + be16(4096)
        + b"\x00\x00\x10"
        + b"\x00\x20\x00"
        + packed
        + (b"\x00" * 16)
    )
    return b"fLaC" + bytes([0x80]) + b"\x00\x00\x22" + streaminfo


def make_mp3_stream() -> bytes:
    frame_length = 417
    frame = bytes.fromhex("ff fb 90 64") + (b"\x00" * (frame_length - 4))
    return b"ID3\x04\x00\x00\x00\x00\x00\x00" + frame


def make_h264_annex_b() -> bytes:
    sps = bytes.fromhex("6764001facd940a02ff970110000030001000003003c8f162d96")
    pps = bytes.fromhex("68ebe3cb22c0")
    start = b"\x00\x00\x00\x01"
    return start + sps + start + pps


def make_hevc_annex_b() -> bytes:
    vps = bytes.fromhex("40 01 0c 01 ff ff 01 60")
    sps = bytes.fromhex("42 01 01 01 60 00 00 03 00 90")
    pps = bytes.fromhex("44 01 c0 f1 83")
    start = b"\x00\x00\x00\x01"
    return start + vps + start + sps + start + pps


def make_av1_obu() -> bytes:
    bits = BitWriter()
    bits.write(0, 3)    # seq_profile
    bits.write(1, 1)    # still_picture
    bits.write(1, 1)    # reduced_still_picture_header
    bits.write(0, 5)    # seq_level_idx_0
    bits.write(9, 4)    # frame_width_bits_minus_1 => 10 bits
    bits.write(8, 4)    # frame_height_bits_minus_1 => 9 bits
    bits.write(639, 10) # max_frame_width_minus_1
    bits.write(359, 9)  # max_frame_height_minus_1
    payload = bits.bytes()
    assert len(payload) < 128
    return bytes([0x0A, len(payload)]) + payload


FIXTURES["video_avc1.mp4"] = make_video_avc1_mp4()
FIXTURES["video_hvc1.mp4"] = make_video_hvc1_mp4()
FIXTURES["audio_aac.mp4"] = make_audio_aac_mp4()
FIXTURES["sample.ogg"] = make_ogg_opus()
FIXTURES["sample.flac"] = make_flac_stream()
FIXTURES["sample.mp3"] = make_mp3_stream()
FIXTURES["sample.h264"] = make_h264_annex_b()
FIXTURES["sample.h265"] = make_hevc_annex_b()
FIXTURES["sample.av1"] = make_av1_obu()
FIXTURES["sample.opushead"] = opus_head()


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
                    assert track["codec"]["sps_bytes"]["length"] > 4
                    assert track["codec"]["pps_bytes"]["length"] > 2
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
                    assert codec["vps_bytes"]["length"] == 8
                    assert codec["sps_bytes"]["length"] == 10
                    assert codec["pps_bytes"]["length"] == 5
                    assert track["sample_table_total"] == 1
                    assert track["sample_table_truncated"] is False
                    sample = track["samples"][0]
                    assert sample["index"] == 1
                    assert sample["size"] == 32
                    assert sample["duration"] == 3000
                    assert sample["dts"] == 0
                    assert sample["pts"] == 0
                    assert sample["sync"] is True
                    assert sample["bytes"]["offset"] == sample["offset"]
                    assert sample["bytes"]["length"] == 32
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
                    assert codec["asc_bytes"]["length"] == 2
                except Exception as exc:
                    failures.append((name, "parsed AAC track", f"error: {exc}", actual))
            if name == "sample.ts":
                try:
                    assert actual["container"]["format"] == "MPEG-TS"
                    assert actual["container"]["packet_count"] == 3
                    assert actual["container"]["packets"][0]["offset"] == 0
                    assert actual["container"]["packets"][0]["length"] == 188
                    assert actual["container"]["packets"][0]["sync"] is True
                except Exception as exc:
                    failures.append((name, "parsed MPEG-TS packets", f"error: {exc}", actual))
            if name == "sample.webm":
                try:
                    assert actual["container"]["format"] == "Matroska/WebM"
                    assert actual["container"]["element_count"] >= 1
                    assert actual["container"]["elements"][0]["id_value"] == 0x1A45DFA3
                except Exception as exc:
                    failures.append((name, "parsed EBML elements", f"error: {exc}", actual))
            if name == "sample.ogg":
                try:
                    assert actual["container"]["format"] == "Ogg"
                    assert actual["container"]["page_count"] == 1
                    assert actual["container"]["pages"][0]["offset"] == 0
                    assert actual["container"]["pages"][0]["body_length"] == len(opus_head())
                    opus = actual["container"]["opus_head"]
                    assert opus["offset"] == 28
                    assert opus["length"] == 19
                    assert opus["channel_count"] == 2
                    assert opus["input_sample_rate"] == 48000
                    assert opus["pre_skip"] == 312
                except Exception as exc:
                    failures.append((name, "parsed Ogg OpusHead", f"error: {exc}", actual))
            if name == "sample.flac":
                try:
                    assert actual["container"]["format"] == "FLAC"
                    block = actual["container"]["metadata_blocks"][0]
                    assert block["offset"] == 4
                    assert block["length"] == 38
                    assert block["block_name"] == "STREAMINFO"
                    assert block["sample_rate"] == 44100
                    assert block["channels"] == 2
                    assert block["bits_per_sample"] == 16
                    assert block["total_samples"] == 44100
                except Exception as exc:
                    failures.append((name, "parsed FLAC STREAMINFO", f"error: {exc}", actual))
            if name == "sample.mp3":
                try:
                    assert actual["bitstream"]["format"] == "MP3"
                    assert actual["bitstream"]["id3"]["length"] == 10
                    frame = actual["bitstream"]["frames"][0]
                    assert frame["offset"] == 10
                    assert frame["length"] == 417
                    assert frame["version"] == "MPEG 1"
                    assert frame["layer"] == "Layer III"
                    assert frame["bitrate_kbps"] == 128
                    assert frame["sample_rate"] == 44100
                except Exception as exc:
                    failures.append((name, "parsed MP3 frame", f"error: {exc}", actual))
            if name == "sample.aac":
                try:
                    frame = actual["bitstream"]["frames"][0]
                    assert actual["bitstream"]["format"] == "ADTS AAC"
                    assert frame["offset"] == 0
                    assert frame["length"] == 7
                    assert frame["sample_rate"] == 44100
                    assert frame["channel_config"] == 2
                except Exception as exc:
                    failures.append((name, "parsed ADTS frame", f"error: {exc}", actual))
            if name == "sample.h264":
                try:
                    nalu = actual["bitstream"]["nal_units"][0]
                    assert actual["bitstream"]["format"] == "Annex B H.264"
                    assert nalu["offset"] == 4
                    assert nalu["nal_type"] == 7
                    assert nalu["nal_name"] == "SPS"
                    assert actual["bitstream"]["parameter_set_count"] == 2
                    sps = actual["bitstream"]["parameter_sets"][0]
                    assert sps["kind"] == "SPS"
                    assert sps["offset"] == 4
                    assert sps["profile"] == "High"
                    assert sps["level"] == "3.1"
                    assert sps["width"] == 640
                    assert sps["height"] == 360
                except Exception as exc:
                    failures.append((name, "parsed H.264 Annex B parameter sets", f"error: {exc}", actual))
            if name == "sample.h265":
                try:
                    assert actual["bitstream"]["format"] == "Annex B HEVC"
                    assert actual["bitstream"]["codec"] == "H.265/HEVC"
                    assert actual["bitstream"]["nal_units"][0]["nal_type"] == 32
                    assert actual["bitstream"]["nal_units"][0]["nal_name"] == "VPS"
                    kinds = [item["kind"] for item in actual["bitstream"]["parameter_sets"]]
                    assert kinds == ["VPS", "SPS", "PPS"]
                except Exception as exc:
                    failures.append((name, "parsed HEVC Annex B parameter sets", f"error: {exc}", actual))
            if name == "sample.av1":
                try:
                    assert actual["bitstream"]["format"] == "AV1 OBU"
                    obu = actual["bitstream"]["obu_units"][0]
                    assert obu["obu_type"] == 1
                    assert obu["obu_name"] == "Sequence Header"
                    seq = actual["bitstream"]["sequence_headers"][0]
                    assert seq["offset"] == 2
                    assert seq["seq_profile"] == 0
                    assert seq["still_picture"] is True
                    assert seq["max_frame_width"] == 640
                    assert seq["max_frame_height"] == 360
                except Exception as exc:
                    failures.append((name, "parsed AV1 sequence header", f"error: {exc}", actual))
            if name == "sample.opushead":
                try:
                    assert actual["bitstream"]["format"] == "OpusHead"
                    opus = actual["bitstream"]["opus_head"]
                    assert opus["offset"] == 0
                    assert opus["length"] == 19
                    assert opus["channel_count"] == 2
                    assert opus["input_sample_rate"] == 48000
                except Exception as exc:
                    failures.append((name, "parsed raw OpusHead", f"error: {exc}", actual))

        if failures:
            for name, expected, actual, detail in failures:
                print(f"FAIL {name}: expected={expected} actual={actual} detail={detail}")
            return 1

    print(f"golden tests passed: {len(FIXTURES)} fixtures")
    return 0


if __name__ == "__main__":
    sys.exit(main())
