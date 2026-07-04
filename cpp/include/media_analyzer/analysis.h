#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace media_analyzer {

struct AnalyzeOptions {
  std::string name_hint;
};

std::string AnalyzeFileToJson(const std::vector<std::uint8_t>& data,
                              const AnalyzeOptions& options);

}  // namespace media_analyzer

