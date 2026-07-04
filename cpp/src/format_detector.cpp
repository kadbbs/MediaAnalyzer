#include "media_analyzer/format_detector.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace media_analyzer {
namespace {

bool HasBytes(std::span<const std::uint8_t> data, std::size_t offset,
              std::string_view bytes) {
  if (data.size() < offset + bytes.size()) {
    return false;
  }
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    if (data[offset + i] != static_cast<std::uint8_t>(bytes[i])) {
      return false;
    }
  }
  return true;
}

bool HasBytesAtZero(std::span<const std::uint8_t> data, std::string_view bytes) {
  return HasBytes(data, 0, bytes);
}

std::uint32_t ReadBe32(std::span<const std::uint8_t> data, std::size_t offset) {
  if (data.size() < offset + 4) {
    return 0;
  }
  return (static_cast<std::uint32_t>(data[offset]) << 24) |
         (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
         static_cast<std::uint32_t>(data[offset + 3]);
}

std::string Lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

bool EndsWith(std::string_view value, std::string_view suffix) {
  if (value.size() < suffix.size()) {
    return false;
  }
  return value.substr(value.size() - suffix.size()) == suffix;
}

bool LooksLikeMp3Frame(std::span<const std::uint8_t> data, std::size_t offset) {
  if (data.size() < offset + 2) {
    return false;
  }
  return data[offset] == 0xFF && (data[offset + 1] & 0xE0) == 0xE0 &&
         (data[offset + 1] & 0x18) != 0x08 && (data[offset + 1] & 0x06) != 0x00;
}

bool LooksLikeAdts(std::span<const std::uint8_t> data, std::size_t offset) {
  if (data.size() < offset + 2) {
    return false;
  }
  return data[offset] == 0xFF && (data[offset + 1] & 0xF6) == 0xF0;
}

bool LooksLikeMpegTs(std::span<const std::uint8_t> data) {
  constexpr std::size_t kPacket = 188;
  if (data.size() < kPacket * 2 + 1) {
    return false;
  }
  for (std::size_t start = 0; start < kPacket && start < data.size(); ++start) {
    if (start + kPacket * 2 < data.size() && data[start] == 0x47 &&
        data[start + kPacket] == 0x47 && data[start + kPacket * 2] == 0x47) {
      return true;
    }
  }
  return false;
}

bool LooksText(std::span<const std::uint8_t> data) {
  const std::size_t limit = std::min<std::size_t>(data.size(), 512);
  if (limit == 0) {
    return false;
  }
  std::size_t printable = 0;
  for (std::size_t i = 0; i < limit; ++i) {
    const auto b = data[i];
    if (b == '\n' || b == '\r' || b == '\t' || (b >= 0x20 && b <= 0x7E)) {
      ++printable;
    }
  }
  return printable > limit * 9 / 10;
}

std::string HeadText(std::span<const std::uint8_t> data) {
  const std::size_t limit = std::min<std::size_t>(data.size(), 2048);
  std::string out;
  out.reserve(limit);
  for (std::size_t i = 0; i < limit; ++i) {
    out.push_back(static_cast<char>(data[i]));
  }
  return out;
}

std::string JsonEscape(std::string_view value) {
  std::ostringstream out;
  for (unsigned char c : value) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(c);
        } else {
          out << c;
        }
    }
  }
  return out.str();
}

FormatDetection MakeDetection(std::string format, FormatFamily family,
                              double confidence,
                              std::vector<std::string> evidence) {
  return FormatDetection{
      .format = std::move(format),
      .family = family,
      .confidence = confidence,
      .evidence = std::move(evidence),
  };
}

}  // namespace

