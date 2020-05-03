#ifndef CRISPY_DOOM_MEMORY_HPP
#define CRISPY_DOOM_MEMORY_HPP

#include <new>
#include <cstdlib>
#include "../src/z_zone.hpp"

// todo fix me
template<typename DataType>
auto create_struct()
{
    auto *mem = malloc(sizeof(DataType));
    return new (mem) DataType{};
}

template <typename DataType> auto create_struct(const std::size_t size) {
  auto *mem = malloc(sizeof(DataType) * size);
  return static_cast<DataType *>(new (mem) DataType[size]);
}

template<typename DataType>
auto zmalloc(int size, int tag, void *ptr)
{
  return static_cast<DataType>(Z_Malloc(size, tag, ptr));
}

template <typename DataType>
auto zmalloc_one(int tag, void* ptr = nullptr)
{
    return static_cast<DataType*>(Z_Malloc(sizeof(DataType), tag, ptr));
}

template <typename DataType, typename ... Args>
auto znew(Args... args)
{
    auto* ptr = zmalloc_one<DataType>(PU_LEVSPEC);
    return new (ptr) DataType{args...};
}

template <typename DataType>
void zdelete(DataType* value)
{
    value.~DataType();
    Z_Free(value);
}


#endif // CRISPY_DOOM_MEMORY_HPP
