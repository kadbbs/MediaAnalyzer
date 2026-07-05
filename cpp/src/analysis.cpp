#include "media_analyzer/analysis.h"

#include "media_analyzer/format_detector.h"
#include "media_analyzer/iso_bmff.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

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

std::uint16_t ReadBe16(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if (offset + 2 > data.size()) {
    return 0;
  }
  return static_cast<std::uint16_t>((data[offset] << 8) | data[offset + 1]);
}

std::uint32_t ReadBe24(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if (offset + 3 > data.size()) {
    return 0;
  }
  return (static_cast<std::uint32_t>(data[offset]) << 16) |
         (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
         static_cast<std::uint32_t>(data[offset + 2]);
}

std::uint16_t ReadLe16(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if (offset + 2 > data.size()) {
    return 0;
  }
  return static_cast<std::uint16_t>(data[offset] | (data[offset + 1] << 8));
}

std::uint32_t ReadLe32(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if (offset + 4 > data.size()) {
    return 0;
  }
  return static_cast<std::uint32_t>(data[offset]) |
         (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

std::uint64_t ReadLe64(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if (offset + 8 > data.size()) {
    return 0;
  }
  return static_cast<std::uint64_t>(ReadLe32(data, offset)) |
         (static_cast<std::uint64_t>(ReadLe32(data, offset + 4)) << 32);
}

std::string BytesToHexAt(const std::vector<std::uint8_t>& data,
                         std::size_t offset,
                         std::size_t length) {
  std::ostringstream out;
  const auto end = std::min(data.size(), offset + length);
  for (std::size_t i = offset; i < end; ++i) {
    if (i != offset) {
      out << ' ';
    }
    out << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(data[i]);
  }
  return out.str();
}

class BitReader {
 public:
  explicit BitReader(std::vector<std::uint8_t> data) : data_(std::move(data)) {}

  std::uint64_t ReadBits(std::size_t count) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < count; ++i) {
      value <<= 1;
      if (bit_offset_ >= data_.size() * 8) {
        ok_ = false;
        continue;
      }
      const auto byte = data_[bit_offset_ / 8];
      const auto bit = 7 - (bit_offset_ % 8);
      value |= (byte >> bit) & 0x01;
      ++bit_offset_;
    }
    return value;
  }

  std::uint32_t ReadUe() {
    std::size_t leading_zero_bits = 0;
    while (bit_offset_ < data_.size() * 8 && ReadBits(1) == 0) {
      ++leading_zero_bits;
      if (leading_zero_bits > 31) {
        ok_ = false;
        return 0;
      }
    }
    if (leading_zero_bits == 0) {
      return 0;
    }
    return static_cast<std::uint32_t>((1u << leading_zero_bits) - 1 + ReadBits(leading_zero_bits));
  }

  std::int32_t ReadSe() {
    const auto code_num = ReadUe();
    const auto magnitude = static_cast<std::int32_t>((code_num + 1) / 2);
    return (code_num % 2) ? magnitude : -magnitude;
  }

  void SkipBits(std::size_t count) {
    (void)ReadBits(count);
  }

  bool Ok() const {
    return ok_;
  }

 private:
  std::vector<std::uint8_t> data_;
  std::size_t bit_offset_ = 0;
  bool ok_ = true;
};

std::vector<std::uint8_t> ToRbsp(const std::vector<std::uint8_t>& data,
                                 std::size_t offset,
                                 std::size_t length) {
  std::vector<std::uint8_t> rbsp;
  const auto end = std::min(data.size(), offset + length);
  rbsp.reserve(end > offset ? end - offset : 0);
  int zero_count = 0;
  for (std::size_t i = offset; i < end; ++i) {
    const auto byte = data[i];
    if (zero_count >= 2 && byte == 0x03) {
      zero_count = 0;
      continue;
    }
    rbsp.push_back(byte);
    if (byte == 0) {
      ++zero_count;
    } else {
      zero_count = 0;
    }
  }
  return rbsp;
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
      return "profile " + std::to_string(profile_idc);
  }
}

std::string H264LevelName(std::uint32_t level_idc) {
  return std::to_string(level_idc / 10) + "." + std::to_string(level_idc % 10);
}

std::string H264NalName(std::uint32_t nal_type) {
  switch (nal_type) {
    case 1:
      return "non-IDR slice";
    case 5:
      return "IDR slice";
    case 6:
      return "SEI";
    case 7:
      return "SPS";
    case 8:
      return "PPS";
    case 9:
      return "AUD";
    default:
      return "NAL type " + std::to_string(nal_type);
  }
}

std::string HevcNalName(std::uint32_t nal_type) {
  switch (nal_type) {
    case 19:
      return "IDR_W_RADL";
    case 20:
      return "IDR_N_LP";
    case 32:
      return "VPS";
    case 33:
      return "SPS";
    case 34:
      return "PPS";
    case 35:
      return "AUD";
    case 39:
      return "PREFIX_SEI";
    case 40:
      return "SUFFIX_SEI";
    default:
      return "NAL type " + std::to_string(nal_type);
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
      return "profile " + std::to_string(profile_idc);
  }
}

std::string HevcLevelName(std::uint32_t level_idc) {
  std::ostringstream out;
  out << (level_idc / 30);
  if (level_idc % 30 != 0) {
    out << "." << ((level_idc % 30) / 3);
  }
  return out.str();
}

