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

struct ByteRange {
  std::uint64_t offset = 0;
  std::uint64_t length = 0;
};

struct SampleInfo {
  std::uint32_t index = 0;
  std::uint64_t offset = 0;
  std::uint32_t size = 0;
  std::uint64_t dts = 0;
  std::int64_t pts = 0;
  std::uint32_t duration = 0;
  std::int64_t composition_offset = 0;
  bool sync = true;
  std::optional<ByteRange> bytes;
};

struct CodecInfo {
  std::string fourcc;
  std::string description;
  std::string profile;
  std::string level;
  std::optional<std::uint32_t> width;
  std::optional<std::uint32_t> height;
  std::optional<std::uint32_t> length_size;
  std::optional<std::uint32_t> vps_count;
  std::optional<std::uint32_t> sps_count;
  std::optional<std::uint32_t> pps_count;
  std::optional<std::uint32_t> bit_depth_luma;
  std::optional<std::uint32_t> bit_depth_chroma;
  std::optional<std::uint32_t> chroma_format;
  std::optional<std::uint32_t> audio_object_type;
  std::optional<std::uint32_t> asc_sample_rate;
  std::optional<std::uint32_t> channel_config;
  std::string raw_header_hex;
  std::string vps_hex;
  std::string sps_hex;
  std::string pps_hex;
  std::string asc_hex;
  std::optional<ByteRange> raw_header_bytes;
  std::optional<ByteRange> vps_bytes;
  std::optional<ByteRange> sps_bytes;
  std::optional<ByteRange> pps_bytes;
  std::optional<ByteRange> asc_bytes;
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
  bool sample_table_truncated = false;
  std::uint32_t sample_table_total = 0;
  std::vector<SampleInfo> samples;
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
