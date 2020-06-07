#pragma once

#include "z_zone.hpp"

#include "i_system.hpp"

namespace doom::zone {
static constexpr auto const ZONEID = 0x1d4a11;

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
    [[nodiscard]] bool can_allocate(size_t size) const { return is_free() && this->size >= size; }
    [[nodiscard]] bool is_tag_between(purge_tags lowtag, purge_tags hightag) const
    {
        return tag >= lowtag && tag <= hightag;
    }

    void merge_with(memblock_t* other)
    {
        size += other->size;
        next       = other->next;
        next->prev = this;

        other->~memblock_t();
    }

    void mark_as_free()
    {
        tag = PU_FREE;
        if (user) *user = nullptr;
        user = nullptr;
        id   = 0;
    }

    void reallocate_to(size_t size, purge_tags tag, void** user)
    {
        if (!user && memblock_t::is_purgeable(tag))
            I_Error("Z_Malloc: an owner is required for purgeable blocks");

        if (user) *user = content();

        this->user = user;
        this->size = size;
        this->tag = tag;
        id = ZONEID;
    }

    void change_user(void** new_user);

    [[nodiscard]] void*  content() { return const_cast<std::byte*>(reinterpret_cast<std::byte const*>(this) + sizeof(memblock_t)); };
    [[nodiscard]] size_t content_size() const { return size - sizeof(memblock_t); }
};
}