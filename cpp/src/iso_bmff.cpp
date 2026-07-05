#include "media_analyzer/iso_bmff.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>

namespace media_analyzer {
namespace {

struct BoxHeader {
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
  std::uint64_t header_size = 0;
  std::string type;
  bool valid = false;
};

constexpr std::uint64_t kBoxPreviewBytes = 160;
constexpr std::uint64_t kCodecPreviewBytes = 256;
constexpr std::uint32_t kSampleOutputLimit = 5000;

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

std::string Indent(int count) {
  return std::string(static_cast<std::size_t>(count), ' ');
}

std::string BytesToHex(std::span<const std::uint8_t> bytes) {
  std::ostringstream out;
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    if (i != 0) {
      out << ' ';
    }
    out << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(bytes[i]);
  }
  return out.str();
}

std::string BytesToAscii(std::span<const std::uint8_t> bytes) {
  std::string out;
  out.reserve(bytes.size());
  for (auto byte : bytes) {
    out.push_back(byte >= 0x20 && byte <= 0x7e ? static_cast<char>(byte) : '.');
  }
  return out;
}

bool CanRead(std::span<const std::uint8_t> data, std::uint64_t offset,
             std::uint64_t length) {
  return offset <= data.size() && length <= data.size() - offset;
}

std::span<const std::uint8_t> ByteSpan(std::span<const std::uint8_t> data,
                                       std::uint64_t offset,
                                       std::uint64_t length) {
  if (!CanRead(data, offset, length)) {
    return {};
  }
  return std::span<const std::uint8_t>(&data[static_cast<std::size_t>(offset)],
                                       static_cast<std::size_t>(length));
}

std::uint8_t ReadU8(std::span<const std::uint8_t> data, std::uint64_t offset) {
  return CanRead(data, offset, 1) ? data[static_cast<std::size_t>(offset)] : 0;
}

std::uint16_t ReadBe16(std::span<const std::uint8_t> data, std::uint64_t offset) {
  if (!CanRead(data, offset, 2)) {
    return 0;
  }
  return static_cast<std::uint16_t>((data[static_cast<std::size_t>(offset)] << 8) |
                                    data[static_cast<std::size_t>(offset + 1)]);
}

std::uint32_t ReadBe32(std::span<const std::uint8_t> data, std::uint64_t offset) {
  if (!CanRead(data, offset, 4)) {
    return 0;
  }
  return (static_cast<std::uint32_t>(data[static_cast<std::size_t>(offset)]) << 24) |
         (static_cast<std::uint32_t>(data[static_cast<std::size_t>(offset + 1)]) << 16) |
         (static_cast<std::uint32_t>(data[static_cast<std::size_t>(offset + 2)]) << 8) |
         static_cast<std::uint32_t>(data[static_cast<std::size_t>(offset + 3)]);
}

std::uint64_t ReadBe64(std::span<const std::uint8_t> data, std::uint64_t offset) {
  return (static_cast<std::uint64_t>(ReadBe32(data, offset)) << 32) |
         ReadBe32(data, offset + 4);
}

std::string ReadFourcc(std::span<const std::uint8_t> data, std::uint64_t offset) {
  if (!CanRead(data, offset, 4)) {
    return "";
  }
  return std::string(reinterpret_cast<const char*>(&data[static_cast<std::size_t>(offset)]), 4);
}

double Seconds(const RationalTime& time) {
  if (time.timescale == 0) {
    return 0.0;
  }
  return static_cast<double>(time.value) / static_cast<double>(time.timescale);
}

BoxHeader ReadBoxHeader(std::span<const std::uint8_t> data, std::uint64_t offset,
                        std::uint64_t end) {
  if (!CanRead(data, offset, 8) || offset + 8 > end) {
    return {};
  }

  BoxHeader header;
  header.offset = offset;
  header.size = ReadBe32(data, offset);
  header.header_size = 8;
  header.type = ReadFourcc(data, offset + 4);

  if (header.size == 1) {
    if (!CanRead(data, offset + 8, 8)) {
      return {};
    }
    header.size = ReadBe64(data, offset + 8);
    header.header_size = 16;
  } else if (header.size == 0) {
    header.size = end - offset;
  }

  if (header.type == "uuid") {
    header.header_size += 16;
  }

  if (header.size < header.header_size || offset + header.size > end) {
    return {};
  }
  header.valid = true;
  return header;
}

bool IsContainerBox(const std::string& type) {
  static constexpr std::array<std::string_view, 23> kContainers = {
      "moov", "trak", "mdia", "minf", "dinf", "stbl", "edts", "udta",
      "meta", "ilst", "moof", "traf", "mvex", "mfra", "tref", "ipro",
      "sinf", "schi", "wave", "skip", "strk", "strd", "encv"};
  return std::find(kContainers.begin(), kContainers.end(), type) != kContainers.end();
}

bool IsVisualSampleEntry(const std::string& type) {
  static constexpr std::array<std::string_view, 10> kTypes = {
      "avc1", "avc3", "hvc1", "hev1", "av01", "vp08", "vp09", "mp4v", "jpeg", "png "};
  return std::find(kTypes.begin(), kTypes.end(), type) != kTypes.end();
}

bool IsAudioSampleEntry(const std::string& type) {
  static constexpr std::array<std::string_view, 10> kTypes = {
      "mp4a", "Opus", "fLaC", "alac", "ac-3", "ec-3", "enca", "sowt", "twos", "lpcm"};
  return std::find(kTypes.begin(), kTypes.end(), type) != kTypes.end();
}

std::string CodecDescription(const std::string& fourcc) {
  if (fourcc == "avc1" || fourcc == "avc3") {
    return "H.264/AVC";
  }
  if (fourcc == "hvc1" || fourcc == "hev1") {
    return "H.265/HEVC";
  }
  if (fourcc == "av01") {
    return "AV1";
  }
  if (fourcc == "mp4a") {
    return "MPEG-4 Audio";
  }
  if (fourcc == "Opus") {
    return "Opus";
  }
  if (fourcc == "vp09") {
    return "VP9";
  }
  if (fourcc == "vp08") {
    return "VP8";
  }
  return fourcc;
}

std::vector<StructureNode> ParseBoxNodes(std::span<const std::uint8_t> data,
                                         std::uint64_t start, std::uint64_t end,
                                         int depth);

StructureNode ParseBoxNode(std::span<const std::uint8_t> data, const BoxHeader& header,
                           int depth) {
  StructureNode node;
  node.type = header.type;
  node.offset = header.offset;
  node.size = header.size;
  node.header_size = header.header_size;
  node.preview_offset = header.offset;
  node.preview_length = std::min<std::uint64_t>(header.size, kBoxPreviewBytes);
  node.preview_truncated = header.size > node.preview_length;
  const auto preview = ByteSpan(data, node.preview_offset, node.preview_length);
  node.hex_preview = BytesToHex(preview);
  node.ascii_preview = BytesToAscii(preview);

  if (depth <= 0) {
    return node;
  }

  const std::uint64_t payload = header.offset + header.header_size;
  const std::uint64_t box_end = header.offset + header.size;

  if (header.type == "stsd" && CanRead(data, payload, 8)) {
    node.children = ParseBoxNodes(data, payload + 8, box_end, depth - 1);
    return node;
  }

  if (IsVisualSampleEntry(header.type) && CanRead(data, payload, 78)) {
    node.children = ParseBoxNodes(data, payload + 78, box_end, depth - 1);
    return node;
  }

  if (IsAudioSampleEntry(header.type) && CanRead(data, payload, 28)) {
    node.children = ParseBoxNodes(data, payload + 28, box_end, depth - 1);
    return node;
  }

  if (header.type == "meta" && CanRead(data, payload, 4)) {
    node.children = ParseBoxNodes(data, payload + 4, box_end, depth - 1);
    return node;
  }

  if (IsContainerBox(header.type)) {
    node.children = ParseBoxNodes(data, payload, box_end, depth - 1);
  }
  return node;
}

std::vector<StructureNode> ParseBoxNodes(std::span<const std::uint8_t> data,
                                         std::uint64_t start, std::uint64_t end,
                                         int depth) {
  std::vector<StructureNode> nodes;
  std::uint64_t offset = start;
  while (offset + 8 <= end) {
    const auto header = ReadBoxHeader(data, offset, end);
    if (!header.valid) {
      break;
    }
    nodes.push_back(ParseBoxNode(data, header, depth));
    offset += header.size;
  }
  return nodes;
}

const StructureNode* FindChild(const StructureNode& node, const std::string& type) {
  for (const auto& child : node.children) {
    if (child.type == type) {
      return &child;
    }
  }
  return nullptr;
}

const StructureNode* FindTop(const std::vector<StructureNode>& nodes, const std::string& type) {
  for (const auto& node : nodes) {
    if (node.type == type) {
      return &node;
    }
  }
  return nullptr;
}

std::vector<const StructureNode*> FindChildren(const StructureNode& node,
                                               const std::string& type) {
  std::vector<const StructureNode*> out;
  for (const auto& child : node.children) {
    if (child.type == type) {
      out.push_back(&child);
    }
  }
  return out;
}

struct BitReader {
  std::span<const std::uint8_t> bytes;
  std::size_t bit_offset = 0;

