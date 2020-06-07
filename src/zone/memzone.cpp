#include "memzone.hpp"

#include "i_system.hpp"
#include "z_zone_ext.h"

#include <cstring>

namespace doom::zone {

namespace {
    // Scan the zone heap for pointers within the specified range, and warn about
    // any remaining pointers.
    void ScanForBlock(void* start, void* end)
    {
        for (auto* block : mem_res)
        {
            auto tag = block->tag;

            if (tag == PU_STATIC || tag == PU_LEVEL || tag == PU_LEVSPEC)
            {
                // Scan for pointers on the assumption that pointers are aligned
                // on word boundaries (word size depending on pointer size):
                void** mem = reinterpret_cast<void**>(block->content());
                auto   len = block->content_size() / sizeof(void*);

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

    struct first_free_block_finder {
        memzone_t::iterator base, rover, start;
        memzone_t*          memzone;

        first_free_block_finder(memzone_t* memzone, memzone_t::iterator rover)
            // if there is a free block behind the rover, back up over them
            : base(rover.prev()->is_free() ? rover.prev() : rover)
            , rover(base)
            , start(base.prev())
            , memzone(memzone)
        {
        }

        memzone_t::iterator find_first_matching(size_t size)
        {
            // scan through the block list,
            // looking for the first free block
            // of sufficient size,
            // throwing out any purgeable blocks along the way.
            do
            {
                if (rover == start)
                {
                    // scanned all the way around the list
                    I_Error("Z_Malloc: failed on allocation of %i bytes", size);

                    // [crispy] allocate another zone twice as big
                    //Z_Init();

                    //base  = mem_res.rover();
                    //rover = base;
                    //start = base.prev();
                }

                if (!rover->is_free())
                {
                    if (!rover->is_purgeable())
                    {
                        // hit a block that can't be purged, so move base past it
                        ++base;
                        ++rover;
                    }
                    else
                    {
                        // the rover can be the base block
                        --base;
                        memzone->free_block(rover);
                        ++base;
                        ++rover;
                    }
                }
                else
                    ++rover;
            } while (!base->can_allocate(size));
            return base;
        }
    };
}


void* memzone_t::allocate(size_t size, purge_tags tag, void* user)
{
    constexpr auto const MEM_ALIGN = alignof(void*);
    size                           = static_cast<int>((size + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1));
    size += sizeof(memblock_t); // account for size of block header

    first_free_block_finder finder { this, rover };
    iterator                base = finder.find_first_matching(size);

    constexpr auto const MINFRAGMENT = 64;

    // found a block big enough
    if (std::size_t extra = base->size - size; extra > MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        auto* ptr = reinterpret_cast<std::byte*>(*base) + size;
        new (ptr) memblock_t { extra, PU_FREE, *base, *(base.next()) };
    }
    else
        size = base->size;

    base->reallocate_to(size, tag, reinterpret_cast<void**>(user));

    // next allocation will start looking here
    rover = base.next();

    RecordZMalloc(size, tag, reinterpret_cast<void**>(user), base->content());

    return base->content();
}

void memzone_t::free_block(iterator block)
{
    if (!block->is_valid())
    {
        Z_DumpHeap(PU_STATIC, PU_NUM_TAGS);
        I_Error("Z_Free: freed a pointer without ZONEID (%p)", *block);
    }

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


void memzone_t::free_tags(purge_tags lowtag, purge_tags hightag)
{
    for (auto* block : *this)
    {
        if (!block->is_free() && block->is_tag_between(lowtag, hightag))
            free_block(iterator { block });
    }
}
}