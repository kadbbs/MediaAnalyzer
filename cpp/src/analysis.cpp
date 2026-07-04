#include "media_analyzer/analysis.h"

#include "media_analyzer/format_detector.h"
#include "media_analyzer/iso_bmff.h"

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

}  // namespace

std::string AnalyzeFileToJson(const std::vector<std::uint8_t>& data,
                              const AnalyzeOptions& options) {
  const auto detection = DetectFormat(data, options.name_hint);

  std::ostringstream out;
  out << "{\n";
  out << "  \"input\": {\n";
  out << "    \"name\": \"" << JsonEscape(options.name_hint) << "\",\n";
  out << "    \"size\": " << data.size() << "\n";
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
  } else {
    out << "\n";
  }

  out << "}\n";
  return out.str();
}

}  // namespace media_analyzer
