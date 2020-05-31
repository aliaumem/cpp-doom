#ifndef CPP_DOOM_Z_ZONE_EXT_H
#define CPP_DOOM_Z_ZONE_EXT_H

#ifdef __cplusplus
#include <cstddef>
#else
#include "stddef.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void RecordHeapMetadata(void* base, size_t size);
void RecordHeapBlock(void* block, size_t size, void** user, int tag);
void RecordZMalloc(int size, int tag, void** user, void* result);
void RecordZFree(void* ptr);
void RecordChangeTag(void* ptr, int tag);
void RecordChangeUser(void* ptr, void** user);

#ifdef __cplusplus
}
#endif

#endif //CPP_DOOM_Z_ZONE_EXT_H