  std::uint32_t ReadBits(std::size_t count) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < count; ++i) {
      value <<= 1;
      if (bit_offset / 8 < bytes.size()) {
        const auto current = bytes[bit_offset / 8];
        value |= (current >> (7 - (bit_offset % 8))) & 1;
      }
      ++bit_offset;
    }
    return value;
  }

  std::uint32_t ReadUe() {
    std::size_t zeros = 0;
    while (bit_offset < bytes.size() * 8 && ReadBits(1) == 0) {
      ++zeros;
    }
    if (zeros == 0) {
      return 0;
    }
    return ((1u << zeros) - 1u) + ReadBits(zeros);
  }

  std::int32_t ReadSe() {
    const auto code_num = ReadUe();
    const auto value = static_cast<std::int32_t>((code_num + 1) / 2);
    return (code_num % 2 == 0) ? -value : value;
  }
};

std::vector<std::uint8_t> ToRbsp(std::span<const std::uint8_t> nal) {
  std::vector<std::uint8_t> rbsp;
  rbsp.reserve(nal.size());
  for (std::size_t i = 0; i < nal.size(); ++i) {
    if (i + 2 < nal.size() && nal[i] == 0x00 && nal[i + 1] == 0x00 && nal[i + 2] == 0x03) {
      rbsp.push_back(0x00);
      rbsp.push_back(0x00);
      i += 2;
      continue;
    }
    rbsp.push_back(nal[i]);
  }
  return rbsp;
}

void SkipScalingList(BitReader& bits, int size) {
  int last_scale = 8;
  int next_scale = 8;
  for (int j = 0; j < size; ++j) {
    if (next_scale != 0) {
      const auto delta_scale = bits.ReadSe();
      next_scale = (last_scale + delta_scale + 256) % 256;
    }
    last_scale = (next_scale == 0) ? last_scale : next_scale;
  }
}

std::string H264ProfileName(std::uint32_t profile_idc) {
  switch (profile_idc) {
    case 66:
      return "Baseline";
    case 77:
      return "Main";
    case 88:
      return "Extended";
    case 100:
      return "High";
    case 110:
      return "High 10";
    case 122:
      return "High 4:2:2";
    case 244:
      return "High 4:4:4 Predictive";
    default:
      return "profile_idc " + std::to_string(profile_idc);
  }
}

std::string HevcProfileName(std::uint32_t profile_idc) {
  switch (profile_idc) {
    case 1:
      return "Main";
    case 2:
      return "Main 10";
    case 3:
      return "Main Still Picture";
    case 4:
      return "Range Extensions";
    default:
      return "profile_idc " + std::to_string(profile_idc);
  }
}

std::string HevcLevelName(std::uint32_t level_idc) {
  return std::to_string(level_idc / 30) + "." +
         std::to_string((level_idc % 30) / 3);
}

std::string AacObjectTypeName(std::uint32_t object_type) {
  switch (object_type) {
    case 1:
      return "AAC Main";
    case 2:
      return "AAC LC";
    case 3:
      return "AAC SSR";
    case 4:
      return "AAC LTP";
    case 5:
      return "HE-AAC SBR";
    case 29:
      return "HE-AAC v2 PS";
    default:
      return "AAC object type " + std::to_string(object_type);
  }
}

std::uint32_t AacSampleRateFromIndex(std::uint32_t index) {
  static constexpr std::array<std::uint32_t, 13> kRates = {
      96000, 88200, 64000, 48000, 44100, 32000, 24000,
      22050, 16000, 12000, 11025, 8000, 7350};
  if (index < kRates.size()) {
    return kRates[index];
  }
  return 0;
}

std::uint32_t ReadAacObjectType(BitReader& bits) {
  auto object_type = bits.ReadBits(5);
  if (object_type == 31) {
    object_type = 32 + bits.ReadBits(6);
  }
  return object_type;
}

