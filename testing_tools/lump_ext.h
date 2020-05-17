#ifndef CPP_DOOM_LUMP_EXT_H
#define CPP_DOOM_LUMP_EXT_H

#ifdef __cplusplus
extern "C" {
#endif
void RecordCacheRequest(char const* lump_name, int lump_index, int tag);
void RecordAlreadyCached(char const* lump_name, int lump_index, int tag);
void RemoveFromCache(char const* lump_name, int lump_index);

#ifdef __cplusplus
}
#endif

#endif //CPP_DOOM_LUMP_EXT_H
