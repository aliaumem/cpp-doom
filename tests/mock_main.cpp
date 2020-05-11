#include "mock_main.hpp"

#ifdef CRISPY_DOOM
extern "C" {
#include "crispy.h"
#include "m_misc.h"
#include "m_argv.h"
}
#elif CPP_DOOM
#include "crispy.hpp"
#include "m_misc.hpp"
#include "m_argv.hpp"
#else
#error "Configuration not supported"
#endif

#include "SDL.h"

#include <array>

#ifdef CRISPY_DOOM
enum dummy {};
static_assert(sizeof(dummy) == sizeof(boolean));
#endif

extern void new_main(const char* demo_name);

void RunDoomMain(char const* demo)
{
    std::array argv = {"/", "-iwad", "doom.wad", "-nogui", "-timedemo", demo, "-nodraw"};
    myargc = argv.size();
    myargv = const_cast<char**>(argv.data());

    {
        char buf[16];
        SDL_version version;
        SDL_GetVersion(&version);
        M_snprintf(buf, sizeof(buf), "%d.%d.%d", version.major, version.minor, version.patch);
        crispy->sdlversion = M_StringDuplicate(buf);
        crispy->platform = SDL_GetPlatform();
    }

#ifdef SDL_HINT_NO_SIGNAL_HANDLERS
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
#endif

    new_main(demo);
}