std::optional<CodecInfo> ParseAudioSpecificConfig(std::span<const std::uint8_t> asc,
                                                  const std::string& fourcc) {
  if (asc.empty()) {
    return std::nullopt;
  }
  CodecInfo codec;
  codec.fourcc = fourcc;
  codec.description = "AAC";
  codec.raw_header_hex = BytesToHex(asc.subspan(0, std::min<std::size_t>(asc.size(), kCodecPreviewBytes)));
  codec.asc_hex = codec.raw_header_hex;

  BitReader bits{asc};
  const auto object_type = ReadAacObjectType(bits);
  const auto sample_rate_index = bits.ReadBits(4);
  std::uint32_t sample_rate = 0;
  if (sample_rate_index == 0x0f) {
    sample_rate = bits.ReadBits(24);
  } else {
    sample_rate = AacSampleRateFromIndex(sample_rate_index);
  }
  const auto channel_config = bits.ReadBits(4);

  codec.audio_object_type = object_type;
  codec.profile = AacObjectTypeName(object_type);
  if (sample_rate != 0) {
    codec.asc_sample_rate = sample_rate;
  }
  codec.channel_config = channel_config;
  return codec;
}

std::optional<std::uint64_t> ReadDescriptorSize(std::span<const std::uint8_t> data,
                                                std::uint64_t& offset,
                                                std::uint64_t end) {
  std::uint64_t size = 0;
  for (int i = 0; i < 4; ++i) {
    if (!CanRead(data, offset, 1) || offset >= end) {
      return std::nullopt;
    }
    const auto byte = ReadU8(data, offset++);
    size = (size << 7) | (byte & 0x7f);
    if ((byte & 0x80) == 0) {
      return size;
    }
  }
  return size;
}

std::optional<CodecInfo> ParseEsdsAac(std::span<const std::uint8_t> data,
                                      std::uint64_t payload,
                                      std::uint64_t size,
                                      const std::string& fourcc) {
  if (!CanRead(data, payload, size) || size < 4) {
    return std::nullopt;
  }
  const auto end = payload + size;
  for (std::uint64_t candidate = payload + 4; candidate + 2 <= end; ++candidate) {
    if (ReadU8(data, candidate) != 0x05) {
      continue;
    }
    std::uint64_t offset = candidate + 1;
    auto desc_size = ReadDescriptorSize(data, offset, end);
    if (desc_size && *desc_size > 0 && offset + *desc_size <= end) {
      auto asc = ByteSpan(data, offset, *desc_size);
      auto codec = ParseAudioSpecificConfig(asc, fourcc);
      if (codec.has_value()) {
        codec->raw_header_hex =
            BytesToHex(ByteSpan(data, payload, std::min<std::uint64_t>(size, kCodecPreviewBytes)));
        codec->asc_bytes = ByteRange{offset, *desc_size};
      }
      return codec;
    }
  }
  return std::nullopt;
}

std::optional<CodecInfo> ParseHvcC(std::span<const std::uint8_t> data,
                                   std::uint64_t payload,
                                   std::uint64_t size,
                                   const std::string& fourcc) {
  if (!CanRead(data, payload, size) || size < 23) {
    return std::nullopt;
  }

  CodecInfo codec;
  codec.fourcc = fourcc;
  codec.description = CodecDescription(fourcc);
  codec.raw_header_hex =
      BytesToHex(ByteSpan(data, payload, std::min<std::uint64_t>(size, kCodecPreviewBytes)));

  const auto profile_byte = ReadU8(data, payload + 1);
  const auto profile_space = (profile_byte >> 6) & 0x03;
  const auto tier_flag = (profile_byte >> 5) & 0x01;
  const auto profile_idc = profile_byte & 0x1f;
  const auto level_idc = ReadU8(data, payload + 12);
  codec.profile = HevcProfileName(profile_idc);
  if (profile_space != 0) {
    codec.profile += " profile_space " + std::to_string(profile_space);
  }
  codec.level = (tier_flag ? "High tier " : "Main tier ") + HevcLevelName(level_idc);
  codec.chroma_format = ReadU8(data, payload + 16) & 0x03;
  codec.bit_depth_luma = (ReadU8(data, payload + 17) & 0x07) + 8;
  codec.bit_depth_chroma = (ReadU8(data, payload + 18) & 0x07) + 8;
  codec.length_size = (ReadU8(data, payload + 21) & 0x03) + 1;

  std::uint64_t offset = payload + 23;
  const auto array_count = ReadU8(data, payload + 22);
  std::uint32_t vps_count = 0;
  std::uint32_t sps_count = 0;
  std::uint32_t pps_count = 0;
  for (std::uint32_t array_index = 0; array_index < array_count && offset + 3 <= payload + size;
       ++array_index) {
    const auto nal_type = ReadU8(data, offset++) & 0x3f;
    const auto nal_count = ReadBe16(data, offset);
    offset += 2;
    for (std::uint32_t i = 0; i < nal_count && offset + 2 <= payload + size; ++i) {
      const auto nal_size = ReadBe16(data, offset);
      offset += 2;
      if (!CanRead(data, offset, nal_size) || offset + nal_size > payload + size) {
        return codec;
      }
      const auto nal_hex = BytesToHex(ByteSpan(data, offset, nal_size));
      if (nal_type == 32) {
        ++vps_count;
        if (codec.vps_hex.empty()) {
          codec.vps_hex = nal_hex;
          codec.vps_bytes = ByteRange{offset, nal_size};
        }
      } else if (nal_type == 33) {
        ++sps_count;
        if (codec.sps_hex.empty()) {
          codec.sps_hex = nal_hex;
          codec.sps_bytes = ByteRange{offset, nal_size};
        }
      } else if (nal_type == 34) {
        ++pps_count;
        if (codec.pps_hex.empty()) {
          codec.pps_hex = nal_hex;
          codec.pps_bytes = ByteRange{offset, nal_size};
        }
      }
      offset += nal_size;
    }
  }
  codec.vps_count = vps_count;
  codec.sps_count = sps_count;
  codec.pps_count = pps_count;
  return codec;
}

