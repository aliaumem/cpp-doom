//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
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
// DESCRIPTION:
//	Archiving: SaveGame I/O.
//	Thinker, Ticker.
//

#include "p_local.hpp"
#include "s_musinfo.hpp" // [crispy] T_MAPMusic()
#include "z_zone.hpp"

#include "doomstat.hpp"

int leveltime;

//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//

//
// P_RunThinkers
//
void P_RunThinkers(void) {
  thinker_list::instance.run();

  // [crispy] support MUSINFO lump (dynamic music changing)
  T_MusInfo();
}

void thinker_list::run() {
  clear_removed();

  for (auto *currentthinker : thinkers) {
    currentthinker->function.call_if(currentthinker);
  }
}

void thinker_list::clear_removed() {
  auto removedIt = std::stable_partition(
      thinkers.begin(), thinkers.end(),
      [](auto *thinker) { return !thinker->needs_removal(); });
  std::for_each(removedIt, thinkers.end(), Z_Free);
  thinkers.erase(removedIt, thinkers.end());
}

//
// P_Ticker
//

void P_Ticker(void) {
  int i;

  // run the tic
  if (paused)
    return;

  // pause if in menu and at least one tic has been run
  if (!netgame && menuactive && !demoplayback &&
      players[consoleplayer].viewz != 1) {
    return;
  }

  for (i = 0; i < MAXPLAYERS; i++)
    if (playeringame[i])
      P_PlayerThink(&players[i]);

  P_RunThinkers();
  P_UpdateSpecials();
  P_RespawnSpecials();

  // for par times
  leveltime++;
}
