#define APPROVALS_CATCH
#include "ApprovalTests.hpp"

#include <fstream>

using namespace ApprovalTests;

extern void DoomLoop();

bool f()
{
    std::ifstream file;
    file.open("BOHFIGHT.LMP");

    return file.is_open();
}

TEST_CASE("Is_In_The_Right_Folder")
{
    REQUIRE(f());
}