std::optional<CodecInfo> ParseAvcC(std::span<const std::uint8_t> data,
                                   std::uint64_t payload,
                                   std::uint64_t size,
                                   const std::string& fourcc) {
  if (!CanRead(data, payload, size) || size < 7) {
    return std::nullopt;
  }

  CodecInfo codec;
  codec.fourcc = fourcc;
  codec.description = CodecDescription(fourcc);
  codec.raw_header_hex =
      BytesToHex(ByteSpan(data, payload, std::min<std::uint64_t>(size, kCodecPreviewBytes)));
  const auto profile_idc = ReadU8(data, payload + 1);
  const auto level_idc = ReadU8(data, payload + 3);
  codec.profile = H264ProfileName(profile_idc);
  codec.level = std::to_string(level_idc / 10) + "." + std::to_string(level_idc % 10);
  codec.length_size = (ReadU8(data, payload + 4) & 0x03) + 1;

  std::uint64_t offset = payload + 5;
  const auto sps_count = ReadU8(data, offset) & 0x1f;
  codec.sps_count = sps_count;
  ++offset;

  if (sps_count > 0 && CanRead(data, offset, 2)) {
    const auto sps_size = ReadBe16(data, offset);
    offset += 2;
    if (CanRead(data, offset, sps_size) && sps_size > 1) {
      std::span<const std::uint8_t> nal(&data[static_cast<std::size_t>(offset)],
                                        static_cast<std::size_t>(sps_size));
      codec.sps_hex = BytesToHex(nal);
      codec.sps_bytes = ByteRange{offset, sps_size};
      if ((nal[0] & 0x1f) == 7) {
        nal = nal.subspan(1);
      }
      auto rbsp = ToRbsp(nal);
      BitReader bits{rbsp};
      const auto sps_profile_idc = bits.ReadBits(8);
      bits.ReadBits(8);
      const auto sps_level_idc = bits.ReadBits(8);
      bits.ReadUe();

      std::uint32_t chroma_format_idc = 1;
      bool separate_colour_plane_flag = false;
      const bool high_profile =
          sps_profile_idc == 100 || sps_profile_idc == 110 || sps_profile_idc == 122 ||
          sps_profile_idc == 244 || sps_profile_idc == 44 || sps_profile_idc == 83 ||
          sps_profile_idc == 86 || sps_profile_idc == 118 || sps_profile_idc == 128 ||
          sps_profile_idc == 138 || sps_profile_idc == 144;
      if (high_profile) {
        chroma_format_idc = bits.ReadUe();
        if (chroma_format_idc == 3) {
          separate_colour_plane_flag = bits.ReadBits(1) != 0;
        }
        bits.ReadUe();
        bits.ReadUe();
        bits.ReadBits(1);
        if (bits.ReadBits(1) != 0) {
          const int count = (chroma_format_idc != 3) ? 8 : 12;
          for (int i = 0; i < count; ++i) {
            if (bits.ReadBits(1) != 0) {
              SkipScalingList(bits, i < 6 ? 16 : 64);
            }
          }
        }
      }

      bits.ReadUe();
      const auto pic_order_cnt_type = bits.ReadUe();
      if (pic_order_cnt_type == 0) {
        bits.ReadUe();
      } else if (pic_order_cnt_type == 1) {
        bits.ReadBits(1);
        bits.ReadSe();
        bits.ReadSe();
        const auto cycle = bits.ReadUe();
        for (std::uint32_t i = 0; i < cycle; ++i) {
          bits.ReadSe();
        }
      }
      bits.ReadUe();
      bits.ReadBits(1);
      const auto pic_width_in_mbs_minus1 = bits.ReadUe();
      const auto pic_height_in_map_units_minus1 = bits.ReadUe();
      const auto frame_mbs_only_flag = bits.ReadBits(1);
      if (frame_mbs_only_flag == 0) {
        bits.ReadBits(1);
      }
      bits.ReadBits(1);

      std::uint32_t crop_left = 0;
      std::uint32_t crop_right = 0;
      std::uint32_t crop_top = 0;
      std::uint32_t crop_bottom = 0;
      if (bits.ReadBits(1) != 0) {
        crop_left = bits.ReadUe();
        crop_right = bits.ReadUe();
        crop_top = bits.ReadUe();
        crop_bottom = bits.ReadUe();
      }

      std::uint32_t crop_unit_x = 1;
      std::uint32_t crop_unit_y = 2 - frame_mbs_only_flag;
      if (!separate_colour_plane_flag) {
        if (chroma_format_idc == 1) {
          crop_unit_x = 2;
          crop_unit_y = 2 * (2 - frame_mbs_only_flag);
        } else if (chroma_format_idc == 2) {
          crop_unit_x = 2;
          crop_unit_y = 2 - frame_mbs_only_flag;
        }
      }

      const auto coded_width = (pic_width_in_mbs_minus1 + 1) * 16;
      const auto coded_height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;
      codec.width = coded_width - (crop_left + crop_right) * crop_unit_x;
      codec.height = coded_height - (crop_top + crop_bottom) * crop_unit_y;
      codec.profile = H264ProfileName(sps_profile_idc);
      codec.level = std::to_string(sps_level_idc / 10) + "." + std::to_string(sps_level_idc % 10);
    }
    offset += sps_size;
  }

  for (std::uint32_t i = 1; i < static_cast<std::uint32_t>(sps_count) &&
                             CanRead(data, offset, 2);
       ++i) {
    const auto sps_size = ReadBe16(data, offset);
    offset += 2 + sps_size;
  }
  if (CanRead(data, offset, 1)) {
    codec.pps_count = ReadU8(data, offset);
    ++offset;
    if (*codec.pps_count > 0 && CanRead(data, offset, 2)) {
      const auto pps_size = ReadBe16(data, offset);
      offset += 2;
      if (CanRead(data, offset, pps_size)) {
        codec.pps_hex = BytesToHex(ByteSpan(data, offset, pps_size));
        codec.pps_bytes = ByteRange{offset, pps_size};
      }
    }
  }
  return codec;
}

void ParseFtyp(std::span<const std::uint8_t> data, const StructureNode& ftyp,
               IsoBmffAnalysis& analysis) {
  const auto payload = ftyp.offset + ftyp.header_size;
  const auto end = ftyp.offset + ftyp.size;
  if (!CanRead(data, payload, 8)) {
    return;
  }
  analysis.major_brand = ReadFourcc(data, payload);
  analysis.minor_version = ReadBe32(data, payload + 4);
  for (auto offset = payload + 8; offset + 4 <= end; offset += 4) {
    analysis.compatible_brands.push_back(ReadFourcc(data, offset));
  }
}

RationalTime ParseMvhd(std::span<const std::uint8_t> data, const StructureNode& mvhd) {
  const auto payload = mvhd.offset + mvhd.header_size;
  if (!CanRead(data, payload, 4)) {
    return {};
  }
  const auto version = ReadU8(data, payload);
  if (version == 1 && CanRead(data, payload + 20, 12)) {
    return {.value = ReadBe64(data, payload + 24), .timescale = ReadBe32(data, payload + 20)};
  }
  if (CanRead(data, payload + 12, 8)) {
    return {.value = ReadBe32(data, payload + 16), .timescale = ReadBe32(data, payload + 12)};
  }
  return {};
}

void ParseTkhd(std::span<const std::uint8_t> data, const StructureNode& tkhd, TrackInfo& track) {
  const auto payload = tkhd.offset + tkhd.header_size;
  if (!CanRead(data, payload, 4)) {
    return;
  }
  const auto version = ReadU8(data, payload);
  if (version == 1 && CanRead(data, payload + 20, 20)) {
    track.id = ReadBe32(data, payload + 20);
  } else if (CanRead(data, payload + 12, 12)) {
    track.id = ReadBe32(data, payload + 12);
  }

  const auto end = tkhd.offset + tkhd.size;
  if (CanRead(data, end - 8, 8)) {
    const auto width_fixed = ReadBe32(data, end - 8);
    const auto height_fixed = ReadBe32(data, end - 4);
    if (width_fixed != 0) {
      track.width = width_fixed >> 16;
    }
    if (height_fixed != 0) {
      track.height = height_fixed >> 16;
    }
  }
}

