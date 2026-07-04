#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace media_analyzer {

enum class FormatFamily {
  Container,
  Manifest,
  ElementaryStream,
  Unknown,
};

struct FormatDetection {
  std::string format;
  FormatFamily family = FormatFamily::Unknown;
  double confidence = 0.0;
  std::vector<std::string> evidence;
};

FormatDetection DetectFormat(std::span<const std::uint8_t> data,
                             const std::string& name_hint = {});

std::string FormatFamilyToString(FormatFamily family);
std::string DetectionToJson(const FormatDetection& detection);

}  // namespace media_analyzer

