#pragma once

#include <iosfwd>
#include <vector>
#include <variant>

namespace doom::testing_utils {
class HeapState;
}

std::ostream &operator<<(std::ostream &os,
                         doom::testing_utils::HeapState const &heap);

namespace doom::testing_utils {
struct HeapBlock {
  std::ptrdiff_t block;
  std::size_t size;
  ptrdiff_t user;
  int tag;
};

struct Alloc {
  std::size_t size;
  int tag;
  std::ptrdiff_t user;
  std::ptrdiff_t result;
};

struct Dealloc {
  std::ptrdiff_t ptr;
};

struct TagChange {
    std::ptrdiff_t ptr;
    int tag;
};

struct UserChange {
    std::ptrdiff_t ptr;
    std::ptrdiff_t user;
};

class HeapState {
  char *m_base;
  std::size_t m_size;
  std::vector<std::variant<Alloc, Dealloc, TagChange, UserChange>> m_memOps;
  std::vector<HeapBlock> m_blocks;

public:
  void recordMetadata(char *base, std::size_t size) {
    m_base = base;
    m_size = size;
  }

  void recordAllocation(std::size_t size, int tag, void **user, void *result) {
    m_memOps.push_back(Alloc{size, tag, userPtr(user), toOffset(result)});
  }

  void recordDeallocation(void *ptr) {
      m_memOps.push_back(Dealloc{toOffset(ptr)});
  }

  void recordTagChange(void *ptr, int tag) {
      m_memOps.push_back(TagChange{toOffset(ptr), tag});
  }

  void recordUserChange(void *ptr, void **user) {
      m_memOps.push_back(UserChange{toOffset(ptr), userPtr(user)});
  }

  void recordBlock(void *block, std::size_t size, void **user, int tag) {

    m_blocks.push_back({toOffset(block), size, userPtr(user), tag});
  }

  friend std::ostream & ::operator<<(std::ostream &os, HeapState const &heap);

private:
  std::ptrdiff_t toOffset(void *ptr) {
    return static_cast<char *>(ptr) - m_base;
  }

  std::ptrdiff_t userPtr(void **user) { return user ? 1 : 0; }
};

extern HeapState heap_state;
} // namespace doom::testing_utils