RationalTime ParseMdhd(std::span<const std::uint8_t> data, const StructureNode& mdhd) {
  const auto payload = mdhd.offset + mdhd.header_size;
  if (!CanRead(data, payload, 4)) {
    return {};
  }
  const auto version = ReadU8(data, payload);
  if (version == 1 && CanRead(data, payload + 20, 12)) {
    return {.value = ReadBe64(data, payload + 24), .timescale = ReadBe32(data, payload + 20)};
  }
  if (CanRead(data, payload + 12, 8)) {
    return {.value = ReadBe32(data, payload + 16), .timescale = ReadBe32(data, payload + 12)};
  }
  return {};
}

std::string ParseHdlr(std::span<const std::uint8_t> data, const StructureNode& hdlr) {
  const auto payload = hdlr.offset + hdlr.header_size;
  if (!CanRead(data, payload + 8, 4)) {
    return "unknown";
  }
  const auto handler = ReadFourcc(data, payload + 8);
  if (handler == "vide") {
    return "video";
  }
  if (handler == "soun") {
    return "audio";
  }
  if (handler == "subt" || handler == "text" || handler == "sbtl") {
    return "subtitle";
  }
  if (handler == "meta") {
    return "metadata";
  }
  return handler;
}

struct StscEntry {
  std::uint32_t first_chunk = 0;
  std::uint32_t samples_per_chunk = 0;
  std::uint32_t sample_description_index = 0;
};

std::vector<std::uint32_t> ParseSttsDurations(std::span<const std::uint8_t> data,
                                              const StructureNode* stts,
                                              std::uint32_t sample_count) {
  std::vector<std::uint32_t> durations(sample_count, 0);
  if (!stts || sample_count == 0) {
    return durations;
  }

  const auto payload = stts->offset + stts->header_size;
  if (!CanRead(data, payload + 4, 4)) {
    return durations;
  }
  const auto entry_count = ReadBe32(data, payload + 4);
  std::uint64_t offset = payload + 8;
  std::uint32_t sample_index = 0;
  for (std::uint32_t i = 0; i < entry_count && CanRead(data, offset, 8) && sample_index < sample_count; ++i) {
    const auto count = ReadBe32(data, offset);
    const auto duration = ReadBe32(data, offset + 4);
    offset += 8;
    for (std::uint32_t j = 0; j < count && sample_index < sample_count; ++j) {
      durations[sample_index++] = duration;
    }
  }
  return durations;
}

std::vector<std::int64_t> ParseCttsOffsets(std::span<const std::uint8_t> data,
                                           const StructureNode* ctts,
                                           std::uint32_t sample_count) {
  std::vector<std::int64_t> offsets(sample_count, 0);
  if (!ctts || sample_count == 0) {
    return offsets;
  }

  const auto payload = ctts->offset + ctts->header_size;
  if (!CanRead(data, payload, 8)) {
    return offsets;
  }
  const auto version = ReadU8(data, payload);
  const auto entry_count = ReadBe32(data, payload + 4);
  std::uint64_t offset = payload + 8;
  std::uint32_t sample_index = 0;
  for (std::uint32_t i = 0; i < entry_count && CanRead(data, offset, 8) && sample_index < sample_count; ++i) {
    const auto count = ReadBe32(data, offset);
    const auto raw = ReadBe32(data, offset + 4);
    const auto composition_offset = version == 1
        ? static_cast<std::int64_t>(static_cast<std::int32_t>(raw))
        : static_cast<std::int64_t>(raw);
    offset += 8;
    for (std::uint32_t j = 0; j < count && sample_index < sample_count; ++j) {
      offsets[sample_index++] = composition_offset;
    }
  }
  return offsets;
}

std::vector<std::uint32_t> ParseStszSizes(std::span<const std::uint8_t> data,
                                          const StructureNode* stsz,
                                          std::uint32_t& sample_count) {
  std::vector<std::uint32_t> sizes;
  if (!stsz) {
    return sizes;
  }

  const auto payload = stsz->offset + stsz->header_size;
  if (!CanRead(data, payload, 12)) {
    return sizes;
  }

  const auto default_size = ReadBe32(data, payload + 4);
  sample_count = ReadBe32(data, payload + 8);
  sizes.resize(sample_count, default_size);
  if (default_size != 0) {
    return sizes;
  }

  std::uint64_t offset = payload + 12;
  for (std::uint32_t i = 0; i < sample_count && CanRead(data, offset, 4); ++i) {
    sizes[i] = ReadBe32(data, offset);
    offset += 4;
  }
  return sizes;
}

std::vector<std::uint64_t> ParseChunkOffsets(std::span<const std::uint8_t> data,
                                             const StructureNode* stco,
                                             const StructureNode* co64) {
  std::vector<std::uint64_t> offsets;
  const StructureNode* box = co64 ? co64 : stco;
  if (!box) {
    return offsets;
  }

  const auto payload = box->offset + box->header_size;
  if (!CanRead(data, payload + 4, 4)) {
    return offsets;
  }
  const auto entry_count = ReadBe32(data, payload + 4);
  offsets.reserve(entry_count);
  std::uint64_t offset = payload + 8;
  for (std::uint32_t i = 0; i < entry_count; ++i) {
    if (box->type == "co64") {
      if (!CanRead(data, offset, 8)) {
        break;
      }
      offsets.push_back(ReadBe64(data, offset));
      offset += 8;
    } else {
      if (!CanRead(data, offset, 4)) {
        break;
      }
      offsets.push_back(ReadBe32(data, offset));
      offset += 4;
    }
  }
  return offsets;
}

std::vector<StscEntry> ParseStsc(std::span<const std::uint8_t> data,
                                 const StructureNode* stsc) {
  std::vector<StscEntry> entries;
  if (!stsc) {
    return entries;
  }

  const auto payload = stsc->offset + stsc->header_size;
  if (!CanRead(data, payload + 4, 4)) {
    return entries;
  }
  const auto entry_count = ReadBe32(data, payload + 4);
  entries.reserve(entry_count);
  std::uint64_t offset = payload + 8;
  for (std::uint32_t i = 0; i < entry_count && CanRead(data, offset, 12); ++i) {
    entries.push_back({
        .first_chunk = ReadBe32(data, offset),
        .samples_per_chunk = ReadBe32(data, offset + 4),
        .sample_description_index = ReadBe32(data, offset + 8),
    });
    offset += 12;
  }
  return entries;
}

