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

static constexpr auto const ZONEID = 0x1d4a11;
static constexpr auto const MINFRAGMENT	= 64;

struct memblock_t {
    memblock_t(size_t size, purge_tags tag, memblock_t* prev, memblock_t* next,
        void** user = nullptr)
        : size { static_cast<int>(size) }
        , user { user }
        , tag { tag }
        , id { ZONEID }
        , next { next }
        , prev { prev }
    {
        next->prev = this;
        prev->next = this;
    }

    memblock_t(size_t size, purge_tags tag, void** user)
        : size { static_cast<int>(size) }
        , user { user }
        , tag { tag }
        , id { ZONEID }
        , next { nullptr }
        , prev { nullptr }
    {
    }

    int         size; // including the header and possibly tiny fragments
    void**      user;
    purge_tags  tag; // PU_FREE if this is free
    int         id;  // should be ZONEID
    memblock_t* next;
    memblock_t* prev;

    [[nodiscard]] bool is_free() const { return tag == purge_tags::PU_FREE; }
    [[nodiscard]] bool static is_purgeable(purge_tags tag) { return tag >= PU_PURGELEVEL; }
    [[nodiscard]] bool is_purgeable() const { return is_purgeable(tag); }
    [[nodiscard]] bool has_user() const { return user != nullptr; }
    [[nodiscard]] bool is_valid() const { return id == ZONEID; }
    [[nodiscard]] bool can_allocate(size_t size) { return is_free() && this->size >= size; }

    void merge_with(memblock_t* other)
    {
        size += other->size;
        next = other->next;
        next->prev = this;

        other->~memblock_t();
    }

    void mark_as_free()
    {
        tag = PU_FREE;
        user = nullptr;
        id = 0;
    }

    [[nodiscard]] void* content() { return const_cast<std::byte*>(reinterpret_cast<std::byte const*>(this) + sizeof(memblock_t)); };
    [[nodiscard]] size_t      content_size() const { return size - sizeof(memblock_t); }
};

struct memzone_t {
    memzone_t(size_t size)
        : size { size }
        , blocklist { size - sizeof(memzone_t),
            PU_STATIC, reinterpret_cast<void**>(
                this) }
        , rover { nullptr }
    {
        auto* firstBlock = new (reinterpret_cast<char*>(this) + sizeof(memzone_t))
            memblock_t { size - sizeof(memzone_t), PU_FREE, &blocklist, &blocklist };
        blocklist.prev = blocklist.next = firstBlock;
        rover = {firstBlock};
    }

    class iterator {
    public:
        using value_type        = memblock_t;
        using reference         = memblock_t*;
        using pointer           = memblock_t*;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator(memblock_t* block)
                : m_block(block)
        {
        }

        iterator& operator++()
        {
            m_block = m_block->next;
            return *this;
        }

        iterator& operator--()
        {
            m_block = m_block->prev;
            return *this;
        }

        iterator next() const { return m_block->next; }
        iterator prev() const { return m_block->prev; }

        pointer   operator->() { return m_block; }
        reference operator*() { return m_block; }
        bool operator==(iterator const& other) const { return m_block == other.m_block; }
        bool operator!=(iterator const& other) const { return !(*this == other); }

    private:
        memblock_t* m_block;
    };

    static iterator iterator_at(void* ptr)
    {
        return { reinterpret_cast<memblock_t*>(reinterpret_cast<std::byte*>(ptr) - sizeof(memblock_t)) };
    }

    void clear() { *this = memzone_t{size}; }

    void free_block(iterator block);
    void* allocate(size_t size, purge_tags tag, void* user);

    // total bytes malloced, including header
    size_t size;

    // start / end cap for linked list
    memblock_t blocklist;

    iterator rover;
};

struct memory_resource {
    memory_resource() noexcept
    : mainzone(nullptr)
    {}

    memory_resource(std::byte* memory_zone, size_t size) noexcept
            : mainzone(new (memory_zone) memzone_t{size})
    {}

    auto begin() -> memzone_t::iterator { return {mainzone->blocklist.next}; }
    auto end() -> memzone_t::iterator { return {&mainzone->blocklist}; }
    auto& rover() { return mainzone->rover; }

    memzone_t* operator ->() const { return mainzone; }

    memzone_t* mainzone;
    bool zero_on_free = false;
    bool scan_on_free = false;
};

static memory_resource mem_res;

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
void Z_InitMem(std::byte* memory_zone, size_t size);

