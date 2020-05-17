#include <ApprovalTests.hpp>
#include <catch2/catch.hpp>

#ifdef CRISPY_DOOM
extern "C" {
#include <z_zone.h>
}
#elif CPP_DOOM
#include <z_zone.hpp>
#endif

#include "args_guard.hpp"

#ifdef CRISPY_DOOM
using dbyte = byte;
#elif CPP_DOOM
using dbyte = std::byte;
#endif

TEST_CASE("Z_Malloc")
{
    SECTION("Init")
    {
        args_guard guard{};
        std::vector<std::byte> mem_zone;
        mem_zone.resize(16*1024*1024);
        Z_InitMem(reinterpret_cast<dbyte*>(mem_zone.data()), mem_zone.size());
        Z_CheckHeap();
    }

    SECTION("Init again")
    {
        std::vector<std::byte> mem_zone;
        mem_zone.resize(16*1024*1024);
        Z_InitMem(reinterpret_cast<dbyte*>(mem_zone.data()), mem_zone.size());
        Z_CheckHeap();
    }
}