std::vector<std::uint32_t> ParseStss(std::span<const std::uint8_t> data,
                                     const StructureNode* stss) {
  std::vector<std::uint32_t> sync_samples;
  if (!stss) {
    return sync_samples;
  }

  const auto payload = stss->offset + stss->header_size;
  if (!CanRead(data, payload + 4, 4)) {
    return sync_samples;
  }
  const auto entry_count = ReadBe32(data, payload + 4);
  sync_samples.reserve(entry_count);
  std::uint64_t offset = payload + 8;
  for (std::uint32_t i = 0; i < entry_count && CanRead(data, offset, 4); ++i) {
    sync_samples.push_back(ReadBe32(data, offset));
    offset += 4;
  }
  std::sort(sync_samples.begin(), sync_samples.end());
  return sync_samples;
}

void ParseSampleTable(std::span<const std::uint8_t> data, const StructureNode& stbl,
                      TrackInfo& track, IsoBmffAnalysis& analysis) {
  std::uint32_t sample_count = track.sample_count;
  const auto sizes = ParseStszSizes(data, FindChild(stbl, "stsz"), sample_count);
  track.sample_count = sample_count;
  track.sample_table_total = sample_count;
  if (sample_count == 0 || sizes.empty()) {
    return;
  }

  const auto chunk_offsets = ParseChunkOffsets(data, FindChild(stbl, "stco"), FindChild(stbl, "co64"));
  const auto stsc_entries = ParseStsc(data, FindChild(stbl, "stsc"));
  const auto durations = ParseSttsDurations(data, FindChild(stbl, "stts"), sample_count);
  const auto composition_offsets = ParseCttsOffsets(data, FindChild(stbl, "ctts"), sample_count);
  const auto sync_samples = ParseStss(data, FindChild(stbl, "stss"));
  if (chunk_offsets.empty() || stsc_entries.empty()) {
    analysis.warnings.push_back("sample table missing chunk offsets or stsc mapping");
  }

  std::vector<std::uint64_t> sample_offsets(sample_count, 0);
  std::uint32_t sample_index = 0;
  std::size_t stsc_index = 0;
  for (std::size_t chunk_index = 0; chunk_index < chunk_offsets.size() && sample_index < sample_count; ++chunk_index) {
    const auto one_based_chunk = static_cast<std::uint32_t>(chunk_index + 1);
    while (stsc_index + 1 < stsc_entries.size() &&
           stsc_entries[stsc_index + 1].first_chunk <= one_based_chunk) {
      ++stsc_index;
    }
    const auto samples_per_chunk = stsc_entries.empty() ? 0 : stsc_entries[stsc_index].samples_per_chunk;
    std::uint64_t offset = chunk_offsets[chunk_index];
    for (std::uint32_t i = 0; i < samples_per_chunk && sample_index < sample_count; ++i) {
      sample_offsets[sample_index] = offset;
      offset += sizes[sample_index];
      ++sample_index;
    }
  }

  std::uint64_t dts = 0;
  const auto output_count = std::min<std::uint32_t>(sample_count, kSampleOutputLimit);
  track.samples.reserve(output_count);
  for (std::uint32_t i = 0; i < sample_count; ++i) {
    const auto duration = i < durations.size() ? durations[i] : 0;
    const auto composition_offset = i < composition_offsets.size() ? composition_offsets[i] : 0;
    if (i < output_count) {
      const bool sync = sync_samples.empty() ||
          std::binary_search(sync_samples.begin(), sync_samples.end(), i + 1);
      SampleInfo sample;
      sample.index = i + 1;
      sample.offset = sample_offsets[i];
      sample.size = sizes[i];
      sample.dts = dts;
      sample.pts = static_cast<std::int64_t>(dts) + composition_offset;
      sample.duration = duration;
      sample.composition_offset = composition_offset;
      sample.sync = sync;
      if (sample.offset != 0 && sample.size != 0) {
        sample.bytes = ByteRange{sample.offset, sample.size};
      }
      track.samples.push_back(sample);
    }
    dts += duration;
  }
  track.sample_table_truncated = sample_count > output_count;
}

void ParseStsd(std::span<const std::uint8_t> data, const StructureNode& stsd, TrackInfo& track) {
  const auto payload = stsd.offset + stsd.header_size;
  if (!CanRead(data, payload + 4, 4)) {
    return;
  }
  track.sample_description_count = ReadBe32(data, payload + 4);

  if (stsd.children.empty()) {
    return;
  }

  const auto& entry = stsd.children.front();
  track.codec.fourcc = entry.type;
  track.codec.description = CodecDescription(entry.type);
  const auto entry_payload = entry.offset + entry.header_size;
  if (IsVisualSampleEntry(entry.type) && CanRead(data, entry_payload + 24, 4)) {
    track.width = ReadBe16(data, entry_payload + 24);
    track.height = ReadBe16(data, entry_payload + 26);
  }
  if (IsAudioSampleEntry(entry.type) && CanRead(data, entry_payload + 16, 12)) {
    track.channel_count = ReadBe16(data, entry_payload + 16);
    track.sample_rate = ReadBe32(data, entry_payload + 24) >> 16;
  }

  for (const auto& child : entry.children) {
    const auto payload_offset = child.offset + child.header_size;
    const auto payload_size = child.size - child.header_size;
    if (child.type == "avcC") {
      auto codec = ParseAvcC(data, payload_offset, payload_size, entry.type);
      if (codec.has_value()) {
        codec->raw_header_bytes = ByteRange{child.offset, child.size};
        track.codec = *codec;
        if (!track.width && codec->width) {
          track.width = codec->width;
        }
        if (!track.height && codec->height) {
          track.height = codec->height;
        }
      }
    } else if (child.type == "hvcC") {
      auto codec = ParseHvcC(data, payload_offset, payload_size, entry.type);
      if (codec.has_value()) {
        codec->raw_header_bytes = ByteRange{child.offset, child.size};
        track.codec = *codec;
      }
    } else if (child.type == "esds") {
      auto codec = ParseEsdsAac(data, payload_offset, payload_size, entry.type);
      if (codec.has_value()) {
        codec->raw_header_bytes = ByteRange{child.offset, child.size};
        track.codec = *codec;
        if (codec->asc_sample_rate) {
          track.sample_rate = codec->asc_sample_rate;
        }
        if (codec->channel_config) {
          track.channel_count = codec->channel_config;
        }
      }
    }
  }
}

