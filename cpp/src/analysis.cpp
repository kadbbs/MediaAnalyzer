#include "media_analyzer/analysis.h"

#include "media_analyzer/format_detector.h"
#include "media_analyzer/iso_bmff.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace media_analyzer {
namespace {

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

std::string BytesToHex(const std::vector<std::uint8_t>& data, std::size_t length) {
  std::ostringstream out;
  const auto limit = std::min(data.size(), length);
  for (std::size_t i = 0; i < limit; ++i) {
    if (i != 0) {
      out << ' ';
    }
    out << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(data[i]);
  }
  return out.str();
}

std::string BytesToAscii(const std::vector<std::uint8_t>& data, std::size_t length) {
  const auto limit = std::min(data.size(), length);
  std::string out;
  out.reserve(limit);
  for (std::size_t i = 0; i < limit; ++i) {
    const auto byte = data[i];
    out.push_back(byte >= 0x20 && byte <= 0x7e ? static_cast<char>(byte) : '.');
  }
  return out;
}

std::string HexByte(std::uint8_t value) {
  std::ostringstream out;
  out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value);
  return out.str();
}

std::uint32_t AacSampleRateFromIndex(std::uint32_t index) {
  static constexpr std::uint32_t kRates[] = {
      96000, 88200, 64000, 48000, 44100, 32000, 24000,
      22050, 16000, 12000, 11025, 8000, 7350};
  return index < (sizeof(kRates) / sizeof(kRates[0])) ? kRates[index] : 0;
}

std::string AnalyzeAdtsToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"ADTS AAC\",\n";
  out << std::string(indent + 2, ' ') << "\"frames\": [\n";
  std::size_t offset = 0;
  std::size_t count = 0;
  while (offset + 7 <= data.size() && count < 64) {
    if (data[offset] != 0xff || (data[offset + 1] & 0xf0) != 0xf0) {
      break;
    }
    const bool protection_absent = (data[offset + 1] & 0x01) != 0;
    const auto profile = static_cast<std::uint32_t>(((data[offset + 2] >> 6) & 0x03) + 1);
    const auto sample_rate_index = static_cast<std::uint32_t>((data[offset + 2] >> 2) & 0x0f);
    const auto channel_config = static_cast<std::uint32_t>(((data[offset + 2] & 0x01) << 2) |
                                                           ((data[offset + 3] >> 6) & 0x03));
    const auto frame_length = static_cast<std::uint32_t>(((data[offset + 3] & 0x03) << 11) |
                                                        (data[offset + 4] << 3) |
                                                        ((data[offset + 5] >> 5) & 0x07));
    if (frame_length < (protection_absent ? 7u : 9u) || offset + frame_length > data.size()) {
      break;
    }
    if (count != 0) {
      out << ",\n";
    }
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (count + 1)
        << ", \"offset\": " << offset
        << ", \"length\": " << frame_length
        << ", \"profile\": " << profile
        << ", \"sample_rate\": " << AacSampleRateFromIndex(sample_rate_index)
        << ", \"channel_config\": " << channel_config
        << ", \"header_length\": " << (protection_absent ? 7 : 9)
        << "}";
    offset += frame_length;
    ++count;
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"frame_count\": " << count << "\n";
  out << std::string(indent, ' ') << "}";
  return out.str();
}

std::size_t FindStartCode(const std::vector<std::uint8_t>& data, std::size_t start) {
  for (std::size_t i = start; i + 3 <= data.size(); ++i) {
    if (i + 3 <= data.size() && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
      return i;
    }
    if (i + 4 <= data.size() && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0 && data[i + 3] == 1) {
      return i;
    }
  }
  return data.size();
}

std::string AnalyzeAnnexBToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"Annex B H.264\",\n";
  out << std::string(indent + 2, ' ') << "\"nal_units\": [\n";
  std::size_t start_code = FindStartCode(data, 0);
  std::size_t count = 0;
  while (start_code < data.size() && count < 128) {
    const std::size_t prefix = (start_code + 4 <= data.size() && data[start_code + 2] == 0) ? 4 : 3;
    const std::size_t nal_start = start_code + prefix;
    const std::size_t next = FindStartCode(data, nal_start);
    if (nal_start >= data.size()) {
      break;
    }
    const auto nal_type = data[nal_start] & 0x1f;
    if (count != 0) {
      out << ",\n";
    }
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (count + 1)
        << ", \"offset\": " << nal_start
        << ", \"length\": " << (next - nal_start)
        << ", \"start_code_length\": " << prefix
        << ", \"nal_type\": " << static_cast<int>(nal_type)
        << "}";
    ++count;
    start_code = next;
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"nal_unit_count\": " << count << "\n";
  out << std::string(indent, ' ') << "}";
  return out.str();
}

