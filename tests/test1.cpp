#include "ApprovalTests.hpp"
#include "catch2/catch.hpp"

#include <fstream>

using namespace ApprovalTests;

#include "crispy.hpp"
#include "SDL.h"
#include "m_misc.hpp"
#include "m_argv.hpp"

extern void D_DoomMain();


TEST_CASE("Is_In_The_Right_Folder")
{
    auto demo = GENERATE("BOHFIGHT.LMP");
    std::array argv = {"/", "-iwad", "doom.wad", "-nogui", "-timedemo", demo, "-nodraw"};
    myargc = argv.size();
    myargv = const_cast<char**>(argv.data());

    M_FindResponseFile();

#ifdef SDL_HINT_NO_SIGNAL_HANDLERS
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
#endif

    {
        char buf[16];
        SDL_version version;
        SDL_GetVersion(&version);
        M_snprintf(buf, sizeof(buf), "%d.%d.%d", version.major, version.minor, version.patch);
        crispy->sdlversion = M_StringDuplicate(buf);
        crispy->platform = SDL_GetPlatform();
    }
    D_DoomMain();
}