void ParseTrack(std::span<const std::uint8_t> data, const StructureNode& trak,
                IsoBmffAnalysis& analysis) {
  TrackInfo track;
  if (const auto* tkhd = FindChild(trak, "tkhd")) {
    ParseTkhd(data, *tkhd, track);
  }

  const auto* mdia = FindChild(trak, "mdia");
  if (!mdia) {
    analysis.warnings.push_back("trak missing mdia");
    analysis.tracks.push_back(track);
    return;
  }

  if (const auto* mdhd = FindChild(*mdia, "mdhd")) {
    track.duration = ParseMdhd(data, *mdhd);
  }
  if (const auto* hdlr = FindChild(*mdia, "hdlr")) {
    track.type = ParseHdlr(data, *hdlr);
  }
  if (const auto* minf = FindChild(*mdia, "minf")) {
    if (const auto* stbl = FindChild(*minf, "stbl")) {
      if (const auto* stsd = FindChild(*stbl, "stsd")) {
        ParseStsd(data, *stsd, track);
      }
      ParseSampleTable(data, *stbl, track, analysis);
    }
  }

  analysis.tracks.push_back(track);
}

void WriteTimeJson(std::ostringstream& out, const RationalTime& time, int indent) {
  out << "{\n";
  out << Indent(indent + 2) << "\"value\": " << time.value << ",\n";
  out << Indent(indent + 2) << "\"timescale\": " << time.timescale << ",\n";
  out << Indent(indent + 2) << "\"seconds\": " << std::fixed << std::setprecision(6)
      << Seconds(time) << "\n";
  out << Indent(indent) << "}";
}

void WriteStructureJson(std::ostringstream& out, const StructureNode& node, int indent) {
  out << "{\n";
  out << Indent(indent + 2) << "\"type\": \"" << JsonEscape(node.type) << "\",\n";
  out << Indent(indent + 2) << "\"offset\": " << node.offset << ",\n";
  out << Indent(indent + 2) << "\"size\": " << node.size << ",\n";
  out << Indent(indent + 2) << "\"header_size\": " << node.header_size << ",\n";
  out << Indent(indent + 2) << "\"bytes\": {\n";
  out << Indent(indent + 4) << "\"offset\": " << node.preview_offset << ",\n";
  out << Indent(indent + 4) << "\"length\": " << node.preview_length << ",\n";
  out << Indent(indent + 4) << "\"truncated\": "
      << (node.preview_truncated ? "true" : "false") << ",\n";
  out << Indent(indent + 4) << "\"hex\": \"" << JsonEscape(node.hex_preview) << "\",\n";
  out << Indent(indent + 4) << "\"ascii\": \"" << JsonEscape(node.ascii_preview) << "\"\n";
  out << Indent(indent + 2) << "}";
  if (!node.children.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"children\": [\n";
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      out << Indent(indent + 4);
      WriteStructureJson(out, node.children[i], indent + 4);
      if (i + 1 != node.children.size()) {
        out << ",";
      }
      out << "\n";
    }
    out << Indent(indent + 2) << "]\n";
    out << Indent(indent) << "}";
  } else {
    out << "\n" << Indent(indent) << "}";
  }
}

void WriteOptionalU32(std::ostringstream& out, const char* key, std::optional<std::uint32_t> value,
                      int indent, bool& needs_comma) {
  if (!value) {
    return;
  }
  if (needs_comma) {
    out << ",\n";
  }
  out << Indent(indent) << "\"" << key << "\": " << *value;
  needs_comma = true;
}

void WriteCodecJson(std::ostringstream& out, const CodecInfo& codec, int indent) {
  out << "{\n";
  out << Indent(indent + 2) << "\"fourcc\": \"" << JsonEscape(codec.fourcc) << "\",\n";
  out << Indent(indent + 2) << "\"description\": \"" << JsonEscape(codec.description) << "\"";
  bool needs_comma = true;
  if (!codec.profile.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"profile\": \"" << JsonEscape(codec.profile) << "\"";
  }
  if (!codec.level.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"level\": \"" << JsonEscape(codec.level) << "\"";
  }
  WriteOptionalU32(out, "width", codec.width, indent + 2, needs_comma);
  WriteOptionalU32(out, "height", codec.height, indent + 2, needs_comma);
  WriteOptionalU32(out, "length_size", codec.length_size, indent + 2, needs_comma);
  WriteOptionalU32(out, "vps_count", codec.vps_count, indent + 2, needs_comma);
  WriteOptionalU32(out, "sps_count", codec.sps_count, indent + 2, needs_comma);
  WriteOptionalU32(out, "pps_count", codec.pps_count, indent + 2, needs_comma);
  WriteOptionalU32(out, "bit_depth_luma", codec.bit_depth_luma, indent + 2, needs_comma);
  WriteOptionalU32(out, "bit_depth_chroma", codec.bit_depth_chroma, indent + 2, needs_comma);
  WriteOptionalU32(out, "chroma_format", codec.chroma_format, indent + 2, needs_comma);
  WriteOptionalU32(out, "audio_object_type", codec.audio_object_type, indent + 2, needs_comma);
  WriteOptionalU32(out, "asc_sample_rate", codec.asc_sample_rate, indent + 2, needs_comma);
  WriteOptionalU32(out, "channel_config", codec.channel_config, indent + 2, needs_comma);
  if (!codec.raw_header_hex.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"raw_header_hex\": \""
        << JsonEscape(codec.raw_header_hex) << "\"";
  }
  if (codec.raw_header_bytes) {
    out << ",\n" << Indent(indent + 2) << "\"raw_header_bytes\": {\n";
    out << Indent(indent + 4) << "\"offset\": " << codec.raw_header_bytes->offset << ",\n";
    out << Indent(indent + 4) << "\"length\": " << codec.raw_header_bytes->length << "\n";
    out << Indent(indent + 2) << "}";
  }
  if (!codec.vps_hex.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"vps_hex\": \""
        << JsonEscape(codec.vps_hex) << "\"";
  }
  if (!codec.sps_hex.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"sps_hex\": \""
        << JsonEscape(codec.sps_hex) << "\"";
  }
  if (!codec.pps_hex.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"pps_hex\": \""
        << JsonEscape(codec.pps_hex) << "\"";
  }
  if (!codec.asc_hex.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"asc_hex\": \""
        << JsonEscape(codec.asc_hex) << "\"";
  }
  if (codec.vps_bytes) {
    out << ",\n" << Indent(indent + 2) << "\"vps_bytes\": {\n";
    out << Indent(indent + 4) << "\"offset\": " << codec.vps_bytes->offset << ",\n";
    out << Indent(indent + 4) << "\"length\": " << codec.vps_bytes->length << "\n";
    out << Indent(indent + 2) << "}";
  }
  if (codec.sps_bytes) {
    out << ",\n" << Indent(indent + 2) << "\"sps_bytes\": {\n";
    out << Indent(indent + 4) << "\"offset\": " << codec.sps_bytes->offset << ",\n";
    out << Indent(indent + 4) << "\"length\": " << codec.sps_bytes->length << "\n";
    out << Indent(indent + 2) << "}";
  }
  if (codec.pps_bytes) {
    out << ",\n" << Indent(indent + 2) << "\"pps_bytes\": {\n";
    out << Indent(indent + 4) << "\"offset\": " << codec.pps_bytes->offset << ",\n";
    out << Indent(indent + 4) << "\"length\": " << codec.pps_bytes->length << "\n";
    out << Indent(indent + 2) << "}";
  }
  if (codec.asc_bytes) {
    out << ",\n" << Indent(indent + 2) << "\"asc_bytes\": {\n";
    out << Indent(indent + 4) << "\"offset\": " << codec.asc_bytes->offset << ",\n";
    out << Indent(indent + 4) << "\"length\": " << codec.asc_bytes->length << "\n";
    out << Indent(indent + 2) << "}";
  }
  out << "\n" << Indent(indent) << "}";
}