void Z_Init () {
    int size;
    std::byte* memory_zone = reinterpret_cast<std::byte*>(I_ZoneBase(&size));
    Z_InitMem(memory_zone, size);
}
void Z_InitMem(std::byte* memory_zone, size_t size)
{
    mem_res = memory_resource(memory_zone, size);

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

// Scan the zone heap for pointers within the specified range, and warn about
// any remaining pointers.
static void ScanForBlock(void *start, void *end)
{
    for(auto* block : mem_res)
    {
        auto tag = block->tag;

        if (tag == PU_STATIC || tag == PU_LEVEL || tag == PU_LEVSPEC)
        {
            // Scan for pointers on the assumption that pointers are aligned
            // on word boundaries (word size depending on pointer size):
            void** mem = reinterpret_cast<void **>(block->content());
            auto len = block->content_size() / sizeof(void *);

            for (size_t i = 0; i < len; ++i)
            {
                if (start <= mem[i] && mem[i] <= end)
                {
                    fprintf(stderr,
                            "%p has dangling pointer into freed block "
                            "%p (%p -> %p)\n",
                            mem, start, &mem[i], mem[i]);
                }
            }
        }
    }
}

//
// Z_Free
//
void Z_Free (void* ptr) {
    auto block = memzone_t::iterator_at(ptr);
    mem_res->free_block(block);
}

void memzone_t::free_block(iterator block)
{
    if (!block->is_valid())
        I_Error ("Z_Free: freed a pointer without ZONEID (%p)", *block);

    RecordZFree(block->content());

    block->mark_as_free();

    // If the -zonezero flag is provided, we zero out the block on free
    // to break code that depends on reading freed memory.
    if (mem_res.zero_on_free)
        memset(block->content(), 0, block->size - sizeof(memblock_t));

    if (mem_res.scan_on_free)
        ScanForBlock(block->content(), reinterpret_cast<std::byte*>(block->content()) + block->content_size());

    if (auto previous = block.prev(); previous->is_free())
    {
        // merge with previous free block
        previous->merge_with(*block);

        if (rover == block) --rover;

        --block;
    }

    if (auto next = block.next(); next->is_free())
    {
        // merge the next free block onto the end
        block->merge_with(*next);

        if (rover == next)
            ++rover;
    }
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

void* memzone_t::allocate(size_t size, purge_tags tag, void* user)
{
    constexpr auto const MEM_ALIGN = alignof(void*);
    size                           = static_cast<int>((size + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1));
    size += sizeof(memblock_t); // account for size of block header

    // scan through the block list,
    // looking for the first free block
    // of sufficient size,
    // throwing out any purgeable blocks along the way.


    // if there is a free block behind the rover,
    //  back up over them
    auto base = rover;

    if (base.prev()->is_free())
        --base;

    auto new_rover = base;
    auto start = base.prev();

    do
    {
        if (new_rover == start)
        {
            // scanned all the way around the list
            I_Error("Z_Malloc: failed on allocation of %i bytes", size);

            // [crispy] allocate another zone twice as big
            //Z_Init();

            //base  = mem_res.rover();
            //new_rover = base;
            //start = base.prev();
        }

        if (!new_rover->is_free())
        {
            if (!new_rover->is_purgeable())
            {
                // hit a block that can't be purged, so move base past it
                ++base;
                ++new_rover;
            }
            else
            {
                // the rover can be the base block
                --base;
                free_block(new_rover);
                ++base;
                ++new_rover;
            }
        }
        else
            ++new_rover;
    } while (!base->can_allocate(size));


    // found a block big enough
    std::size_t extra = base->size - size;

    if (extra > MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        auto* ptr = reinterpret_cast<std::byte*>(*base) + size;
        new (ptr) memblock_t { extra, PU_FREE, *base, *(base.next()) };

        base->size = size;
    }

    if (!user && memblock_t::is_purgeable(tag))
        I_Error("Z_Malloc: an owner is required for purgeable blocks");

    base->user = reinterpret_cast<void**>(user);
    base->tag  = tag;

    auto* result = base->content();

    if (base->user)
        *base->user = result;

    // next allocation will start looking here
    rover = base.next();

    base->id = ZONEID;

    RecordZMalloc(size, tag, reinterpret_cast<void**>(user), result);

    return result;
}


//
// Z_FreeTags
//
void
Z_FreeTags
( int		lowtag,
  int		hightag )
{
    for (auto block = mem_res.begin(), next = mem_res.begin() ;
	 block != mem_res.end() ;
	 block = next)
    {
	// get link before freeing
	next = block.next();

	// free block?
	if (block->tag == PU_FREE)
	    continue;

	if (block->tag >= lowtag && block->tag <= hightag)
	    Z_Free (block->content());
    }
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

    if (reinterpret_cast<byte *>(*block) + block->size != (byte *)block->next)
      printf("ERROR: block size does not touch the next block\n");

    if (block->next->prev != block)
      printf("ERROR: next block doesn't have proper back link\n");

    if (block->tag == PU_FREE && block->next->tag == PU_FREE)
      printf("ERROR: two consecutive free blocks\n");
  }
}

//
// Z_FileDumpHeap
//
void Z_FileDumpHeap (FILE* f)
{
    memblock_t*	block;

    fprintf (f,"zone size: %i  location: %p\n",mem_res->size,mem_res.operator->());

    for (block = mem_res->blocklist.next ; ; block = block->next)
    {
	fprintf (f,"block:%p    size:%7i    user:%p    tag:%3i\n",
		 block, block->size, block->user, block->tag);

	if (block->next == &mem_res->blocklist)
	{
	    // all blocks have been hit
	    break;
	}

	if ( (byte *)block + block->size != (byte *)block->next)
	    fprintf (f,"ERROR: block size does not touch the next block\n");

	if ( block->next->prev != block)
	    fprintf (f,"ERROR: next block doesn't have proper back link\n");

	if (block->tag == PU_FREE && block->next->tag == PU_FREE)
	    fprintf (f,"ERROR: two consecutive free blocks\n");
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

void Z_ChangeUser(void *ptr, void **user)
{
    auto block = memzone_t::iterator_at(ptr);

    if (!block->is_valid())
        I_Error("Z_ChangeUser: Tried to change user for invalid block!");

    block->user = user;
    *user = ptr;

    RecordChangeUser(ptr, user);
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
