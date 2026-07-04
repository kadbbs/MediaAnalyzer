#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace media_analyzer {

struct StructureNode {
  std::string type;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
  std::uint64_t header_size = 0;
  std::string hex_preview;
  std::string ascii_preview;
  std::uint64_t preview_offset = 0;
  std::uint64_t preview_length = 0;
  bool preview_truncated = false;
  std::vector<StructureNode> children;
};

struct RationalTime {
  std::uint64_t value = 0;
  std::uint32_t timescale = 0;
};

struct CodecInfo {
  std::string fourcc;
  std::string description;
  std::string profile;
  std::string level;
  std::optional<std::uint32_t> width;
  std::optional<std::uint32_t> height;
  std::optional<std::uint32_t> length_size;
  std::optional<std::uint32_t> sps_count;
  std::optional<std::uint32_t> pps_count;
  std::string raw_header_hex;
  std::string sps_hex;
  std::string pps_hex;
};

struct TrackInfo {
  std::uint32_t id = 0;
  std::string type = "unknown";
  RationalTime duration;
  std::optional<std::uint32_t> width;
  std::optional<std::uint32_t> height;
  std::optional<std::uint32_t> channel_count;
  std::optional<std::uint32_t> sample_rate;
  std::uint32_t sample_count = 0;
  std::uint32_t sample_description_count = 0;
  CodecInfo codec;
};

struct IsoBmffAnalysis {
  std::string major_brand;
  std::uint32_t minor_version = 0;
  std::vector<std::string> compatible_brands;
  RationalTime duration;
  std::vector<StructureNode> structure;
  std::vector<TrackInfo> tracks;
  std::vector<std::string> warnings;
};

std::optional<IsoBmffAnalysis> ParseIsoBmff(std::span<const std::uint8_t> data);
std::string IsoBmffAnalysisToJson(const IsoBmffAnalysis& analysis, int indent = 2);

}  // namespace media_analyzer