void WriteSampleJson(std::ostringstream& out, const SampleInfo& sample, int indent) {
  out << "{\n";
  out << Indent(indent + 2) << "\"index\": " << sample.index << ",\n";
  out << Indent(indent + 2) << "\"offset\": " << sample.offset << ",\n";
  out << Indent(indent + 2) << "\"size\": " << sample.size << ",\n";
  out << Indent(indent + 2) << "\"dts\": " << sample.dts << ",\n";
  out << Indent(indent + 2) << "\"pts\": " << sample.pts << ",\n";
  out << Indent(indent + 2) << "\"duration\": " << sample.duration << ",\n";
  out << Indent(indent + 2) << "\"composition_offset\": " << sample.composition_offset << ",\n";
  out << Indent(indent + 2) << "\"sync\": " << (sample.sync ? "true" : "false");
  if (sample.bytes) {
    out << ",\n" << Indent(indent + 2) << "\"bytes\": {\n";
    out << Indent(indent + 4) << "\"offset\": " << sample.bytes->offset << ",\n";
    out << Indent(indent + 4) << "\"length\": " << sample.bytes->length << "\n";
    out << Indent(indent + 2) << "}";
  }
  out << "\n" << Indent(indent) << "}";
}

void WriteTrackJson(std::ostringstream& out, const TrackInfo& track, int indent) {
  out << "{\n";
  out << Indent(indent + 2) << "\"id\": " << track.id << ",\n";
  out << Indent(indent + 2) << "\"type\": \"" << JsonEscape(track.type) << "\",\n";
  out << Indent(indent + 2) << "\"duration\": ";
  WriteTimeJson(out, track.duration, indent + 2);
  out << ",\n";
  if (track.width) {
    out << Indent(indent + 2) << "\"width\": " << *track.width << ",\n";
  }
  if (track.height) {
    out << Indent(indent + 2) << "\"height\": " << *track.height << ",\n";
  }
  if (track.channel_count) {
    out << Indent(indent + 2) << "\"channel_count\": " << *track.channel_count << ",\n";
  }
  if (track.sample_rate) {
    out << Indent(indent + 2) << "\"sample_rate\": " << *track.sample_rate << ",\n";
  }
  out << Indent(indent + 2) << "\"sample_count\": " << track.sample_count << ",\n";
  out << Indent(indent + 2) << "\"sample_description_count\": "
      << track.sample_description_count << ",\n";
  out << Indent(indent + 2) << "\"sample_table_total\": "
      << track.sample_table_total << ",\n";
  out << Indent(indent + 2) << "\"sample_table_truncated\": "
      << (track.sample_table_truncated ? "true" : "false") << ",\n";
  out << Indent(indent + 2) << "\"samples\": [\n";
  for (std::size_t i = 0; i < track.samples.size(); ++i) {
    out << Indent(indent + 4);
    WriteSampleJson(out, track.samples[i], indent + 4);
    if (i + 1 != track.samples.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << Indent(indent + 2) << "],\n";
  out << Indent(indent + 2) << "\"codec\": ";
  WriteCodecJson(out, track.codec, indent + 2);
  out << "\n" << Indent(indent) << "}";
}

}  // namespace

std::optional<IsoBmffAnalysis> ParseIsoBmff(std::span<const std::uint8_t> data) {
  if (data.size() < 12 || ReadFourcc(data, 4) != "ftyp") {
    return std::nullopt;
  }

  IsoBmffAnalysis analysis;
  analysis.structure = ParseBoxNodes(data, 0, data.size(), 12);

  if (const auto* ftyp = FindTop(analysis.structure, "ftyp")) {
    ParseFtyp(data, *ftyp, analysis);
  }

  const auto* moov = FindTop(analysis.structure, "moov");
  if (!moov) {
    analysis.warnings.push_back("moov box not found in analyzed bytes");
    return analysis;
  }

  if (const auto* mvhd = FindChild(*moov, "mvhd")) {
    analysis.duration = ParseMvhd(data, *mvhd);
  }

  for (const auto* trak : FindChildren(*moov, "trak")) {
    ParseTrack(data, *trak, analysis);
  }

  return analysis;
}

std::string IsoBmffAnalysisToJson(const IsoBmffAnalysis& analysis, int indent) {
  std::ostringstream out;
  out << "{\n";
  out << Indent(indent + 2) << "\"format\": \"ISO-BMFF\",\n";
  out << Indent(indent + 2) << "\"major_brand\": \"" << JsonEscape(analysis.major_brand) << "\",\n";
  out << Indent(indent + 2) << "\"minor_version\": " << analysis.minor_version << ",\n";
  out << Indent(indent + 2) << "\"compatible_brands\": [";
  for (std::size_t i = 0; i < analysis.compatible_brands.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }
    out << "\"" << JsonEscape(analysis.compatible_brands[i]) << "\"";
  }
  out << "],\n";

  out << Indent(indent + 2) << "\"duration\": ";
  WriteTimeJson(out, analysis.duration, indent + 2);
  out << ",\n";

  out << Indent(indent + 2) << "\"tracks\": [\n";
  for (std::size_t i = 0; i < analysis.tracks.size(); ++i) {
    out << Indent(indent + 4);
    WriteTrackJson(out, analysis.tracks[i], indent + 4);
    if (i + 1 != analysis.tracks.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << Indent(indent + 2) << "],\n";

  out << Indent(indent + 2) << "\"structure\": [\n";
  for (std::size_t i = 0; i < analysis.structure.size(); ++i) {
    out << Indent(indent + 4);
    WriteStructureJson(out, analysis.structure[i], indent + 4);
    if (i + 1 != analysis.structure.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << Indent(indent + 2) << "],\n";

  out << Indent(indent + 2) << "\"warnings\": [";
  for (std::size_t i = 0; i < analysis.warnings.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }
    out << "\"" << JsonEscape(analysis.warnings[i]) << "\"";
  }
  out << "]\n";
  out << Indent(indent) << "}";
  return out.str();
}

}  // namespace media_analyzer