std::string AnalyzeMpegTsToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"MPEG-TS\",\n";
  out << std::string(indent + 2, ' ') << "\"packet_size\": 188,\n";
  out << std::string(indent + 2, ' ') << "\"packets\": [\n";
  const auto packet_count = data.size() / 188;
  const auto limit = std::min<std::size_t>(packet_count, 128);
  for (std::size_t i = 0; i < limit; ++i) {
    const auto offset = i * 188;
    if (i != 0) {
      out << ",\n";
    }
    const auto pid = static_cast<std::uint16_t>(((data[offset + 1] & 0x1f) << 8) | data[offset + 2]);
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (i + 1)
        << ", \"offset\": " << offset
        << ", \"length\": 188"
        << ", \"sync\": " << (data[offset] == 0x47 ? "true" : "false")
        << ", \"payload_unit_start\": " << (((data[offset + 1] & 0x40) != 0) ? "true" : "false")
        << ", \"pid\": " << pid
        << ", \"adaptation_field_control\": " << static_cast<int>((data[offset + 3] >> 4) & 0x03)
        << ", \"continuity_counter\": " << static_cast<int>(data[offset + 3] & 0x0f)
        << "}";
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"packet_count\": " << packet_count << "\n";
  out << std::string(indent, ' ') << "}";
  return out.str();
}

bool ReadEbmlVint(const std::vector<std::uint8_t>& data, std::size_t& offset,
                  std::uint64_t& value, std::size_t& width, bool keep_marker) {
  if (offset >= data.size()) {
    return false;
  }
  const auto first = data[offset];
  std::uint8_t mask = 0x80;
  width = 1;
  while (width <= 8 && (first & mask) == 0) {
    mask >>= 1;
    ++width;
  }
  if (width > 8 || offset + width > data.size()) {
    return false;
  }
  value = keep_marker ? first : (first & ~mask);
  for (std::size_t i = 1; i < width; ++i) {
    value = (value << 8) | data[offset + i];
  }
  offset += width;
  return true;
}

std::string AnalyzeEbmlToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"Matroska/WebM\",\n";
  out << std::string(indent + 2, ' ') << "\"elements\": [\n";
  std::size_t offset = 0;
  std::size_t count = 0;
  while (offset < data.size() && count < 64) {
    const auto element_offset = offset;
    std::uint64_t id = 0;
    std::uint64_t size = 0;
    std::size_t id_width = 0;
    std::size_t size_width = 0;
    if (!ReadEbmlVint(data, offset, id, id_width, true) ||
        !ReadEbmlVint(data, offset, size, size_width, false)) {
      break;
    }
    if (count != 0) {
      out << ",\n";
    }
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (count + 1)
        << ", \"offset\": " << element_offset
        << ", \"header_size\": " << (id_width + size_width)
        << ", \"id\": \"" << JsonEscape(HexByte(static_cast<std::uint8_t>((id >> ((id_width - 1) * 8)) & 0xff))) << "\""
        << ", \"id_value\": " << id
        << ", \"size\": " << size
        << "}";
    offset += static_cast<std::size_t>(std::min<std::uint64_t>(size, data.size() - offset));
    ++count;
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"element_count\": " << count << "\n";
  out << std::string(indent, ' ') << "}";
  return out.str();
}

}  // namespace

std::string AnalyzeFileToJson(const std::vector<std::uint8_t>& data,
                              const AnalyzeOptions& options) {
  const auto detection = DetectFormat(data, options.name_hint);

  std::ostringstream out;
  out << "{\n";
  out << "  \"input\": {\n";
  out << "    \"name\": \"" << JsonEscape(options.name_hint) << "\",\n";
  out << "    \"size\": " << data.size() << ",\n";
  constexpr std::size_t kInputPreviewBytes = 16 * 1024 * 1024;
  const auto preview_length = std::min(data.size(), kInputPreviewBytes);
  out << "    \"bytes\": {\n";
  out << "      \"offset\": 0,\n";
  out << "      \"length\": " << preview_length << ",\n";
  out << "      \"truncated\": " << (data.size() > preview_length ? "true" : "false") << ",\n";
  out << "      \"hex\": \"" << JsonEscape(BytesToHex(data, preview_length)) << "\",\n";
  out << "      \"ascii\": \"" << JsonEscape(BytesToAscii(data, preview_length)) << "\"\n";
  out << "    }\n";
  out << "  },\n";

  std::string detection_json = DetectionToJson(detection);
  if (!detection_json.empty() && detection_json.back() == '\n') {
    detection_json.pop_back();
  }
  out << "  \"detection\": ";
  for (std::size_t i = 0; i < detection_json.size(); ++i) {
    out << detection_json[i];
    if (detection_json[i] == '\n') {
      out << "  ";
    }
  }

  if (detection.format == "iso-bmff") {
    out << ",\n";
    auto analysis = ParseIsoBmff(data);
    if (analysis.has_value()) {
      out << "  \"container\": " << IsoBmffAnalysisToJson(*analysis, 2) << "\n";
    } else {
      out << "  \"container\": null\n";
    }
  } else if (detection.format == "mpeg-ts") {
    out << ",\n";
    out << "  \"container\": " << AnalyzeMpegTsToJson(data, 2) << "\n";
  } else if (detection.format == "matroska/webm") {
    out << ",\n";
    out << "  \"container\": " << AnalyzeEbmlToJson(data, 2) << "\n";
  } else if (detection.format == "adts-aac") {
    out << ",\n";
    out << "  \"bitstream\": " << AnalyzeAdtsToJson(data, 2) << "\n";
  } else if (detection.format == "annex-b-bitstream") {
    out << ",\n";
    out << "  \"bitstream\": " << AnalyzeAnnexBToJson(data, 2) << "\n";
  } else {
    out << "\n";
  }

  out << "}\n";
  return out.str();
}

}  // namespace media_analyzer
