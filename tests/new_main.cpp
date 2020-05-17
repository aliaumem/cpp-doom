#ifdef CRISPY_DOOM
extern "C" {
#include <m_config.h>
#include <v_video.h>
#include <i_system.h>
#include <d_iwad.h>
#include <doomstat.h>
#include <w_wad.h>
#include <w_main.h>
#include <d_englsh.h>
#include <deh_str.h>
#include <g_game.h>
#include <i_sound.h>
#include <sounds.h>
#include <m_menu.h>
#include <r_main.h>
#include <p_setup.h>
#include <s_sound.h>
#include <hu_stuff.h>
#include <st_stuff.h>
#include <m_misc.h>
#include <cstdlib>
#include <net_client.h>
#include "d_main.h"
#include "z_zone.h"
};

#elif CPP_DOOM

#include <m_config.hpp>
#include <v_video.hpp>
#include <i_system.hpp>
#include <d_iwad.hpp>
#include <doomstat.hpp>
#include <w_wad.hpp>
#include <w_main.hpp>
#include <d_englsh.hpp>
#include <deh_str.hpp>
#include <g_game.hpp>
#include <i_sound.hpp>
#include <sounds.hpp>
#include <m_menu.hpp>
#include <r_main.hpp>
#include <p_setup.hpp>
#include <s_sound.hpp>
#include <hu_stuff.hpp>
#include <st_stuff.hpp>
#include <m_misc.hpp>
#include <cstdlib>
#include <net_client.hpp>
#include <vector>
#include "d_main.hpp"
#include "z_zone.hpp"
#endif

#include <vector>
#include <stdexcept>

#ifdef CRISPY_DOOM
extern "C" {
#endif

extern void D_BindVariables();
extern void D_IdentifyVersion();
extern void InitGameVersion();
extern void D_SetGameDescription();
extern void EnableLoadingDisk();
extern void R_ExecuteSetViewSize();
extern void D_RunFrame();
extern void D_ConnectNetGame();
extern void D_CheckNetGame();
extern void M_CrispyReinitHUDWidgets();
extern char * iwadfile;
extern boolean storedemo;

#ifdef CRISPY_DOOM
}
#endif

void new_loop();