FormatDetection DetectFormat(std::span<const std::uint8_t> data,
                             const std::string& name_hint) {
  const auto lower_name = Lower(name_hint);

  if (data.size() >= 12 && HasBytes(data, 4, "ftyp")) {
    std::string brand;
    if (data.size() >= 12) {
      brand.assign(reinterpret_cast<const char*>(&data[8]), 4);
    }
    return MakeDetection("iso-bmff", FormatFamily::Container, 0.99,
                         {"found ftyp box at offset 4", "major_brand=" + brand});
  }

  if (HasBytesAtZero(data, "\x1A\x45\xDF\xA3")) {
    return MakeDetection("matroska/webm", FormatFamily::Container, 0.96,
                         {"found EBML header magic 1A45DFA3"});
  }

  if (LooksLikeMpegTs(data)) {
    return MakeDetection("mpeg-ts", FormatFamily::Container, 0.94,
                         {"found repeated 0x47 sync bytes at 188-byte intervals"});
  }

  if (HasBytesAtZero(data, "RIFF") && HasBytes(data, 8, "WAVE")) {
    return MakeDetection("wav", FormatFamily::Container, 0.98,
                         {"found RIFF header", "found WAVE form type"});
  }

  if (HasBytesAtZero(data, "RF64") && HasBytes(data, 8, "WAVE")) {
    return MakeDetection("rf64", FormatFamily::Container, 0.98,
                         {"found RF64 header", "found WAVE form type"});
  }

  if (HasBytesAtZero(data, "RIFF") && HasBytes(data, 8, "AVI ")) {
    return MakeDetection("avi", FormatFamily::Container, 0.96,
                         {"found RIFF header", "found AVI form type"});
  }

  if (HasBytesAtZero(data, "OggS")) {
    return MakeDetection("ogg", FormatFamily::Container, 0.98,
                         {"found Ogg page capture pattern"});
  }

  if (HasBytesAtZero(data, "fLaC")) {
    return MakeDetection("flac", FormatFamily::Container, 0.98,
                         {"found native FLAC marker"});
  }

  if (HasBytesAtZero(data, "FLV")) {
    return MakeDetection("flv", FormatFamily::Container, 0.98,
                         {"found FLV signature"});
  }

  if (HasBytesAtZero(data, "ID3")) {
    return MakeDetection("mp3", FormatFamily::ElementaryStream, 0.86,
                         {"found ID3 tag at start"});
  }

  if (LooksLikeMp3Frame(data, 0)) {
    return MakeDetection("mp3", FormatFamily::ElementaryStream, 0.82,
                         {"found MPEG audio frame sync at start"});
  }

  if (LooksLikeAdts(data, 0)) {
    return MakeDetection("adts-aac", FormatFamily::ElementaryStream, 0.85,
                         {"found ADTS syncword at start"});
  }

  if (HasBytes(data, 0, std::string_view("\x00\x00\x00\x01", 4)) ||
      HasBytes(data, 0, std::string_view("\x00\x00\x01", 3))) {
    return MakeDetection("annex-b-bitstream", FormatFamily::ElementaryStream, 0.72,
                         {"found Annex B start code at start"});
  }

  if (LooksText(data)) {
    const auto text = HeadText(data);
    if (text.find("#EXTM3U") != std::string::npos) {
      return MakeDetection("hls-m3u8", FormatFamily::Manifest, 0.98,
                           {"found #EXTM3U manifest marker"});
    }
    if (text.find("<MPD") != std::string::npos ||
        text.find("urn:mpeg:dash:schema:mpd") != std::string::npos) {
      return MakeDetection("dash-mpd", FormatFamily::Manifest, 0.96,
                           {"found DASH MPD marker"});
    }
  }

  if (EndsWith(lower_name, ".m3u8")) {
    return MakeDetection("hls-m3u8", FormatFamily::Manifest, 0.55,
                         {"file extension hint .m3u8"});
  }
  if (EndsWith(lower_name, ".mpd")) {
    return MakeDetection("dash-mpd", FormatFamily::Manifest, 0.55,
                         {"file extension hint .mpd"});
  }

  if (data.size() >= 8) {
    const auto box_size = ReadBe32(data, 0);
    if (box_size >= 8 && box_size <= data.size()) {
      return MakeDetection("unknown-box-container", FormatFamily::Container, 0.35,
                           {"first 4 bytes look like a bounded big-endian box size"});
    }
  }

  return MakeDetection("unknown", FormatFamily::Unknown, 0.0,
                       {"no known signature matched"});
}

std::string FormatFamilyToString(FormatFamily family) {
  switch (family) {
    case FormatFamily::Container:
      return "container";
    case FormatFamily::Manifest:
      return "manifest";
    case FormatFamily::ElementaryStream:
      return "elementary_stream";
    case FormatFamily::Unknown:
      return "unknown";
  }
  return "unknown";
}

std::string DetectionToJson(const FormatDetection& detection) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"format\": \"" << JsonEscape(detection.format) << "\",\n";
  out << "  \"family\": \"" << FormatFamilyToString(detection.family) << "\",\n";
  out << "  \"confidence\": " << std::fixed << std::setprecision(2)
      << detection.confidence << ",\n";
  out << "  \"evidence\": [";
  for (std::size_t i = 0; i < detection.evidence.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }
    out << "\"" << JsonEscape(detection.evidence[i]) << "\"";
  }
  out << "]\n";
  out << "}\n";
  return out.str();
}

}  // namespace media_analyzer
