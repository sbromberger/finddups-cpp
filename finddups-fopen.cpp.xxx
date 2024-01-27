#include "_deps/cxxopts-src/include/cxxopts.hpp"
#include "_deps/xxhash_cpp-src/include/xxhash.hpp"
#include "dehumanize.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <climits>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <unordered_map>

constexpr unsigned long BUFSZ = 4096UL;

struct Config {
  unsigned long min_sz;
  unsigned long max_sz;
  std::vector<std::string_view> includes;
  std::vector<std::string_view> excludes;
};

std::optional<Config> get_config(const cxxopts::ParseResult &result) {

  // std::cout << "result min = " << result["min"].as<std::string>() << "\n";
  // std::cout << "result max = " << result["max"].as<std::string>() << "\n";
  auto min_sz_opt = finddups::dehumanize(result["min"].as<std::string>());
  auto max_sz_opt = finddups::dehumanize(result["max"].as<std::string>());
  if (!min_sz_opt.has_value()) {
    std::cerr << "Parsing error: invalid minimum value "
              << result["min"].as<std::string>();
    return std::nullopt;
  };
  if (!max_sz_opt.has_value()) {
    std::cerr << "Parsing error: invalid maximum value "
              << result["max"].as<std::string>();
  };

  auto min_sz = min_sz_opt.value_or(0);
  auto max_sz = max_sz_opt.value_or(0);
  if (max_sz < min_sz) {
    std::cerr << "Parsing error: Maximum file size (" << max_sz
              << ") must be at least minimum file size (" << min_sz << ").\n";
    return std::nullopt;
  };

  return Config{min_sz, max_sz};
}

std::ostream &operator<<(std::ostream &os, const Config &cfg) {
  os << "min size: " << cfg.min_sz << ", max size: " << cfg.max_sz
     << ", includes: [";
  std::for_each(cfg.includes.begin(), cfg.includes.end(),
                [&os](const std::string_view el) { os << el << ", "; });

  os << "], excludes: [";
  std::for_each(cfg.excludes.begin(), cfg.excludes.end(),
                [&os](const std::string_view el) { os << el << ", "; });
  os << "]";
  return os;
}

using entry_size_map =
    std::unordered_map<uintmax_t,
                       std::vector<std::filesystem::directory_entry>>;
using entry_hash_map =
    std::unordered_map<xxh::hash64_t,
                       std::vector<std::filesystem::directory_entry>>;

entry_size_map sizemap(const std::string &dir, const Config &cfg) {
  entry_size_map sz{};
  using recursive_directory_iter =
      std::filesystem::recursive_directory_iterator;
  for (const auto &dirEntry : recursive_directory_iter(dir)) {

    if (!dirEntry.is_regular_file()) {
      continue;
    }
    uintmax_t fsize = std::filesystem::file_size(dirEntry);
    if ((fsize < cfg.min_sz) || (fsize > cfg.max_sz)) {
      continue;
    }
    if (!sz.contains(fsize)) {
      sz[fsize] = std::vector<std::filesystem::directory_entry>{};
    }
    sz.at(fsize).emplace_back(dirEntry.path());
  }

  return sz;
}
entry_hash_map hashmap(const entry_size_map &sizemap, const Config &cfg) {
  entry_hash_map hm{};
  std::unique_ptr<std::array<char, BUFSZ>> buffer(new (std::align_val_t(BUFSZ))
                                                      std::array<char, BUFSZ>);
  for (const auto &size_entry : sizemap) {
    auto poss_dups = size_entry.second;
    for (const auto &f : poss_dups) {
      // std::cout << "opening " << f.path() << std::endl;
      auto *f_handle = fopen(f.path().c_str(), "r");
      if (f_handle == NULL) { // file couldn't be opened
        std::cerr << "Error opening " << f << ": " << strerror(errno)
                  << "; skipping\n";
        continue;
      }
      xxh::hash_state_t<64> hstate{};
      auto n_read = fread(buffer->data(), sizeof(char), BUFSZ, f_handle);
      while (n_read != 0) {
        // std::cout << "  n_read = " << n_read
        // << ", buffer->size() = " << buffer->size() << std::endl;
        hstate.update(buffer->data(), n_read * sizeof(char));
        // std::memset(buffer->data(), 0, BUFSZ);
        // std::cout << "post-memset" << std::endl;
        n_read = fread(buffer->data(), sizeof(char), BUFSZ, f_handle);
      }

      if (feof(f_handle) != 1) {
        std::cerr << "Error reading " << f << ": " << strerror(errno)
                  << "; skipping\n";
        fclose(f_handle);
        continue;
      }
      fclose(f_handle);
      auto hash = hstate.digest();
      // std::cout << "closing " << f.path() << " with hash " << hash <<
      // std::endl;
      if (!hm.contains(hash)) {
        hm[hash] = std::vector<std::filesystem::directory_entry>{};
      } // else {
        //  std::cout << "adding second hash " << f << " with hash " << hash
        //  << " to map" << std::endl;
      // }
      hm.at(hash).emplace_back(f.path());
    }
  }
  return hm;
};

int main(int argc, char *argv[]) {
  auto start = std::chrono::high_resolution_clock::now();
  auto opts = cxxopts::Options{"finddups", "recursively find duplicate files"};
  opts.add_options()

      ("min", "Minimum file size (in bytes) to include",
       cxxopts::value<std::string>()->default_value("0"))(
          "max", "Maximum file size (in bytes) to include",
          cxxopts::value<std::string>()->default_value(
              std::to_string(LONG_MAX)));

  cxxopts::ParseResult result;
  try {
    result = opts.parse(argc, argv);
  } catch (cxxopts::exceptions::parsing e) {
    std::cerr << "Parsing error: " << e.what() << "\n";
    exit(1);
  }

  auto cfg_opt = get_config(result);
  if (!cfg_opt.has_value()) {
    std::cerr << "Invalid configuration found\n";
    exit(1);
  }
  auto cfg = std::move(cfg_opt.value());
  std::cout << cfg << "\n";

  std::unordered_multimap<xxh::hash_t<64>, std::string> dupsmap{};

  auto sizes = sizemap(".", cfg);
  std::cout << "sizemap size = " << sizes.size() << "\n";
  // for (const auto &entries : sizes) {
  //   if (entries.second.size() > 1) {
  //     std::cout << "[ ";
  //     for (const auto &pathname : entries.second) {
  //       std::cout << pathname << " ";
  //     }
  //     std::cout << "]\n";
  //   }
  // }
  std::cout << "finished sizemap" << std::endl;
  auto hashes = hashmap(sizes, cfg);
  std::cout << "hashmap size = " << hashes.size() << "\n";

  for (const auto &entries : hashes) {
    if (entries.second.size() > 1) {
      std::cout << "[ ";
      for (const auto &pathname : entries.second) {
        std::cout << pathname << " ";
      }
      std::cout << "]\n";
    }
  }
  std::cerr << "Total time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - start)
            << "\n";
  return 0;
}
