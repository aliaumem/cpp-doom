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
// DESCRIPTION:  Ceiling aninmation (lowering, crushing, raising)
//

#include "doomdef.hpp"
#include "p_local.hpp"
#include "z_zone.hpp"

#include "s_sound.hpp"

// State.
#include "doomstat.hpp"
#include "r_state.hpp"

// Data.
#include "../../utils/memory.hpp"
#include "sounds.hpp"

//
// CEILINGS
//

std::array<ceiling_t *, MAXCEILINGS> activeceilings;

//
// T_MoveCeiling
//

void T_MoveCeiling(ceiling_t *ceiling) {
  /*    ceiling->think();
  }

  void ceiling_t::think() {*/
  result_e res;

  switch (ceiling->direction) {
  case 0:
    // IN STASIS
    break;
  case 1:
    // UP
    res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->topheight,
                      false, 1, ceiling->direction);

    if (!(leveltime & 7)) {
      switch (ceiling->type) {
      case silentCrushAndRaise:
        break;
      default:
        S_StartSound(&ceiling->sector->soundorg, sfx_stnmov);
        // ?
        break;
      }
    }

    if (res == pastdest) {
      switch (ceiling->type) {
      case raiseToHighest:
        P_RemoveActiveCeiling(ceiling);
        break;

      case silentCrushAndRaise:
        S_StartSound(&ceiling->sector->soundorg, sfx_pstop);
      case fastCrushAndRaise:
      case crushAndRaise:
        ceiling->direction = -1;
        break;

      default:
        break;
      }
    }
    break;

  case -1:
    // DOWN
    res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight,
                      ceiling->crush, 1, ceiling->direction);

    if (!(leveltime & 7)) {
      switch (ceiling->type) {
      case silentCrushAndRaise:
        break;
      default:
        S_StartSound(&ceiling->sector->soundorg, sfx_stnmov);
      }
    }

    if (res == pastdest) {
      switch (ceiling->type) {
      case silentCrushAndRaise:
        S_StartSound(&ceiling->sector->soundorg, sfx_pstop);
      case crushAndRaise:
        ceiling->speed = CEILSPEED;
      case fastCrushAndRaise:
        ceiling->direction = 1;
        break;

      case lowerAndCrush:
      case lowerToFloor:
        P_RemoveActiveCeiling(ceiling);
        break;

      default:
        break;
      }
    } else // ( res != pastdest )
    {
      if (res == crushed) {
        switch (ceiling->type) {
        case silentCrushAndRaise:
        case crushAndRaise:
        case lowerAndCrush:
          ceiling->speed = CEILSPEED / 8;
          break;

        default:
          break;
        }
      }
    }
    break;
  }
}

//
// EV_DoCeiling
// Move a ceiling up/down and all around!
//
int EV_DoCeiling(line_t *line, ceiling_e type) {
  int secnum;
  int rtn;
  sector_t *sec;
  ceiling_t *ceiling;

  secnum = -1;
  rtn = 0;

  //	Reactivate in-stasis ceilings...for certain types.
  switch (type) {
  case fastCrushAndRaise:
  case silentCrushAndRaise:
  case crushAndRaise:
    P_ActivateInStasisCeiling(line);
  default:
    break;
  }

  while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0) {
    sec = &sectors[secnum];
    if (sec->specialdata)
      continue;

    // new door thinker
    rtn = 1;
    ceiling = znew<ceiling_t>();
    thinker_list::instance.push_back(ceiling);
    sec->specialdata = ceiling;
    ceiling->sector = sec;
    ceiling->crush = false;

    switch (type) {
    case fastCrushAndRaise:
      ceiling->crush = true;
      ceiling->topheight = sec->ceilingheight;
      ceiling->bottomheight = sec->floorheight + (8 * FRACUNIT);
      ceiling->direction = -1;
      ceiling->speed = CEILSPEED * 2;
      break;

    case silentCrushAndRaise:
    case crushAndRaise:
      ceiling->crush = true;
      ceiling->topheight = sec->ceilingheight;
    case lowerAndCrush:
    case lowerToFloor:
      ceiling->bottomheight = sec->floorheight;
      if (type != lowerToFloor)
        ceiling->bottomheight += 8 * FRACUNIT;
      ceiling->direction = -1;
      ceiling->speed = CEILSPEED;
      break;

    case raiseToHighest:
      ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
      ceiling->direction = 1;
      ceiling->speed = CEILSPEED;
      break;
    }

    ceiling->tag = sec->tag;
    ceiling->type = type;
    P_AddActiveCeiling(ceiling);
  }
  return rtn;
}

//
// Add an active ceiling
//
void P_AddActiveCeiling(ceiling_t *c) {
  for (auto &active_ceiling : activeceilings) {
    if (active_ceiling == nullptr) {
      active_ceiling = c;
      return;
    }
  }
}

//
// Remove a ceiling's thinker
//
void P_RemoveActiveCeiling(ceiling_t *c) {
  for (auto &active_ceiling : activeceilings) {
    if (active_ceiling == c) {
      active_ceiling->sector->specialdata = nullptr;
      active_ceiling->mark_for_removal();
      active_ceiling = nullptr;
      break;
    }
  }
}

//
// Restart a ceiling that's in-stasis
//
void P_ActivateInStasisCeiling(line_t *line) {
  for (auto &active_ceiling : activeceilings) {
    if (active_ceiling && (active_ceiling->tag == line->tag) &&
        (active_ceiling->direction == 0)) {
      active_ceiling->direction = active_ceiling->olddirection;
      active_ceiling->start_moving();
    }
  }
}

//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
//
int EV_CeilingCrushStop(line_t *line) {
  int rtn;

  rtn = 0;
  for (auto &active_ceiling : activeceilings) {
    if (active_ceiling && (active_ceiling->tag == line->tag) &&
        (active_ceiling->direction != 0)) {
      active_ceiling->olddirection = active_ceiling->direction;
      active_ceiling->stop_moving();
      active_ceiling->direction = 0; // in-stasis
      rtn = 1;
    }
  }

  return rtn;
}