struct ParsedParamSet {
  std::string kind;
  std::size_t offset = 0;
  std::size_t length = 0;
  std::uint32_t nal_type = 0;
  std::optional<std::uint32_t> profile_idc;
  std::string profile;
  std::optional<std::uint32_t> level_idc;
  std::string level;
  std::optional<std::uint32_t> width;
  std::optional<std::uint32_t> height;
  std::optional<std::uint32_t> id;
  std::optional<std::uint32_t> referenced_id;
  std::optional<std::uint32_t> chroma_format_idc;
  std::optional<std::uint32_t> bit_depth_luma;
  std::optional<std::uint32_t> bit_depth_chroma;
  std::optional<std::uint32_t> max_sub_layers_minus1;
};

std::optional<ParsedParamSet> ParseH264Sps(const std::vector<std::uint8_t>& data,
                                           std::size_t offset,
                                           std::size_t length) {
  if (length < 2 || offset + length > data.size()) {
    return std::nullopt;
  }
  auto rbsp = ToRbsp(data, offset + 1, length - 1);
  BitReader bits{std::move(rbsp)};
  ParsedParamSet out;
  out.kind = "SPS";
  out.offset = offset;
  out.length = length;
  out.nal_type = 7;
  const auto profile_idc = static_cast<std::uint32_t>(bits.ReadBits(8));
  bits.ReadBits(8);
  const auto level_idc = static_cast<std::uint32_t>(bits.ReadBits(8));
  const auto seq_parameter_set_id = bits.ReadUe();
  out.profile_idc = profile_idc;
  out.profile = H264ProfileName(profile_idc);
  out.level_idc = level_idc;
  out.level = H264LevelName(level_idc);
  out.id = seq_parameter_set_id;

  std::uint32_t chroma_format_idc = 1;
  bool separate_colour_plane_flag = false;
  const bool high_profile =
      profile_idc == 100 || profile_idc == 110 || profile_idc == 122 ||
      profile_idc == 244 || profile_idc == 44 || profile_idc == 83 ||
      profile_idc == 86 || profile_idc == 118 || profile_idc == 128 ||
      profile_idc == 138 || profile_idc == 144;
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
          int last_scale = 8;
          int next_scale = 8;
          const int size = i < 6 ? 16 : 64;
          for (int j = 0; j < size; ++j) {
            if (next_scale != 0) {
              const auto delta_scale = bits.ReadSe();
              next_scale = (last_scale + delta_scale + 256) % 256;
            }
            last_scale = (next_scale == 0) ? last_scale : next_scale;
          }
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
  std::uint32_t crop_unit_y = 2 - static_cast<std::uint32_t>(frame_mbs_only_flag);
  if (!separate_colour_plane_flag) {
    if (chroma_format_idc == 1) {
      crop_unit_x = 2;
      crop_unit_y = 2 * (2 - static_cast<std::uint32_t>(frame_mbs_only_flag));
    } else if (chroma_format_idc == 2) {
      crop_unit_x = 2;
      crop_unit_y = 2 - static_cast<std::uint32_t>(frame_mbs_only_flag);
    }
  }

  const auto coded_width = (pic_width_in_mbs_minus1 + 1) * 16;
  const auto coded_height = (2 - static_cast<std::uint32_t>(frame_mbs_only_flag)) *
                            (pic_height_in_map_units_minus1 + 1) * 16;
  out.width = coded_width - (crop_left + crop_right) * crop_unit_x;
  out.height = coded_height - (crop_top + crop_bottom) * crop_unit_y;
  out.chroma_format_idc = chroma_format_idc;
  return out;
}

std::optional<ParsedParamSet> ParseH264Pps(const std::vector<std::uint8_t>& data,
                                           std::size_t offset,
                                           std::size_t length) {
  if (length < 2 || offset + length > data.size()) {
    return std::nullopt;
  }
  auto rbsp = ToRbsp(data, offset + 1, length - 1);
  BitReader bits{std::move(rbsp)};
  ParsedParamSet out;
  out.kind = "PPS";
  out.offset = offset;
  out.length = length;
  out.nal_type = 8;
  out.id = bits.ReadUe();
  out.referenced_id = bits.ReadUe();
  return out;
}

void SkipHevcProfileTierLevel(BitReader& bits, std::uint32_t max_sub_layers_minus1,
                              ParsedParamSet* target) {
  const auto profile_space = bits.ReadBits(2);
  const auto tier_flag = bits.ReadBits(1);
  const auto profile_idc = static_cast<std::uint32_t>(bits.ReadBits(5));
  bits.SkipBits(32);
  bits.SkipBits(48);
  const auto level_idc = static_cast<std::uint32_t>(bits.ReadBits(8));
  if (target) {
    target->profile_idc = profile_idc;
    target->profile = HevcProfileName(profile_idc);
    if (profile_space != 0) {
      target->profile += " profile_space " + std::to_string(profile_space);
    }
    target->level_idc = level_idc;
    target->level = std::string(tier_flag ? "High tier " : "Main tier ") + HevcLevelName(level_idc);
  }

  std::array<bool, 8> sub_layer_profile_present{};
  std::array<bool, 8> sub_layer_level_present{};
  for (std::uint32_t i = 0; i < max_sub_layers_minus1 && i < sub_layer_profile_present.size(); ++i) {
    sub_layer_profile_present[i] = bits.ReadBits(1) != 0;
    sub_layer_level_present[i] = bits.ReadBits(1) != 0;
  }
  if (max_sub_layers_minus1 > 0) {
    for (std::uint32_t i = max_sub_layers_minus1; i < 8; ++i) {
      bits.SkipBits(2);
    }
  }
  for (std::uint32_t i = 0; i < max_sub_layers_minus1 && i < sub_layer_profile_present.size(); ++i) {
    if (sub_layer_profile_present[i]) {
      bits.SkipBits(88);
    }
    if (sub_layer_level_present[i]) {
      bits.SkipBits(8);
    }
  }
}

