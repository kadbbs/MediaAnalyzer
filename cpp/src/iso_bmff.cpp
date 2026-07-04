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

void ParseStsz(std::span<const std::uint8_t> data, const StructureNode& stsz, TrackInfo& track) {
  const auto payload = stsz.offset + stsz.header_size;
  if (CanRead(data, payload + 8, 4)) {
    track.sample_count = ReadBe32(data, payload + 8);
  }
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
    if (child.type == "avcC") {
      const auto payload_offset = child.offset + child.header_size;
      const auto payload_size = child.size - child.header_size;
      auto codec = ParseAvcC(data, payload_offset, payload_size, entry.type);
      if (codec.has_value()) {
        track.codec = *codec;
        if (!track.width && codec->width) {
          track.width = codec->width;
        }
        if (!track.height && codec->height) {
          track.height = codec->height;
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
      if (const auto* stsz = FindChild(*stbl, "stsz")) {
        ParseStsz(data, *stsz, track);
      }
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
  WriteOptionalU32(out, "sps_count", codec.sps_count, indent + 2, needs_comma);
  WriteOptionalU32(out, "pps_count", codec.pps_count, indent + 2, needs_comma);
  if (!codec.raw_header_hex.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"raw_header_hex\": \""
        << JsonEscape(codec.raw_header_hex) << "\"";
  }
  if (!codec.sps_hex.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"sps_hex\": \""
        << JsonEscape(codec.sps_hex) << "\"";
  }
  if (!codec.pps_hex.empty()) {
    out << ",\n" << Indent(indent + 2) << "\"pps_hex\": \""
        << JsonEscape(codec.pps_hex) << "\"";
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
