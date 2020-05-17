#include "lump_ext.h"
#include "lump_testing.hpp"

#include <iostream>
#include <cstring>

using namespace std::string_view_literals;

namespace {
std::string_view tag_to_sv(int tag) {
  switch (tag) {
  case 1:
    return "PU_STATIC"sv;
  case 2:
    return "PU_SOUND"sv;
  case 3:
    return "PU_MUSIC"sv;
  case 4:
    return "PU_FREE"sv;
  case 5:
    return "PU_LEVEL"sv;
  case 6:
    return "PU_LEVSPEC"sv;
  case 7:
    return "PU_PURGELEVEL"sv;
  case 8:
    return "PU_CACHE"sv;
  default:
    throw std::runtime_error("unsupported tag");
  }
}

std::string_view lump_name_to_sv(char const* name)
{
    auto len = std::strlen(name);
    return std::string_view {name, std::min(8ul, len)};
}
} // namespace

namespace doom::testing_utils {
LumpOps lump_ops;
} // namespace doom::testing_utils

extern "C" void RecordCacheRequest(char const *lump_name, int lump_index,
                                   int tag) {
  doom::testing_utils::lump_ops.recordCache(false, lump_name_to_sv(lump_name), lump_index, tag);
}

extern "C" void RecordAlreadyCached(char const *lump_name, int lump_index,
                                    int tag) {
  doom::testing_utils::lump_ops.recordCache(true, lump_name_to_sv(lump_name), lump_index, tag);
}

extern "C" void RemoveFromCache(char const *lump_name, int lump_index) {
  doom::testing_utils::lump_ops.removeFromCache(lump_name_to_sv(lump_name), lump_index);
}

std::ostream &operator<<(std::ostream &os, doom::testing_utils::LumpOps const &ops)
{
  for (auto const &op : ops.ops) {
    if (op.index() == 0) {
      auto const &request = std::get<0>(op);
      os << "Caching " << request.name << " index: " << request.index
         << " cached: " << (request.was_in_cache ? "yes" : "no")
         << " tag: " << tag_to_sv(request.tag) << "\n";
    } else if (op.index() == 1) {
      auto const &removal = std::get<1>(op);
      os << "Removing " << removal.name << " index: " << removal.index << "\n";
    }
  }
  return os;
}