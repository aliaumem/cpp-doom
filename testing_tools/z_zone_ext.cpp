#include "z_zone_ext.h"
#include "zone_testing.hpp"
#include <iostream>

namespace doom::testing_utils {
HeapState heap_state;
}

using namespace doom::testing_utils;

namespace {
std::ostream &operator<<(std::ostream &os, Alloc const &alloc) {
  return os << "Alloc - size: " << alloc.size << "\ttag: " << alloc.tag
            << "\tuser: " << alloc.user << "\tresult: " << alloc.result << "\n";
}

std::ostream &operator<<(std::ostream &os, Dealloc const &dealloc) {
  return os << "Dealloc - ptr: " << dealloc.ptr << "\n";
}

std::ostream &operator<<(std::ostream &os, TagChange const &tagChange) {
  return os << "Tag change - ptr: " << tagChange.ptr
            << "\ttag: " << tagChange.tag << "\n";
}

std::ostream &operator<<(std::ostream &os, UserChange const &userChange) {
  return os << "User change - ptr: " << userChange.ptr
            << "\tuser: " << userChange.user << "\n";
}

} // namespace

std::ostream &operator<<(std::ostream &os, HeapState const &heap) {
  os << "Operations\n";
  for (auto const &op : heap.m_memOps) {
    std::visit([&os](auto const &val) { os << val; }, op);
  }

  os << "heap size: " << heap.m_size << "\n";
  for (auto const &block : heap.m_blocks) {
    os << "block: " << block.block << " size: " << block.size
       << " user: " << block.user << " tag: " << block.tag << "\n";
  }
  return os;
}

extern "C" {
void RecordHeapMetadata(void *base, size_t size) {
  heap_state.recordMetadata(static_cast<char *>(base), size);
}

void RecordHeapBlock(void *block, size_t size, void **user, int tag) {
  heap_state.recordBlock(block, size, user, tag);
}

void RecordZMalloc(int size, int tag, void **user, void *result) {
  heap_state.recordAllocation(static_cast<size_t>(size), tag, user, result);
}

void RecordZFree(void *ptr) { heap_state.recordDeallocation(ptr); }

void RecordChangeTag(void *ptr, int tag) {
  heap_state.recordTagChange(ptr, tag);
}

void RecordChangeUser(void *ptr, void **user) {
  doom::testing_utils::heap_state.recordUserChange(ptr, user);
}
}