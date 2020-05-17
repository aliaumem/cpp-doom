#pragma once
#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

namespace doom::testing_utils
{
    class LumpOps;
}

std::ostream & operator<<(std::ostream& os, doom::testing_utils::LumpOps const& ops);


namespace doom::testing_utils {
struct cache_request {
  bool was_in_cache;
  std::string name;
  int index;
  int tag;
};

struct cache_removal {
  std::string name;
  int index;
};

class LumpOps {
  std::vector<std::variant<cache_request, cache_removal>> ops;

public:
  void recordCache(bool inCache, std::string_view name, int index, int tag) {
    ops.push_back(cache_request{inCache, {name.data(), name.size()}, index, tag});
  }

  void removeFromCache(std::string_view name, int index) {
    ops.push_back(cache_removal{{name.data(), name.size()}, index});
  }

  friend std::ostream &::operator<<(std::ostream &os, LumpOps const &ops);
};


extern LumpOps lump_ops;
} // namespace doom::testing