std::optional<ParsedParamSet> ParseHevcVps(const std::vector<std::uint8_t>& data,
                                           std::size_t offset,
                                           std::size_t length) {
  if (length < 4 || offset + length > data.size()) {
    return std::nullopt;
  }
  auto rbsp = ToRbsp(data, offset + 2, length - 2);
  BitReader bits{std::move(rbsp)};
  ParsedParamSet out;
  out.kind = "VPS";
  out.offset = offset;
  out.length = length;
  out.nal_type = 32;
  out.id = static_cast<std::uint32_t>(bits.ReadBits(4));
  bits.SkipBits(2);
  bits.SkipBits(6);
  out.max_sub_layers_minus1 = static_cast<std::uint32_t>(bits.ReadBits(3));
  bits.SkipBits(1);
  return out;
}

std::optional<ParsedParamSet> ParseHevcSps(const std::vector<std::uint8_t>& data,
                                           std::size_t offset,
                                           std::size_t length) {
  if (length < 4 || offset + length > data.size()) {
    return std::nullopt;
  }
  auto rbsp = ToRbsp(data, offset + 2, length - 2);
  BitReader bits{std::move(rbsp)};
  ParsedParamSet out;
  out.kind = "SPS";
  out.offset = offset;
  out.length = length;
  out.nal_type = 33;
  out.referenced_id = static_cast<std::uint32_t>(bits.ReadBits(4));
  const auto max_sub_layers_minus1 = static_cast<std::uint32_t>(bits.ReadBits(3));
  out.max_sub_layers_minus1 = max_sub_layers_minus1;
  bits.SkipBits(1);
  SkipHevcProfileTierLevel(bits, max_sub_layers_minus1, &out);
  out.id = bits.ReadUe();
  auto chroma_format_idc = bits.ReadUe();
  bool separate_colour_plane_flag = false;
  if (chroma_format_idc == 3) {
    separate_colour_plane_flag = bits.ReadBits(1) != 0;
  }
  auto width = bits.ReadUe();
  auto height = bits.ReadUe();
  if (bits.ReadBits(1) != 0) {
    const auto conf_win_left = bits.ReadUe();
    const auto conf_win_right = bits.ReadUe();
    const auto conf_win_top = bits.ReadUe();
    const auto conf_win_bottom = bits.ReadUe();
    std::uint32_t sub_width_c = 1;
    std::uint32_t sub_height_c = 1;
    if (!separate_colour_plane_flag) {
      if (chroma_format_idc == 1) {
        sub_width_c = 2;
        sub_height_c = 2;
      } else if (chroma_format_idc == 2) {
        sub_width_c = 2;
      }
    }
    width -= std::min(width, (conf_win_left + conf_win_right) * sub_width_c);
    height -= std::min(height, (conf_win_top + conf_win_bottom) * sub_height_c);
  }
  out.chroma_format_idc = chroma_format_idc;
  out.width = width;
  out.height = height;
  out.bit_depth_luma = bits.ReadUe() + 8;
  out.bit_depth_chroma = bits.ReadUe() + 8;
  return out;
}

std::optional<ParsedParamSet> ParseHevcPps(const std::vector<std::uint8_t>& data,
                                           std::size_t offset,
                                           std::size_t length) {
  if (length < 3 || offset + length > data.size()) {
    return std::nullopt;
  }
  auto rbsp = ToRbsp(data, offset + 2, length - 2);
  BitReader bits{std::move(rbsp)};
  ParsedParamSet out;
  out.kind = "PPS";
  out.offset = offset;
  out.length = length;
  out.nal_type = 34;
  out.id = bits.ReadUe();
  out.referenced_id = bits.ReadUe();
  return out;
}

void WriteOptionalNumber(std::ostringstream& out, const char* name,
                         const std::optional<std::uint32_t>& value,
                         int indent) {
  if (!value) {
    return;
  }
  out << ",\n" << std::string(indent, ' ') << "\"" << name << "\": " << *value;
}

