#include "mock_main.hpp"

#ifdef CRISPY_DOOM
#define BEGIN_EXTERN extern "C" {
#define END_EXTERN }
BEGIN_EXTERN
#include "crispy.h"
#include "m_misc.h"
#include "m_argv.h"
END_EXTERN
#elif CPP_DOOM
#define BEGIN_EXTERN
#define END_EXTERN
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

#include "args_guard.hpp"
#include "lights_guard.hpp"

extern void new_main(const char* demo_name);

BEGIN_EXTERN
extern boolean already_quitting;
END_EXTERN

struct exit_guard
{
    ~exit_guard() {
        already_quitting = false;
    }
};

BEGIN_EXTERN
struct lumpinfo_t;
using lumpindex_t = int;
extern lumpindex_t* lumphash;
extern lumpinfo_t** lumpinfo;
extern unsigned numlumps;
END_EXTERN

struct lump_guard{
    ~lump_guard()
    {
#ifdef CRISPY_DOOM
#endif
        lumphash = nullptr;
        free(lumpinfo);
        lumpinfo = nullptr;
        numlumps = 0;
    }
};

BEGIN_EXTERN
struct deh_subsitution_t;
extern deh_subsitution_t** hash_table;
extern int hash_table_entries;
extern int hash_table_length;
END_EXTERN

struct hash_table_guard
{
    ~hash_table_guard()
    {
        hash_table = nullptr;
        hash_table_entries = 0;
        hash_table_length = -1;
    }
};

BEGIN_EXTERN
extern uint8_t *disk_data;
extern uint8_t *saved_background;
END_EXTERN

struct diskicon_guard
{
    ~diskicon_guard()
    {
        disk_data = nullptr;
        saved_background = nullptr;
    }
};

BEGIN_EXTERN
extern gameaction_t gameaction;
END_EXTERN

struct gameplay_guard{
    ~gameplay_guard()
    {
        gameaction = ga_nothing;
    }
};

void RunDoomMain(char const* demo)
{
    args_guard guard{
        std::array{"-iwad", "doom.wad", "-nogui", "-timedemo", demo, "-nodraw"}};
    exit_guard exit_g{};
    lights_guard lights_g{};
    lump_guard lump_g{};
    hash_table_guard hash_g{};
    diskicon_guard disk_g{};
    gameplay_guard game_g{};

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



