#include "ApprovalTests.hpp"
#include "catch2/catch.hpp"

#include "mock_main.hpp"

using namespace ApprovalTests;


TEST_CASE("Launch_Doom_Main")
{
    auto demo = GENERATE("BOHFIGHT.LMP");
    RunDoomMain(demo);
}