void new_main(const char* demo_name)
{
    char file[256];
    char demolumpname[9];

    std::vector<std::byte> ram(32*1024*1024); // Give it 32MB of Ram
    printf("RAM is located at %p\n", static_cast<void*>(ram.data()));
#ifdef CRISPY_DOOM
    auto ramBuff = reinterpret_cast<uint8_t*>(ram.data());
#elif CPP_DOOM
    auto ramBuff = ram.data();
#endif
    Z_InitMem (ramBuff, static_cast<int>(ram.size()));

    {
        // Auto-detect the configuration dir.

        M_SetConfigDir(nullptr);
    }

    // init subsystems
    V_Init ();

    M_SetConfigFilenames("default.cfg", PROGRAM_PREFIX "doom.cfg");
    D_BindVariables();
    M_LoadDefaults();

    // Save configuration at exit.
    I_AtExit(M_SaveDefaults, true); // [crispy] always save configuration at exit

    // Find main IWAD file and load it.
    iwadfile = D_FindIWAD(IWAD_MASK_DOOM, &gamemission);

    // None found?

    if (iwadfile == NULL)
    {
        I_Error("Game mode indeterminate.  No IWAD file was found.  Try\n"
                "specifying one with the '-iwad' command line parameter.\n");
    }

    modifiedgame = false;

    W_AddFile(iwadfile);

    W_CheckCorrectIWAD(doom);

    // Now that we've loaded the IWAD, we can figure out what gamemission
    // we're playing and which version of Vanilla Doom we need to emulate.
    D_IdentifyVersion();
    InitGameVersion();

    // Check which IWAD variant we are using.

    if (W_CheckNumForName("FREEDOOM") >= 0)
    {
        if (W_CheckNumForName("FREEDM") >= 0)
        {
            gamevariant = freedm;
        }
        else
        {
            gamevariant = freedoom;
        }
    }
    else if (W_CheckNumForName("DMENUPIC") >= 0)
    {
        gamevariant = bfgedition;
    }

    // Doom 3: BFG Edition includes modified versions of the classic
    // IWADs which can be identified by an additional DMENUPIC lump.
    // Furthermore, the M_GDHIGH lumps have been modified in a way that
    // makes them incompatible to Vanilla Doom and the modified version
    // of doom2.wad is missing the TITLEPIC lump.
    // We specifically check for DMENUPIC here, before PWADs have been
    // loaded which could probably include a lump of that name.

    if (gamevariant == bfgedition)
    {
        printf("BFG Edition: Using workarounds as needed.\n");

        // BFG Edition changes the names of the secret levels to
        // censor the Wolfenstein references. It also has an extra
        // secret level (MAP33). In Vanilla Doom (meaning the DOS
        // version), MAP33 overflows into the Plutonia level names
        // array, so HUSTR_33 is actually PHUSTR_1.
        DEH_AddStringReplacement(HUSTR_31, "level 31: idkfa");
        DEH_AddStringReplacement(HUSTR_32, "level 32: keen");
        DEH_AddStringReplacement(PHUSTR_1, "level 33: betray");

        // The BFG edition doesn't have the "low detail" menu option (fair
        // enough). But bizarrely, it reuses the M_GDHIGH patch as a label
        // for the options menu (says "Fullscreen:"). Why the perpetrators
        // couldn't just add a new graphic lump and had to reuse this one,
        // I don't know.
        //
        // The end result is that M_GDHIGH is too wide and causes the game
        // to crash. As a workaround to get a minimum level of support for
        // the BFG edition IWADs, use the "ON"/"OFF" graphics instead.
        DEH_AddStringReplacement("M_GDHIGH", "M_MSGON");
        DEH_AddStringReplacement("M_GDLOW", "M_MSGOFF");

        // The BFG edition's "Screen Size:" graphic has also been changed
        // to say "Gamepad:". Fortunately, it (along with the original
        // Doom IWADs) has an unused graphic that says "Display". So we
        // can swap this in instead, and it kind of makes sense.
        DEH_AddStringReplacement("M_SCRNSZ", "M_DISP");
    }

    // Load PWAD files.
    modifiedgame = W_ParseCommandLine();

    {
        char *uc_filename = strdup(demo_name);
        M_ForceUppercase(uc_filename);

        // With Vanilla you have to specify the file without extension,
        // but make that optional.
        if (M_StringEndsWith(uc_filename, ".LMP"))
        {
            M_StringCopy(file, demo_name, sizeof(file));
        }
        else
        {
            DEH_snprintf(file, sizeof(file), "%s.lmp", demo_name);
        }

        free(uc_filename);

        if (W_AddFile(file))
        {
            M_StringCopy(demolumpname, lumpinfo[numlumps - 1]->name,
                         sizeof(demolumpname));
        }
        else
        {
            // If file failed to load, still continue trying to play
            // the demo in the same way as Vanilla Doom.  This makes
            // tricks like "-playdemo demo1" possible.

            M_StringCopy(demolumpname, demo_name, sizeof(demolumpname));
        }

        printf("Playing demo %s.\n", file);
    }

    I_AtExit([](){G_CheckDemoStatus();}, true);

    // Generate the WAD hash table.  Speed things up a bit.
    W_GenerateHashTable();

    // Set the gamedescription string. This is only possible now that
    // we've finished loading Dehacked patches.
    D_SetGameDescription();

    savegamedir = M_GetSaveGameDir(D_SaveGameIWADName(gamemission));

    // Check for -file in shareware
    if (modifiedgame && (gamevariant != freedoom))
    {
        // These are the lumps that will be checked in IWAD,
        // if any one is not present, execution will be aborted.
        char name[23][9]=
                {
                        "e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
                        "e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
                        "dphoof","bfgga0","heada1","cybra1","spida1d1"
                };
        int i;

        if ( gamemode == shareware)
            I_Error(DEH_String("\nYou cannot -file with the shareware "
                               "version. Register!"));

        // Check for fake IWAD with right name,
        // but w/o all the lumps of the registered version.
        if (gamemode == registered)
            for (i = 0;i < 23; i++)
                if (W_CheckNumForName(name[i])<0)
                    I_Error(DEH_String("\nThis is not the registered version."));
    }

    I_InitTimer();
    //I_InitSound(true);
    //I_InitMusic();

    // [crispy] check for SSG resources
    crispy->havessg =
            (
                    gamemode == commercial ||
                    (
                            W_CheckNumForName("sht2a0")         != -1 && // [crispy] wielding/firing sprite sequence
                            I_GetSfxLumpNum(&S_sfx[sfx_dshtgn]) != -1 && // [crispy] firing sound
                            I_GetSfxLumpNum(&S_sfx[sfx_dbopn])  != -1 && // [crispy] opening sound
                            I_GetSfxLumpNum(&S_sfx[sfx_dbload]) != -1 && // [crispy] reloading sound
                            I_GetSfxLumpNum(&S_sfx[sfx_dbcls])  != -1    // [crispy] closing sound
                    )
            );

    // [crispy] check for presence of a 5th episode
    crispy->haved1e5 = (gameversion == exe_ultimate) &&
                       (W_CheckNumForName("m_epi5") != -1) &&
                       (W_CheckNumForName("e5m1") != -1) &&
                       (W_CheckNumForName("wilv40") != -1);

    // [crispy] check for presence of E1M10
    crispy->havee1m10 = (gamemode == retail) &&
                        (W_CheckNumForName("e1m10") != -1) &&
                        (W_CheckNumForName("sewers") != -1);

    // [crispy] check for presence of MAP33
    crispy->havemap33 = (gamemode == commercial) &&
                        (W_CheckNumForName("map33") != -1) &&
                        (W_CheckNumForName("cwilv32") != -1);

    // [crispy] change level name for MAP33 if not already changed
    if (crispy->havemap33 && !DEH_HasStringReplacement(PHUSTR_1))
    {
        DEH_AddStringReplacement(PHUSTR_1, "level 33: betray");
    }

    NET_Init ();

    // Initial netgame startup. Connect to server etc.
    D_ConnectNetGame();


    // get skill / episode / map from parms
    startskill = sk_medium;
    startepisode = 1;
    startmap = 1;
    autostart = false;


    timelimit = 0;

    // Check for load game parameter
    // We do this here and save the slot number, so that the network code
    // can override it or send the load slot to other players.

    {
        // Not loading a game
        startloadgame = -1;
    }

    M_Init ();

    R_Init ();

    P_Init ();

    S_Init (sfxVolume * 8, musicVolume * 8);

    D_CheckNetGame();

    HU_Init ();

    ST_Init ();

    // If Doom II without a MAP01 lump, this is a store demo.
    // Moved this here so that MAP01 isn't constantly looked up
    // in the main loop.

    if (gamemode == commercial && W_CheckNumForName("map01") < 0)
        storedemo = true;

    crispy->demowarp = 0; // [crispy] we don't play a demo, so don't skip maps

        G_TimeDemo (demolumpname);
        new_loop ();  // never returns
}

void new_loop ()
{
    if (gamevariant == bfgedition &&
        (demorecording || (gameaction == ga_playdemo) || netgame))
    {
        printf(" WARNING: You are playing using one of the Doom Classic\n"
               " IWAD files shipped with the Doom 3: BFG Edition. These are\n"
               " known to be incompatible with the regular IWAD files and\n"
               " may cause demos and network games to get out of sync.\n");
    }

    // [crispy] no need to write a demo header in demo continue mode
    if (demorecording && gameaction != ga_playdemo)
        G_BeginRecording ();

    I_SetWindowTitle(gamedescription);
    I_GraphicsCheckCommandLine();
    I_SetGrabMouseCallback([]() -> boolean {return false;});
    EnableLoadingDisk();

    TryRunTics();

    V_RestoreBuffer();
    R_ExecuteSetViewSize();

    D_StartGameLoop();

    if (testcontrols)
    {
        wipegamestate = gamestate;
    }

    try {
        while (1) {
            D_RunFrame();
        }
    }
    catch(std::runtime_error&)
    {
        // let's get out gracefully
    }
}