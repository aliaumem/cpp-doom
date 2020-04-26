//
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2014 Fabian Greffrath
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// Parses [CODEPTR] sections in BEX files
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string_view>
#include <array>

#include "info.hpp"

#include "deh_io.hpp"
#include "deh_main.hpp"
#include "event_function_decls.hpp"

struct bex_codeptr_t {
    const std::string_view mnemonic;
    const actionf_t pointer;
};

static constexpr const std::array bex_codeptrtable = {
    bex_codeptr_t{"Light0", {A_Light0}},
    bex_codeptr_t{"WeaponReady", {A_WeaponReady}},
    bex_codeptr_t{"Lower", {A_Lower}},
    bex_codeptr_t{"Raise", {A_Raise}},
    bex_codeptr_t{"Punch", {A_Punch}},
    bex_codeptr_t{"ReFire", {A_ReFire}},
    bex_codeptr_t{"FirePistol", {A_FirePistol}},
    bex_codeptr_t{"Light1", {A_Light1}},
    bex_codeptr_t{"FireShotgun", {A_FireShotgun}},
    bex_codeptr_t{"Light2", {A_Light2}},
    bex_codeptr_t{"FireShotgun2", {A_FireShotgun2}},
    bex_codeptr_t{"CheckReload", {A_CheckReload}},
    bex_codeptr_t{"OpenShotgun2", {A_OpenShotgun2}},
    bex_codeptr_t{"LoadShotgun2", {A_LoadShotgun2}},
    bex_codeptr_t{"CloseShotgun2", {A_CloseShotgun2}},
    bex_codeptr_t{"FireCGun", {A_FireCGun}},
    bex_codeptr_t{"GunFlash", {A_GunFlash}},
    bex_codeptr_t{"FireMissile", {A_FireMissile}},
    bex_codeptr_t{"Saw", {A_Saw}},
    bex_codeptr_t{"FirePlasma", {A_FirePlasma}},
    bex_codeptr_t{"BFGsound", {A_BFGsound}},
    bex_codeptr_t{"FireBFG", {A_FireBFG}},
    bex_codeptr_t{"BFGSpray", {A_BFGSpray}},
    bex_codeptr_t{"Explode", {A_Explode}},
    bex_codeptr_t{"Pain", {A_Pain}},
    bex_codeptr_t{"PlayerScream", {A_PlayerScream}},
    bex_codeptr_t{"Fall", {A_Fall}},
    bex_codeptr_t{"XScream", {A_XScream}},
    bex_codeptr_t{"Look", {A_Look}},
    bex_codeptr_t{"Chase", {A_Chase}},
    bex_codeptr_t{"FaceTarget", {A_FaceTarget}},
    bex_codeptr_t{"PosAttack", {A_PosAttack}},
    bex_codeptr_t{"Scream", {A_Scream}},
    bex_codeptr_t{"SPosAttack", {A_SPosAttack}},
    bex_codeptr_t{"VileChase", {A_VileChase}},
    bex_codeptr_t{"VileStart", {A_VileStart}},
    bex_codeptr_t{"VileTarget", {A_VileTarget}},
    bex_codeptr_t{"VileAttack", {A_VileAttack}},
    bex_codeptr_t{"StartFire", {A_StartFire}},
    bex_codeptr_t{"Fire", {A_Fire}},
    bex_codeptr_t{"FireCrackle", {A_FireCrackle}},
    bex_codeptr_t{"Tracer", {A_Tracer}},
    bex_codeptr_t{"SkelWhoosh", {A_SkelWhoosh}},
    bex_codeptr_t{"SkelFist", {A_SkelFist}},
    bex_codeptr_t{"SkelMissile", {A_SkelMissile}},
    bex_codeptr_t{"FatRaise", {A_FatRaise}},
    bex_codeptr_t{"FatAttack1", {A_FatAttack1}},
    bex_codeptr_t{"FatAttack2", {A_FatAttack2}},
    bex_codeptr_t{"FatAttack3", {A_FatAttack3}},
    bex_codeptr_t{"BossDeath", {A_BossDeath}},
    bex_codeptr_t{"CPosAttack", {A_CPosAttack}},
    bex_codeptr_t{"CPosRefire", {A_CPosRefire}},
    bex_codeptr_t{"TroopAttack", {A_TroopAttack}},
    bex_codeptr_t{"SargAttack", {A_SargAttack}},
    bex_codeptr_t{"HeadAttack", {A_HeadAttack}},
    bex_codeptr_t{"BruisAttack", {A_BruisAttack}},
    bex_codeptr_t{"SkullAttack", {A_SkullAttack}},
    bex_codeptr_t{"Metal", {A_Metal}},
    bex_codeptr_t{"SpidRefire", {A_SpidRefire}},
    bex_codeptr_t{"BabyMetal", {A_BabyMetal}},
    bex_codeptr_t{"BspiAttack", {A_BspiAttack}},
    bex_codeptr_t{"Hoof", {A_Hoof}},
    bex_codeptr_t{"CyberAttack", {A_CyberAttack}},
    bex_codeptr_t{"PainAttack", {A_PainAttack}},
    bex_codeptr_t{"PainDie", {A_PainDie}},
    bex_codeptr_t{"KeenDie", {A_KeenDie}},
    bex_codeptr_t{"BrainPain", {A_BrainPain}},
    bex_codeptr_t{"BrainScream", {A_BrainScream}},
    bex_codeptr_t{"BrainDie", {A_BrainDie}},
    bex_codeptr_t{"BrainAwake", {A_BrainAwake}},
    bex_codeptr_t{"BrainSpit", {A_BrainSpit}},
    bex_codeptr_t{"SpawnSound", {A_SpawnSound}},
    bex_codeptr_t{"SpawnFly", {A_SpawnFly}},
    bex_codeptr_t{"BrainExplode", {A_BrainExplode}},
    // [crispy] additional BOOM and MBF states, sprites and code pointers
    bex_codeptr_t{"Stop", {A_Stop}},
    bex_codeptr_t{"Die", {A_Die}},
    bex_codeptr_t{"FireOldBFG", {A_FireOldBFG}},
    bex_codeptr_t{"Detonate", {A_Detonate}},
    bex_codeptr_t{"Mushroom", {A_Mushroom}},
    bex_codeptr_t{"BetaSkullAttack", {A_BetaSkullAttack}},
    // [crispy] more MBF code pointers
    bex_codeptr_t{"Spawn", {A_Spawn}},
    bex_codeptr_t{"Turn", {A_Turn}},
    bex_codeptr_t{"Face", {A_Face}},
    bex_codeptr_t{"Scratch", {A_Scratch}},
    bex_codeptr_t{"PlaySound", {A_PlaySound}},
    bex_codeptr_t{"RandomJump", {A_RandomJump}},
    bex_codeptr_t{"LineEffect", {A_LineEffect}},
    bex_codeptr_t{"NULL", {}},
};