void WriteParamSetJson(std::ostringstream& out, const ParsedParamSet& set, int indent) {
  out << std::string(indent, ' ') << "{"
      << "\"kind\": \"" << JsonEscape(set.kind) << "\""
      << ", \"offset\": " << set.offset
      << ", \"length\": " << set.length
      << ", \"nal_type\": " << set.nal_type;
  WriteOptionalNumber(out, "profile_idc", set.profile_idc, indent + 2);
  if (!set.profile.empty()) {
    out << ",\n" << std::string(indent + 2, ' ') << "\"profile\": \"" << JsonEscape(set.profile) << "\"";
  }
  WriteOptionalNumber(out, "level_idc", set.level_idc, indent + 2);
  if (!set.level.empty()) {
    out << ",\n" << std::string(indent + 2, ' ') << "\"level\": \"" << JsonEscape(set.level) << "\"";
  }
  WriteOptionalNumber(out, "width", set.width, indent + 2);
  WriteOptionalNumber(out, "height", set.height, indent + 2);
  WriteOptionalNumber(out, "id", set.id, indent + 2);
  WriteOptionalNumber(out, "referenced_id", set.referenced_id, indent + 2);
  WriteOptionalNumber(out, "chroma_format_idc", set.chroma_format_idc, indent + 2);
  WriteOptionalNumber(out, "bit_depth_luma", set.bit_depth_luma, indent + 2);
  WriteOptionalNumber(out, "bit_depth_chroma", set.bit_depth_chroma, indent + 2);
  WriteOptionalNumber(out, "max_sub_layers_minus1", set.max_sub_layers_minus1, indent + 2);
  out << "\n" << std::string(indent, ' ') << "}";
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

std::string AnalyzeAnnexBToJson(const std::vector<std::uint8_t>& data,
                                int indent,
                                bool hevc) {
  std::ostringstream out;
  std::vector<ParsedParamSet> parameter_sets;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"Annex B "
      << (hevc ? "HEVC" : "H.264") << "\",\n";
  out << std::string(indent + 2, ' ') << "\"codec\": \""
      << (hevc ? "H.265/HEVC" : "H.264/AVC") << "\",\n";
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
    const auto nal_length = next - nal_start;
    const auto nal_type = hevc && nal_length >= 2
                              ? static_cast<std::uint32_t>((data[nal_start] >> 1) & 0x3f)
                              : static_cast<std::uint32_t>(data[nal_start] & 0x1f);
    if (hevc) {
      if (nal_type == 32) {
        if (auto parsed = ParseHevcVps(data, nal_start, nal_length)) {
          parameter_sets.push_back(*parsed);
        }
      } else if (nal_type == 33) {
        if (auto parsed = ParseHevcSps(data, nal_start, nal_length)) {
          parameter_sets.push_back(*parsed);
        }
      } else if (nal_type == 34) {
        if (auto parsed = ParseHevcPps(data, nal_start, nal_length)) {
          parameter_sets.push_back(*parsed);
        }
      }
    } else {
      if (nal_type == 7) {
        if (auto parsed = ParseH264Sps(data, nal_start, nal_length)) {
          parameter_sets.push_back(*parsed);
        }
      } else if (nal_type == 8) {
        if (auto parsed = ParseH264Pps(data, nal_start, nal_length)) {
          parameter_sets.push_back(*parsed);
        }
      }
    }
    if (count != 0) {
      out << ",\n";
    }
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (count + 1)
        << ", \"offset\": " << nal_start
        << ", \"length\": " << nal_length
        << ", \"start_code_offset\": " << start_code
        << ", \"start_code_length\": " << prefix
        << ", \"nal_type\": " << nal_type
        << ", \"nal_name\": \"" << JsonEscape(hevc ? HevcNalName(nal_type) : H264NalName(nal_type)) << "\"";
    if (hevc && nal_length >= 2) {
      const auto layer_id = static_cast<std::uint32_t>(((data[nal_start] & 0x01) << 5) |
                                                       ((data[nal_start + 1] >> 3) & 0x1f));
      const auto temporal_id_plus1 = static_cast<std::uint32_t>(data[nal_start + 1] & 0x07);
      out << ", \"nuh_layer_id\": " << layer_id
          << ", \"temporal_id_plus1\": " << temporal_id_plus1;
    }
    out << "}";
    ++count;
    start_code = next;
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"parameter_sets\": [\n";
  for (std::size_t i = 0; i < parameter_sets.size(); ++i) {
    if (i != 0) {
      out << ",\n";
    }
    WriteParamSetJson(out, parameter_sets[i], indent + 4);
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"nal_unit_count\": " << count << ",\n";
  out << std::string(indent + 2, ' ') << "\"parameter_set_count\": " << parameter_sets.size() << "\n";
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

bool ReadLeb128(const std::vector<std::uint8_t>& data,
                std::size_t& offset,
                std::size_t limit,
                std::uint64_t& value,
                std::size_t& width) {
  value = 0;
  width = 0;
  std::uint32_t shift = 0;
  while (offset < limit && width < 8) {
    const auto byte = data[offset++];
    value |= static_cast<std::uint64_t>(byte & 0x7f) << shift;
    ++width;
    if ((byte & 0x80) == 0) {
      return true;
    }
    shift += 7;
  }
  return false;
}

std::string Av1ObuName(std::uint32_t type) {
  switch (type) {
    case 1:
      return "Sequence Header";
    case 2:
      return "Temporal Delimiter";
    case 3:
      return "Frame Header";
    case 4:
      return "Tile Group";
    case 5:
      return "Metadata";
    case 6:
      return "Frame";
    case 7:
      return "Redundant Frame Header";
    case 8:
      return "Tile List";
    case 15:
      return "Padding";
    default:
      return "OBU type " + std::to_string(type);
  }
}

struct Av1SequenceHeader {
  std::size_t offset = 0;
  std::size_t length = 0;
  std::uint32_t seq_profile = 0;
  bool still_picture = false;
  bool reduced_still_picture_header = false;
  std::uint32_t seq_level_idx_0 = 0;
  std::uint32_t max_frame_width = 0;
  std::uint32_t max_frame_height = 0;
};

std::optional<Av1SequenceHeader> ParseAv1SequenceHeader(const std::vector<std::uint8_t>& data,
                                                        std::size_t offset,
                                                        std::size_t length) {
  if (offset + length > data.size() || length == 0) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> payload(data.begin() + static_cast<std::ptrdiff_t>(offset),
                                    data.begin() + static_cast<std::ptrdiff_t>(offset + length));
  BitReader bits{std::move(payload)};
  Av1SequenceHeader header;
  header.offset = offset;
  header.length = length;
  header.seq_profile = static_cast<std::uint32_t>(bits.ReadBits(3));
  header.still_picture = bits.ReadBits(1) != 0;
  header.reduced_still_picture_header = bits.ReadBits(1) != 0;
  if (header.reduced_still_picture_header) {
    header.seq_level_idx_0 = static_cast<std::uint32_t>(bits.ReadBits(5));
  } else {
    const bool timing_info_present_flag = bits.ReadBits(1) != 0;
    if (timing_info_present_flag) {
      bits.SkipBits(32);
      bits.SkipBits(32);
      if (bits.ReadBits(1) != 0) {
        bits.SkipBits(32);
        bits.SkipBits(5);
        bits.SkipBits(5);
        bits.SkipBits(5);
      }
    }
    const bool initial_display_delay_present_flag = bits.ReadBits(1) != 0;
    const auto operating_points_cnt_minus_1 = static_cast<std::uint32_t>(bits.ReadBits(5));
    for (std::uint32_t i = 0; i <= operating_points_cnt_minus_1 && i < 32; ++i) {
      bits.SkipBits(12);
      const auto seq_level_idx = static_cast<std::uint32_t>(bits.ReadBits(5));
      if (i == 0) {
        header.seq_level_idx_0 = seq_level_idx;
      }
      if (seq_level_idx > 7) {
        bits.SkipBits(1);
      }
      if (initial_display_delay_present_flag && bits.ReadBits(1) != 0) {
        bits.SkipBits(4);
      }
    }
  }
  const auto frame_width_bits_minus_1 = static_cast<std::uint32_t>(bits.ReadBits(4));
  const auto frame_height_bits_minus_1 = static_cast<std::uint32_t>(bits.ReadBits(4));
  header.max_frame_width =
      static_cast<std::uint32_t>(bits.ReadBits(frame_width_bits_minus_1 + 1)) + 1;
  header.max_frame_height =
      static_cast<std::uint32_t>(bits.ReadBits(frame_height_bits_minus_1 + 1)) + 1;
  return header;
}

std::string AnalyzeAv1ObuToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  std::vector<Av1SequenceHeader> sequence_headers;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"AV1 OBU\",\n";
  out << std::string(indent + 2, ' ') << "\"codec\": \"AV1\",\n";
  out << std::string(indent + 2, ' ') << "\"obu_units\": [\n";
  std::size_t offset = 0;
  std::size_t count = 0;
  while (offset < data.size() && count < 128) {
    const auto obu_offset = offset;
    const auto header = data[offset++];
    if ((header & 0x80) != 0) {
      break;
    }
    const auto obu_type = static_cast<std::uint32_t>((header >> 3) & 0x0f);
    const bool extension_flag = (header & 0x04) != 0;
    const bool has_size_field = (header & 0x02) != 0;
    const bool reserved_bit = (header & 0x01) != 0;
    std::uint32_t temporal_id = 0;
    std::uint32_t spatial_id = 0;
    if (extension_flag) {
      if (offset >= data.size()) {
        break;
      }
      const auto ext = data[offset++];
      temporal_id = (ext >> 5) & 0x07;
      spatial_id = (ext >> 3) & 0x03;
    }
    std::uint64_t payload_length = data.size() - offset;
    std::size_t leb_width = 0;
    if (has_size_field) {
      if (!ReadLeb128(data, offset, data.size(), payload_length, leb_width)) {
        break;
      }
    }
    if (offset + payload_length > data.size()) {
      break;
    }
    if (obu_type == 1) {
      if (auto parsed = ParseAv1SequenceHeader(data, offset, static_cast<std::size_t>(payload_length))) {
        sequence_headers.push_back(*parsed);
      }
    }
    if (count != 0) {
      out << ",\n";
    }
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (count + 1)
        << ", \"offset\": " << obu_offset
        << ", \"length\": " << (offset + payload_length - obu_offset)
        << ", \"header_size\": " << (offset - obu_offset)
        << ", \"payload_offset\": " << offset
        << ", \"payload_length\": " << payload_length
        << ", \"obu_type\": " << obu_type
        << ", \"obu_name\": \"" << JsonEscape(Av1ObuName(obu_type)) << "\""
        << ", \"has_size_field\": " << (has_size_field ? "true" : "false")
        << ", \"extension_flag\": " << (extension_flag ? "true" : "false")
        << ", \"reserved_bit\": " << (reserved_bit ? "true" : "false")
        << ", \"temporal_id\": " << temporal_id
        << ", \"spatial_id\": " << spatial_id
        << "}";
    offset += static_cast<std::size_t>(payload_length);
    ++count;
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"sequence_headers\": [\n";
  for (std::size_t i = 0; i < sequence_headers.size(); ++i) {
    if (i != 0) {
      out << ",\n";
    }
    const auto& header = sequence_headers[i];
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (i + 1)
        << ", \"offset\": " << header.offset
        << ", \"length\": " << header.length
        << ", \"seq_profile\": " << header.seq_profile
        << ", \"still_picture\": " << (header.still_picture ? "true" : "false")
        << ", \"reduced_still_picture_header\": "
        << (header.reduced_still_picture_header ? "true" : "false")
        << ", \"seq_level_idx_0\": " << header.seq_level_idx_0
        << ", \"max_frame_width\": " << header.max_frame_width
        << ", \"max_frame_height\": " << header.max_frame_height
        << "}";
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"obu_count\": " << count << ",\n";
  out << std::string(indent + 2, ' ') << "\"sequence_header_count\": "
      << sequence_headers.size() << "\n";
  out << std::string(indent, ' ') << "}";
  return out.str();
}

struct OpusHeadInfo {
  std::size_t offset = 0;
  std::size_t length = 0;
  std::uint32_t version = 0;
  std::uint32_t channel_count = 0;
  std::uint32_t pre_skip = 0;
  std::uint32_t input_sample_rate = 0;
  std::int32_t output_gain = 0;
  std::uint32_t channel_mapping_family = 0;
  std::optional<std::uint32_t> stream_count;
  std::optional<std::uint32_t> coupled_count;
};

std::optional<OpusHeadInfo> ParseOpusHeadAt(const std::vector<std::uint8_t>& data,
                                            std::size_t offset) {
  if (offset + 19 > data.size()) {
    return std::nullopt;
  }
  if (std::string_view(reinterpret_cast<const char*>(&data[offset]), 8) != "OpusHead") {
    return std::nullopt;
  }
  OpusHeadInfo info;
  info.offset = offset;
  info.length = 19;
  info.version = data[offset + 8];
  info.channel_count = data[offset + 9];
  info.pre_skip = ReadLe16(data, offset + 10);
  info.input_sample_rate = ReadLe32(data, offset + 12);
  info.output_gain = static_cast<std::int16_t>(ReadLe16(data, offset + 16));
  info.channel_mapping_family = data[offset + 18];
  if (info.channel_mapping_family != 0 && offset + 21 <= data.size()) {
    info.stream_count = data[offset + 19];
    info.coupled_count = data[offset + 20];
    const auto mapped_length = 21 + info.channel_count;
    if (offset + mapped_length <= data.size()) {
      info.length = mapped_length;
    }
  }
  return info;
}

void WriteOpusHeadJson(std::ostringstream& out, const OpusHeadInfo& info, int indent) {
  out << std::string(indent, ' ') << "{"
      << "\"offset\": " << info.offset
      << ", \"length\": " << info.length
      << ", \"version\": " << info.version
      << ", \"channel_count\": " << info.channel_count
      << ", \"pre_skip\": " << info.pre_skip
      << ", \"input_sample_rate\": " << info.input_sample_rate
      << ", \"output_gain\": " << info.output_gain
      << ", \"channel_mapping_family\": " << info.channel_mapping_family;
  if (info.stream_count) {
    out << ", \"stream_count\": " << *info.stream_count;
  }
  if (info.coupled_count) {
    out << ", \"coupled_count\": " << *info.coupled_count;
  }
  out << "}";
}

std::string AnalyzeOpusHeadToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"OpusHead\",\n";
  out << std::string(indent + 2, ' ') << "\"codec\": \"Opus\"";
  if (auto info = ParseOpusHeadAt(data, 0)) {
    out << ",\n" << std::string(indent + 2, ' ') << "\"opus_head\": ";
    WriteOpusHeadJson(out, *info, indent + 2);
    out << "\n";
  } else {
    out << "\n";
  }
  out << std::string(indent, ' ') << "}";
  return out.str();
}

std::string AnalyzeOggToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  std::optional<OpusHeadInfo> opus_head;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"Ogg\",\n";
  out << std::string(indent + 2, ' ') << "\"pages\": [\n";
  std::size_t offset = 0;
  std::size_t count = 0;
  while (offset + 27 <= data.size() && count < 64) {
    if (std::string_view(reinterpret_cast<const char*>(&data[offset]), 4) != "OggS") {
      break;
    }
    const auto page_offset = offset;
    const auto version = data[offset + 4];
    const auto header_type = data[offset + 5];
    const auto granule_position = ReadLe64(data, offset + 6);
    const auto serial = ReadLe32(data, offset + 14);
    const auto sequence = ReadLe32(data, offset + 18);
    const auto segment_count = data[offset + 26];
    const auto segment_table = offset + 27;
    if (segment_table + segment_count > data.size()) {
      break;
    }
    std::size_t body_length = 0;
    for (std::size_t i = 0; i < segment_count; ++i) {
      body_length += data[segment_table + i];
    }
    const auto body_offset = segment_table + segment_count;
    const auto page_length = 27 + segment_count + body_length;
    if (page_offset + page_length > data.size()) {
      break;
    }
    if (!opus_head && body_length >= 19) {
      opus_head = ParseOpusHeadAt(data, body_offset);
    }
    if (count != 0) {
      out << ",\n";
    }
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (count + 1)
        << ", \"offset\": " << page_offset
        << ", \"length\": " << page_length
        << ", \"version\": " << static_cast<int>(version)
        << ", \"header_type\": " << static_cast<int>(header_type)
        << ", \"granule_position\": " << granule_position
        << ", \"bitstream_serial_number\": " << serial
        << ", \"page_sequence_number\": " << sequence
        << ", \"segment_count\": " << static_cast<int>(segment_count)
        << ", \"body_offset\": " << body_offset
        << ", \"body_length\": " << body_length
        << "}";
    offset += page_length;
    ++count;
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"page_count\": " << count;
  if (opus_head) {
    out << ",\n" << std::string(indent + 2, ' ') << "\"opus_head\": ";
    WriteOpusHeadJson(out, *opus_head, indent + 2);
  }
  out << "\n" << std::string(indent, ' ') << "}";
  return out.str();
}

std::string FlacBlockTypeName(std::uint32_t type) {
  switch (type) {
    case 0:
      return "STREAMINFO";
    case 1:
      return "PADDING";
    case 2:
      return "APPLICATION";
    case 3:
      return "SEEKTABLE";
    case 4:
      return "VORBIS_COMMENT";
    case 5:
      return "CUESHEET";
    case 6:
      return "PICTURE";
    default:
      return "BLOCK " + std::to_string(type);
  }
}

std::string AnalyzeFlacToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"FLAC\",\n";
  out << std::string(indent + 2, ' ') << "\"metadata_blocks\": [\n";
  std::size_t offset = 4;
  std::size_t count = 0;
  bool last = false;
  while (offset + 4 <= data.size() && count < 64 && !last) {
    const auto block_offset = offset;
    const auto header = data[offset];
    last = (header & 0x80) != 0;
    const auto type = static_cast<std::uint32_t>(header & 0x7f);
    const auto payload_length = ReadBe24(data, offset + 1);
    const auto payload_offset = offset + 4;
    if (payload_offset + payload_length > data.size()) {
      break;
    }
    if (count != 0) {
      out << ",\n";
    }
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (count + 1)
        << ", \"offset\": " << block_offset
        << ", \"length\": " << (payload_length + 4)
        << ", \"payload_offset\": " << payload_offset
        << ", \"payload_length\": " << payload_length
        << ", \"last\": " << (last ? "true" : "false")
        << ", \"block_type\": " << type
        << ", \"block_name\": \"" << JsonEscape(FlacBlockTypeName(type)) << "\"";
    if (type == 0 && payload_length >= 34) {
      const auto sample_rate = (static_cast<std::uint32_t>(data[payload_offset + 10]) << 12) |
                               (static_cast<std::uint32_t>(data[payload_offset + 11]) << 4) |
                               ((data[payload_offset + 12] >> 4) & 0x0f);
      const auto channels = static_cast<std::uint32_t>(((data[payload_offset + 12] >> 1) & 0x07) + 1);
      const auto bits_per_sample = static_cast<std::uint32_t>(
          (((data[payload_offset + 12] & 0x01) << 4) | ((data[payload_offset + 13] >> 4) & 0x0f)) + 1);
      const auto total_samples = (static_cast<std::uint64_t>(data[payload_offset + 13] & 0x0f) << 32) |
                                 (static_cast<std::uint64_t>(data[payload_offset + 14]) << 24) |
                                 (static_cast<std::uint64_t>(data[payload_offset + 15]) << 16) |
                                 (static_cast<std::uint64_t>(data[payload_offset + 16]) << 8) |
                                 static_cast<std::uint64_t>(data[payload_offset + 17]);
      out << ", \"min_block_size\": " << ReadBe16(data, payload_offset)
          << ", \"max_block_size\": " << ReadBe16(data, payload_offset + 2)
          << ", \"min_frame_size\": " << ReadBe24(data, payload_offset + 4)
          << ", \"max_frame_size\": " << ReadBe24(data, payload_offset + 7)
          << ", \"sample_rate\": " << sample_rate
          << ", \"channels\": " << channels
          << ", \"bits_per_sample\": " << bits_per_sample
          << ", \"total_samples\": " << total_samples
          << ", \"md5\": \"" << JsonEscape(BytesToHexAt(data, payload_offset + 18, 16)) << "\"";
    }
    out << "}";
    offset += payload_length + 4;
    ++count;
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"metadata_block_count\": " << count << "\n";
  out << std::string(indent, ' ') << "}";
  return out.str();
}

std::size_t Id3TagLength(const std::vector<std::uint8_t>& data) {
  if (data.size() < 10 || std::string_view(reinterpret_cast<const char*>(&data[0]), 3) != "ID3") {
    return 0;
  }
  const auto size = ((data[6] & 0x7f) << 21) |
                    ((data[7] & 0x7f) << 14) |
                    ((data[8] & 0x7f) << 7) |
                    (data[9] & 0x7f);
  return std::min<std::size_t>(data.size(), 10 + size);
}

std::string MpegAudioVersionName(std::uint32_t version_id) {
  switch (version_id) {
    case 0:
      return "MPEG 2.5";
    case 2:
      return "MPEG 2";
    case 3:
      return "MPEG 1";
    default:
      return "reserved";
  }
}

std::string MpegAudioLayerName(std::uint32_t layer_bits) {
  switch (layer_bits) {
    case 1:
      return "Layer III";
    case 2:
      return "Layer II";
    case 3:
      return "Layer I";
    default:
      return "reserved";
  }
}

std::string ChannelModeName(std::uint32_t mode) {
  switch (mode) {
    case 0:
      return "Stereo";
    case 1:
      return "Joint stereo";
    case 2:
      return "Dual channel";
    case 3:
      return "Single channel";
    default:
      return "unknown";
  }
}

std::uint32_t MpegAudioBitrateKbps(std::uint32_t version_id,
                                   std::uint32_t layer_bits,
                                   std::uint32_t index) {
  if (index == 0 || index == 15 || layer_bits == 0 || version_id == 1) {
    return 0;
  }
  static constexpr std::array<std::uint32_t, 16> kMpeg1LayerI =
      {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0};
  static constexpr std::array<std::uint32_t, 16> kMpeg1LayerII =
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0};
  static constexpr std::array<std::uint32_t, 16> kMpeg1LayerIII =
      {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
  static constexpr std::array<std::uint32_t, 16> kMpeg2LayerI =
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0};
  static constexpr std::array<std::uint32_t, 16> kMpeg2LayerIIIII =
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
  if (version_id == 3) {
    if (layer_bits == 3) {
      return kMpeg1LayerI[index];
    }
    if (layer_bits == 2) {
      return kMpeg1LayerII[index];
    }
    return kMpeg1LayerIII[index];
  }
  if (layer_bits == 3) {
    return kMpeg2LayerI[index];
  }
  return kMpeg2LayerIIIII[index];
}

std::uint32_t MpegAudioSampleRate(std::uint32_t version_id, std::uint32_t index) {
  if (index >= 3) {
    return 0;
  }
  static constexpr std::array<std::uint32_t, 3> kMpeg1 = {44100, 48000, 32000};
  static constexpr std::array<std::uint32_t, 3> kMpeg2 = {22050, 24000, 16000};
  static constexpr std::array<std::uint32_t, 3> kMpeg25 = {11025, 12000, 8000};
  if (version_id == 3) {
    return kMpeg1[index];
  }
  if (version_id == 2) {
    return kMpeg2[index];
  }
  if (version_id == 0) {
    return kMpeg25[index];
  }
  return 0;
}

std::string AnalyzeMp3ToJson(const std::vector<std::uint8_t>& data, int indent) {
  std::ostringstream out;
  const auto id3_length = Id3TagLength(data);
  out << "{\n";
  out << std::string(indent + 2, ' ') << "\"format\": \"MP3\",\n";
  out << std::string(indent + 2, ' ') << "\"codec\": \"MPEG Audio\"";
  if (id3_length > 0) {
    out << ",\n" << std::string(indent + 2, ' ') << "\"id3\": {"
        << "\"offset\": 0"
        << ", \"length\": " << id3_length
        << ", \"version_major\": " << static_cast<int>(data[3])
        << ", \"version_revision\": " << static_cast<int>(data[4])
        << ", \"flags\": " << static_cast<int>(data[5])
        << "}";
  }
  out << ",\n" << std::string(indent + 2, ' ') << "\"frames\": [\n";
  std::size_t offset = id3_length;
  std::size_t count = 0;
  while (offset + 4 <= data.size() && count < 64) {
    if (data[offset] != 0xff || (data[offset + 1] & 0xe0) != 0xe0) {
      ++offset;
      continue;
    }
    const auto version_id = static_cast<std::uint32_t>((data[offset + 1] >> 3) & 0x03);
    const auto layer_bits = static_cast<std::uint32_t>((data[offset + 1] >> 1) & 0x03);
    const bool protection_absent = (data[offset + 1] & 0x01) != 0;
    const auto bitrate_index = static_cast<std::uint32_t>((data[offset + 2] >> 4) & 0x0f);
    const auto sample_rate_index = static_cast<std::uint32_t>((data[offset + 2] >> 2) & 0x03);
    const auto padding = static_cast<std::uint32_t>((data[offset + 2] >> 1) & 0x01);
    const auto channel_mode = static_cast<std::uint32_t>((data[offset + 3] >> 6) & 0x03);
    const auto bitrate = MpegAudioBitrateKbps(version_id, layer_bits, bitrate_index);
    const auto sample_rate = MpegAudioSampleRate(version_id, sample_rate_index);
    if (bitrate == 0 || sample_rate == 0 || layer_bits == 0 || version_id == 1) {
      break;
    }
    std::size_t frame_length = 0;
    if (layer_bits == 3) {
      frame_length = ((12 * bitrate * 1000) / sample_rate + padding) * 4;
    } else if (layer_bits == 1 && version_id != 3) {
      frame_length = (72 * bitrate * 1000) / sample_rate + padding;
    } else {
      frame_length = (144 * bitrate * 1000) / sample_rate + padding;
    }
    if (frame_length < 4) {
      break;
    }
    if (offset + frame_length > data.size()) {
      frame_length = data.size() - offset;
    }
    if (count != 0) {
      out << ",\n";
    }
    out << std::string(indent + 4, ' ') << "{"
        << "\"index\": " << (count + 1)
        << ", \"offset\": " << offset
        << ", \"length\": " << frame_length
        << ", \"version\": \"" << JsonEscape(MpegAudioVersionName(version_id)) << "\""
        << ", \"layer\": \"" << JsonEscape(MpegAudioLayerName(layer_bits)) << "\""
        << ", \"bitrate_kbps\": " << bitrate
        << ", \"sample_rate\": " << sample_rate
        << ", \"padding\": " << padding
        << ", \"channel_mode\": \"" << JsonEscape(ChannelModeName(channel_mode)) << "\""
        << ", \"protected_by_crc\": " << (protection_absent ? "false" : "true")
        << "}";
    offset += frame_length;
    ++count;
  }
  out << "\n" << std::string(indent + 2, ' ') << "],\n";
  out << std::string(indent + 2, ' ') << "\"frame_count\": " << count << "\n";
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
  } else if (detection.format == "ogg") {
    out << ",\n";
    out << "  \"container\": " << AnalyzeOggToJson(data, 2) << "\n";
  } else if (detection.format == "flac") {
    out << ",\n";
    out << "  \"container\": " << AnalyzeFlacToJson(data, 2) << "\n";
  } else if (detection.format == "mp3") {
    out << ",\n";
    out << "  \"bitstream\": " << AnalyzeMp3ToJson(data, 2) << "\n";
  } else if (detection.format == "adts-aac") {
    out << ",\n";
    out << "  \"bitstream\": " << AnalyzeAdtsToJson(data, 2) << "\n";
  } else if (detection.format == "h264-annex-b") {
    out << ",\n";
    out << "  \"bitstream\": " << AnalyzeAnnexBToJson(data, 2, false) << "\n";
  } else if (detection.format == "hevc-annex-b") {
    out << ",\n";
    out << "  \"bitstream\": " << AnalyzeAnnexBToJson(data, 2, true) << "\n";
  } else if (detection.format == "av1-obu") {
    out << ",\n";
    out << "  \"bitstream\": " << AnalyzeAv1ObuToJson(data, 2) << "\n";
  } else if (detection.format == "opus-head") {
    out << ",\n";
    out << "  \"bitstream\": " << AnalyzeOpusHeadToJson(data, 2) << "\n";
  } else {
    out << "\n";
  }

  out << "}\n";
  return out.str();
}

}  // namespace media_analyzer
