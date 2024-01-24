#include <optional>
#include <string>

namespace finddups {
std::optional<unsigned long int> dehumanize(std::string s) {
  if (s.empty()) {
    return std::nullopt;
  }

  auto last_char = s.back();
  unsigned long multiplier{1};
  switch (last_char) {
  case 't':
  case 'T':
    multiplier = 1024UL * 1024 * 1024 * 1024;
    s.pop_back();
    break;
  case 'g':
  case 'G':
    multiplier = 1024UL * 1024 * 1024;
    s.pop_back();
    break;
  case 'm':
  case 'M':
    multiplier = 1024UL * 1024UL;
    s.pop_back();
    break;
  case 'k':
  case 'K':
    multiplier = 1024UL;
    s.pop_back();
    break;
  }

  unsigned long n{};
  try {
    n = std::stoul(s);
  } catch (...) {
    return std::nullopt;
  }
  return n * multiplier;
}
} // namespace finddups
