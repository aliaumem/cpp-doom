//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Zone Memory Allocation. Neat.
//

#include <cstring>
#include <cassert>

#include "doomtype.hpp"
#include "i_system.hpp"
#include "m_argv.hpp"

#include "z_zone.hpp"
#include "z_zone_ext.h"

#include "zone/memzone.hpp"

#include <new>
#include <iterator>

//
// ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
//

using namespace doom::zone;

//
// Z_ClearZone
//
void Z_ClearZone (memzone_t* zone)
{
    // set the entire zone to one free block
    *zone = memzone_t{zone->size};
}



//
// Z_Init
//
void Z_Init () {
    int size;
    auto* memory_zone = reinterpret_cast<std::byte*>(I_ZoneBase(&size));
    memory_resource::init(memory_zone, size);
}

void memory_resource::init(std::byte *zone, std::size_t size)
{
    mem_res = memory_resource(zone, size);

    // [Deliberately undocumented]
    // Zone memory debugging flag. If set, memory is zeroed after it is freed
    // to deliberately break any code that attempts to use it after free.
    //
    mem_res.zero_on_free = M_ParmExists("-zonezero");

    // [Deliberately undocumented]
    // Zone memory debugging flag. If set, each time memory is freed, the zone
    // heap is scanned to look for remaining pointers to the freed block.
    //
    mem_res.scan_on_free = M_ParmExists("-zonescan");

    RecordHeapMetadata(reinterpret_cast<char *>(&mem_res->blocklist),
                       mem_res->size);
}

void Z_InitMem(std::byte* zone, std::size_t size)
{
    memory_resource::init(zone, size);
}
//
// Z_Free
//
void Z_Free (void* ptr) {
    auto block = memzone_t::iterator_at(ptr);
    mem_res->free_block(block);
}

//
// Z_Malloc
// You can pass a NULL user if the tag can't be purged.
//
void* Z_Malloc(int size,
    purge_tags     tag,
    void*          user)
{
    return mem_res->allocate(size, tag, user);
}

//
// Z_FreeTags
//
void
Z_FreeTags
( purge_tags lowtag,
  purge_tags hightag ) {
    mem_res->free_tags(lowtag, hightag);
}

//
// Z_DumpHeap
// Note: TFileDumpHeap( stdout ) ?
//
void Z_DumpHeap(int lowtag, int hightag) {

  printf("zone size: %i  location: %p\n", mem_res->size, mem_res.operator->());

  printf("tag range: %i to %i\n", lowtag, hightag);

  for (auto block = mem_res.begin();; ++block) {
    if (block->tag >= lowtag && block->tag <= hightag)
      printf("block:%p    size:%7i    user:%p    tag:%3i id:%d\n", block,
             block->size, block->user, block->tag, block->id);
    RecordHeapBlock(*block, block->size, block->user, block->tag);

    if (block.next() == mem_res.end()) {
      // all blocks have been hit
      break;
    }

    if (reinterpret_cast<byte *>(*block) + block->size != reinterpret_cast<byte*>(*(block.next())))
      printf("ERROR: block size does not touch the next block\n");

    if (block.next().prev() != block)
      printf("ERROR: next block doesn't have proper back link\n");

    if (block->is_free() && block.next()->is_free())
      printf("ERROR: two consecutive free blocks\n");
  }
}

//
// Z_CheckHeap
//
void Z_CheckHeap(void)
{
    memblock_t* block;

    for (block = mem_res->blocklist.next;; block = block->next)
    {
        if (block->next == &mem_res->blocklist)
        {
            // all blocks have been hit
            break;
        }

        if ((byte*)block + block->size != (byte*)block->next)
            I_Error("Z_CheckHeap: block size does not touch the next block\n");

        if (block->next->prev != block)
            I_Error("Z_CheckHeap: next block doesn't have proper back link\n");

        if (block->tag == PU_FREE && block->next->tag == PU_FREE)
            I_Error("Z_CheckHeap: two consecutive free blocks\n");
    }
}


//
// Z_ChangeTag
//
void Z_ChangeTag2(void *ptr, purge_tags tag, const char *file, int line)
{
    auto block = memzone_t::iterator_at(ptr);

    if (!block->is_valid())
        I_Error("%s:%i: Z_ChangeTag: block without a ZONEID!",
            file, line);

    if (memblock_t::is_purgeable(tag) && !block->has_user())
        I_Error("%s:%i: Z_ChangeTag: an owner is required "
                "for purgeable blocks", file, line);

    block->tag = tag;

    RecordChangeTag(ptr, tag);
}

void Z_ChangeUser(void *ptr, void **user) {
    auto block = memzone_t::iterator_at(ptr);
    block->change_user(user);
}


//
// Z_FreeMemory
//
size_t Z_FreeMemory ()
{
    size_t free = 0;

    for (auto block : mem_res)
    {
        if (block->is_free() || block->is_purgeable())
            free += block->size;
    }

    return free;
}

size_t Z_ZoneSize()
{
    return mem_res->size;
}
