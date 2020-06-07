#pragma once

#include "memblock.hpp"

#include <iterator>

namespace doom::zone {
struct memzone_t {
    explicit memzone_t(size_t size)
        : size { size }
        , blocklist { size - sizeof(memzone_t),
            PU_STATIC, reinterpret_cast<void**>(this) }
        , rover { nullptr }
    {
        auto* firstBlock = new (reinterpret_cast<char*>(this) + sizeof(memzone_t))
            memblock_t { size - sizeof(memzone_t), PU_FREE, &blocklist, &blocklist };
        blocklist.prev = blocklist.next = firstBlock;
        rover                           = iterator { firstBlock };
    }

    class iterator {
    public:
        using value_type        = memblock_t;
        using reference         = memblock_t*;
        using pointer           = memblock_t*;
        using iterator_category = std::bidirectional_iterator_tag;

        explicit iterator(memblock_t* block)
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

        [[nodiscard]] iterator next() const { return iterator { m_block->next }; }
        [[nodiscard]] iterator prev() const { return iterator { m_block->prev }; }

        pointer   operator->() { return m_block; }
        reference operator*() { return m_block; }
        bool      operator==(iterator const& other) const { return m_block == other.m_block; }
        bool      operator!=(iterator const& other) const { return !(*this == other); }

    private:
        memblock_t* m_block;
    };

    static iterator iterator_at(void* ptr)
    {
        return iterator { reinterpret_cast<memblock_t*>(reinterpret_cast<std::byte*>(ptr) - sizeof(memblock_t)) };
    }

    void clear() { *this = memzone_t { size }; }

    void  free_block(iterator block);
    void* allocate(size_t size, purge_tags tag, void* user);
    void  free_tags(purge_tags lowtag, purge_tags hightag);

    [[nodiscard]] iterator begin() { return iterator { blocklist.next }; }
    [[nodiscard]] iterator end() { return iterator { &blocklist }; }

    // total bytes malloced, including header
    size_t size;

    // start / end cap for linked list
    memblock_t blocklist;

    iterator rover;
};

struct memory_resource {
    static void init(std::byte* zone, std::size_t size);

    memory_resource() noexcept
        : mainzone(nullptr)
    {
    }

    memory_resource(std::byte* memory_zone, size_t size) noexcept
        : mainzone(new (memory_zone) memzone_t { size })
    {
    }

    auto begin() { return mainzone->begin(); }
    auto end() { return mainzone->end(); }

    memzone_t* operator->() const { return mainzone; }

    memzone_t* mainzone;
    bool       zero_on_free = false;
    bool       scan_on_free = false;
};

inline static memory_resource mem_res;
}
