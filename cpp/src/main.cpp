#include "media_analyzer/analysis.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kDefaultReadLimit = 256ull * 1024ull * 1024ull;

void PrintUsage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " <media-file> [read-limit-bytes]\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    PrintUsage(argv[0]);
    return 2;
  }

  const std::string path = argv[1];
  std::size_t read_limit = kDefaultReadLimit;
  if (argc == 3) {
    try {
      read_limit = static_cast<std::size_t>(std::stoull(argv[2]));
    } catch (...) {
      std::cerr << "Invalid read limit: " << argv[2] << "\n";
      return 2;
    }
  }

  std::ifstream input(path, std::ios::binary);
  if (!input) {
    std::cerr << "Failed to open file: " << path << "\n";
    return 1;
  }

  std::vector<std::uint8_t> data;
  data.reserve(read_limit);

  char ch = 0;
  while (data.size() < read_limit && input.get(ch)) {
    data.push_back(static_cast<std::uint8_t>(ch));
  }

  std::cout << media_analyzer::AnalyzeFileToJson(
      data, media_analyzer::AnalyzeOptions{.name_hint = path});
  return 0;
}
