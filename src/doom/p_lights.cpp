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
//	Handle Sector base lighting effects.
//	Muzzle flash?
//

#include "m_random.hpp"
#include "z_zone.hpp"

#include "doomdef.hpp"
#include "p_local.hpp"

// State.
#include "../../utils/memory.hpp"
#include "r_state.hpp"

//
// FIRELIGHT FLICKER
//

void fireflicker_t::perform() {
  int amount;

  if (--count)
    return;

  amount = (P_Random() & 3) * 16;

  if (sector->lightlevel - amount < minlight)
    sector->lightlevel = minlight;
  else
    sector->lightlevel = maxlight - amount;

  count = 4;
}

//
// P_SpawnFireFlicker
//
void P_SpawnFireFlicker(sector_t *sector) {
  // Note that we are resetting sector attributes.
  // Nothing special about it during gameplay.
  sector->special = 0;

  znew<fireflicker_t>(basic_light{sector, min_light_offset{16}}, 4);
}

//
// BROKEN LIGHT FLASHING
//

//
// T_LightFlash
// Do flashing lights.
//
inline void lightflash_t::perform() {
  if (--count)
    return;

  if (sector->lightlevel == maxlight) {
    sector->lightlevel = minlight;
    count = (P_Random() & mintime) + 1;
  } else {
    sector->lightlevel = maxlight;
    count = (P_Random() & maxtime) + 1;
  }
}

//
// P_SpawnLightFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void P_SpawnLightFlash(sector_t *sector) {
  // nothing special about it during gameplay
  sector->special = 0;

  znew<lightflash_t>(basic_light{sector}, (P_Random() & 64) + 1, 64, 7);
}

//
// STROBE LIGHT FLASHING
//

//
// T_StrobeFlash
//
void T_StrobeFlash(strobe_t *flash) {
  if (--flash->count)
    return;

  if (flash->sector->lightlevel == flash->minlight) {
    flash->sector->lightlevel = flash->maxlight;
    flash->count = flash->brighttime;
  } else {
    flash->sector->lightlevel = flash->minlight;
    flash->count = flash->darktime;
  }
}

//
// P_SpawnStrobeFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void P_SpawnStrobeFlash(sector_t *sector, int fastOrSlow, int inSync) {
  // nothing special about it during gameplay
  sector->special = 0;

  znew<strobe_t>(basic_light{sector}, inSync ? 1 : (P_Random() & 7) + 1,
                 fastOrSlow, STROBEBRIGHT);
}

//
// Start strobing lights (usually from a trigger)
//
void EV_StartLightStrobing(line_t *line) {
  int secnum;
  sector_t *sec;

  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0) {
    sec = &sectors[secnum];
    if (sec->specialdata)
      continue;

    P_SpawnStrobeFlash(sec, SLOWDARK, 0);
  }
}

//
// TURN LINE'S TAG LIGHTS OFF
//
void EV_TurnTagLightsOff(line_t *line) {
  int i;
  int j;
  int min;
  sector_t *sector;
  sector_t *tsec;
  line_t *templine;

  sector = sectors;

  for (j = 0; j < numsectors; j++, sector++) {
    if (sector->tag == line->tag) {
      min = sector->lightlevel;
      for (i = 0; i < sector->linecount; i++) {
        templine = sector->lines[i];
        tsec = getNextSector(templine, sector);
        if (!tsec)
          continue;
        if (tsec->lightlevel < min)
          min = tsec->lightlevel;
      }
      sector->lightlevel = min;
    }
  }
}

//
// TURN LINE'S TAG LIGHTS ON
//
void EV_LightTurnOn(line_t *line, int bright) {
  int i;
  int j;
  sector_t *sector;
  sector_t *temp;
  line_t *templine;

  sector = sectors;

  for (i = 0; i < numsectors; i++, sector++) {
    if (sector->tag == line->tag) {
      // bright = 0 means to search
      // for highest light level
      // surrounding sector
      if (!bright) {
        for (j = 0; j < sector->linecount; j++) {
          templine = sector->lines[j];
          temp = getNextSector(templine, sector);

          if (!temp)
            continue;

          if (temp->lightlevel > bright)
            bright = temp->lightlevel;
        }
      }
      sector->lightlevel = bright;
    }
  }
}

//
// Spawn glowing light
//
void P_SpawnGlowingLight(sector_t *sector) {
    sector->special = 0;

    znew<glow_t>(basic_light{sector}, glow_t::direction::down);
}

inline void glow_t::perform() {
//void T_Glow(glow_t *g) {
  switch (dir) {
  case direction::down:
    // DOWN
    sector->lightlevel -= GLOWSPEED;
    if (sector->lightlevel <= minlight) {
      sector->lightlevel += GLOWSPEED;
        dir = direction::up;
    }
    break;

  case direction::up:
    // UP
    sector->lightlevel += GLOWSPEED;
    if (sector->lightlevel >= maxlight) {
      sector->lightlevel -= GLOWSPEED;
        dir = direction::down;
    }
    break;
  }
}