extern actionf_t codeptrs[NUMSTATES];

static void *DEH_BEXPtrStart(deh_context_t *context, char *line)
{
    char s[10];

    if (sscanf(line, "%9s", s) == 0 || strcmp("[CODEPTR]", s))
    {
	DEH_Warning(context, "Parse error on section start");
    }

    return NULL;
}

static void DEH_BEXPtrParseLine(deh_context_t *context, char *line, void *tag)
{
    state_t *state;
    char *variable_name, *value, frame_str[6];
    int frame_number, i;

    // parse "FRAME nn = mnemonic", where
    // variable_name = "FRAME nn" and value = "mnemonic"
    if (!DEH_ParseAssignment(line, &variable_name, &value))
    {
	DEH_Warning(context, "Failed to parse assignment: %s", line);
	return;
    }

    // parse "FRAME nn", where frame_number = "nn"
    if (sscanf(variable_name, "%5s %32d", frame_str, &frame_number) != 2 ||
        strcasecmp(frame_str, "FRAME"))
    {
	DEH_Warning(context, "Failed to parse assignment: %s", variable_name);
	return;
    }

    if (frame_number < 0 || frame_number >= NUMSTATES)
    {
	DEH_Warning(context, "Invalid frame number: %i", frame_number);
	return;
    }

    state = (state_t *) &states[frame_number];

    for (i = 0; i < bex_codeptrtable.size(); i++)
    {
	if (bex_codeptrtable[i].mnemonic == value)
	{
	    state->action = bex_codeptrtable[i].pointer;
	    return;
	}
    }

    DEH_Warning(context, "Invalid mnemonic '%s'", value);
}

deh_section_t deh_section_bexptr =
{
    "[CODEPTR]",
    NULL,
    DEH_BEXPtrStart,
    DEH_BEXPtrParseLine,
    NULL,
    NULL,
};
