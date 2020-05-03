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
//

#include <cassert>
#include <stdio.h>
#include <stdlib.h>

#include "deh_main.hpp"
#include "dstrings.hpp"
#include "i_system.hpp"
#include "p_local.hpp"
#include "p_saveg.hpp"
#include "z_zone.hpp"

// State.
#include "../../utils/memory.hpp"
#include "doomstat.hpp"
#include "g_game.hpp"
#include "m_misc.hpp"
#include "r_state.hpp"

FILE *save_stream;
int savegamelength;
boolean savegame_error;
static int restoretargets_fail;

// Get the filename of a temporary file to write the savegame to.  After
// the file has been successfully saved, it will be renamed to the
// real file.

char *P_TempSaveGameFile(void) {
  static char *filename = NULL;

  if (filename == NULL) {
    filename = M_StringJoin(savegamedir, "temp.dsg", NULL);
  }

  return filename;
}

// Get the filename of the save game file to use for the specified slot.

char *P_SaveGameFile(int slot) {
  static char *filename = NULL;
  static size_t filename_size = 0;
  char basename[32];

  if (filename == NULL) {
    filename_size = strlen(savegamedir) + 32;
    filename = static_cast<char *>(malloc(filename_size));
  }

  DEH_snprintf(basename, 32, SAVEGAMENAME "%d.dsg", slot);
  M_snprintf(filename, filename_size, "%s%s", savegamedir, basename);

  return filename;
}

// Endian-safe integer read/write functions

static byte saveg_read8(void) {
  byte result = -1;

  if (fread(&result, 1, 1, save_stream) < 1) {
    if (!savegame_error) {
      fprintf(stderr, "saveg_read8: Unexpected end of file while "
                      "reading save game\n");

      savegame_error = true;
    }
  }

  return result;
}

static void saveg_write8(byte value) {
  if (fwrite(&value, 1, 1, save_stream) < 1) {
    if (!savegame_error) {
      fprintf(stderr, "saveg_write8: Error while writing save game\n");

      savegame_error = true;
    }
  }
}

static short saveg_read16(void) {
  int result;

  result = saveg_read8();
  result |= saveg_read8() << 8;

  return result;
}

static void saveg_write16(short value) {
  saveg_write8(value & 0xff);
  saveg_write8((value >> 8) & 0xff);
}

static int saveg_read32(void) {
  int result;

  result = saveg_read8();
  result |= saveg_read8() << 8;
  result |= saveg_read8() << 16;
  result |= saveg_read8() << 24;

  return result;
}

static void saveg_write32(int value) {
  saveg_write8(value & 0xff);
  saveg_write8((value >> 8) & 0xff);
  saveg_write8((value >> 16) & 0xff);
  saveg_write8((value >> 24) & 0xff);
}

// Pad to 4-byte boundaries

static void saveg_read_pad(void) {
  unsigned long pos;
  int padding;
  int i;

  pos = ftell(save_stream);

  padding = (4 - (pos & 3)) & 3;

  for (i = 0; i < padding; ++i) {
    saveg_read8();
  }
}

static void saveg_write_pad(void) {
  unsigned long pos;
  int padding;
  int i;

  pos = ftell(save_stream);

  padding = (4 - (pos & 3)) & 3;

  for (i = 0; i < padding; ++i) {
    saveg_write8(0);
  }
}

// Pointers

static void *saveg_readp(void) {
  return reinterpret_cast<void *>(static_cast<intptr_t>(saveg_read32()));
}

static void saveg_writep(const void *p) {
  saveg_write32(reinterpret_cast<intptr_t>(p));
}

// Enum values are 32-bit integers.

#define saveg_read_enum saveg_read32
#define saveg_write_enum saveg_write32

template <typename Enum,
          typename Enabled = std::enable_if_t<std::is_enum_v<Enum>>>
Enum savegame_read() {
  return static_cast<Enum>(saveg_read32());
}
template <typename Enum,
          typename Enabled = std::enable_if_t<std::is_enum_v<Enum>>>
void savegame_write(Enum value) {
  saveg_write32(
      static_cast<int>(static_cast<std::underlying_type_t<Enum>>(value)));
}

void savegame_write(sector_t *sector) { saveg_write32(sector - sectors); }

sector_t *savegame_read_sector() { return &sectors[saveg_read32()]; }

//
// Structure read/write functions
//

//
// mapthing_t
//

static void saveg_read_mapthing_t(mapthing_t *str) {
  // short x;
  str->x = saveg_read16();

  // short y;
  str->y = saveg_read16();

  // short angle;
  str->angle = saveg_read16();

  // short type;
  str->type = saveg_read16();

  // short options;
  str->options = saveg_read16();
}

static void saveg_write_mapthing_t(mapthing_t *str) {
  // short x;
  saveg_write16(str->x);

  // short y;
  saveg_write16(str->y);

  // short angle;
  saveg_write16(str->angle);

  // short type;
  saveg_write16(str->type);

  // short options;
  saveg_write16(str->options);
}

//
// think_t
//
// This is just an actionf_t.
//

#define saveg_read_think_t saveg_read_actionf_t
#define saveg_write_think_t saveg_write_actionf_t

//
// thinker_t
//
static bool savegame_read_thinker_space() {
  saveg_read32();
  saveg_read32();
  return saveg_read32() != 0;
}

static void savegame_write_thinker_space(bool has_thinker) {
  saveg_write32(0);
  saveg_write32(0);
  saveg_write32(static_cast<int>(has_thinker));
}

//
// mobj_t
//

static mobj_t *saveg_read_mobj_t() {
  int pl;

  auto *mobj = znew<mobj_t>();

  // thinker_t thinker;
  savegame_read_thinker_space();

  // fixed_t x;
  mobj->x = saveg_read32();

  // fixed_t y;
  mobj->y = saveg_read32();

  // fixed_t z;
  mobj->z = saveg_read32();

  // struct mobj_s* snext;
  mobj->snext = static_cast<mobj_t *>(saveg_readp());

  mobj->sprev = static_cast<mobj_t *>(saveg_readp());

  // angle_t angle;
  mobj->angle = saveg_read32();

  // spritenum_t sprite;
  mobj->sprite = savegame_read<spritenum_t>();

  // int frame;
  mobj->frame = saveg_read32();

  // struct mobj_s* bnext;
  mobj->bnext = static_cast<mobj_t *>(saveg_readp());

  // struct mobj_s* bprev;
  mobj->bprev = static_cast<mobj_t *>(saveg_readp());

  // struct subsector_s* subsector;
  mobj->subsector = static_cast<subsector_t *>(saveg_readp());

  // fixed_t floorz;
  mobj->floorz = saveg_read32();

  // fixed_t ceilingz;
  mobj->ceilingz = saveg_read32();

  // fixed_t radius;
  mobj->radius = saveg_read32();

  // fixed_t height;
  mobj->height = saveg_read32();

  // fixed_t momx;
  mobj->momx = saveg_read32();

  // fixed_t momy;
  mobj->momy = saveg_read32();

  // fixed_t momz;
  mobj->momz = saveg_read32();

  // int validcount;
  mobj->validcount = saveg_read32();

  // mobjtype_t type;
  mobj->type = savegame_read<mobjtype_t>();

  // mobjinfo_t* info;
  mobj->info = static_cast<mobjinfo_t *>(saveg_readp());

  // int tics;
  mobj->tics = saveg_read32();

  // state_t* state;
  mobj->state = &states[saveg_read32()];

  // int flags;
  mobj->flags = saveg_read32();

  // int health;
  mobj->health = saveg_read32();

  // int movedir;
  mobj->movedir = saveg_read32();

  // int movecount;
  mobj->movecount = saveg_read32();

  // struct mobj_s* target;
  mobj->target = static_cast<mobj_t *>(saveg_readp());

  // int reactiontime;
  mobj->reactiontime = saveg_read32();

  // int threshold;
  mobj->threshold = saveg_read32();

  // struct player_s* player;
  pl = saveg_read32();

  if (pl > 0) {
    mobj->player = &players[pl - 1];
    mobj->player->mo = mobj;
    mobj->player->so = Crispy_PlayerSO(pl - 1); // [crispy] weapon sound sources
  } else {
    mobj->player = NULL;
  }

  // int lastlook;
  mobj->lastlook = saveg_read32();

  // mapthing_t spawnpoint;
  saveg_read_mapthing_t(&mobj->spawnpoint);

  // struct mobj_s* tracer;
  mobj->tracer = static_cast<mobj_t *>(saveg_readp());

  return mobj;
}

// [crispy] enumerate all thinker pointers
uint32_t thinker_list::index_of(mobj_t *mobj) {
  if (!mobj)
    return 0;
  assert(is_a<mobj_t>(mobj));

  uint32_t i = 0;
  for (auto *th : *this) {
    if (auto *mt = thinker_cast<mobj_t>(th); mt) {
      ++i;
      if (mt == mobj)
        return i;
    }
  }

  return 0;
}

// [crispy] replace indizes with corresponding pointers
mobj_t *thinker_list::mobj_at(uintptr_t index) {
  if (!index)
    return nullptr;

  uint32_t i = 0;
  for (auto *th : *this) {
    if (auto *mobj = thinker_cast<mobj_t>(th); mobj) {
      ++i;
      if (i == index)
        return mobj;
    }
  }

  restoretargets_fail++;
  return nullptr;
}

static void saveg_write_mobj_t(mobj_t *str) {
  savegame_write_thinker_space(true);
  saveg_write32(str->x);
  saveg_write32(str->y);
  saveg_write32(str->z);
  saveg_writep(str->snext);
  saveg_writep(str->sprev);
  saveg_write32(str->angle);
  savegame_write(str->sprite);
  saveg_write32(str->frame);
  saveg_writep(str->bnext);
  saveg_writep(str->bprev);
  saveg_writep(str->subsector);
  saveg_write32(str->floorz);
  saveg_write32(str->ceilingz);
  saveg_write32(str->radius);
  saveg_write32(str->height);
  saveg_write32(str->momx);
  saveg_write32(str->momy);
  saveg_write32(str->momz);
  saveg_write32(str->validcount);
  savegame_write(str->type);
  saveg_writep(str->info);
  saveg_write32(str->tics);
  saveg_write32(str->state - states);
  saveg_write32(str->flags);
  saveg_write32(str->health);
  saveg_write32(str->movedir);
  saveg_write32(str->movecount);
  // [crispy] instead of the actual pointer, store the
  // corresponding index in the mobj->target field
  saveg_write32(thinker_list::instance.index_of(str->target));
  saveg_write32(str->reactiontime);
  saveg_write32(str->threshold);

  // struct player_s* player;
  if (str->player) {
    saveg_write32(str->player - players + 1);
  } else {
    saveg_write32(0);
  }

  saveg_write32(str->lastlook);
  saveg_write_mapthing_t(&str->spawnpoint);

  // [crispy] instead of the actual pointer, store the
  // corresponding index in the mobj->tracers field
  saveg_write32(thinker_list::instance.index_of(str->tracer));
}

//
// ticcmd_t
//

static void saveg_read_ticcmd_t(ticcmd_t *str) {

  // signed char forwardmove;
  str->forwardmove = saveg_read8();

  // signed char sidemove;
  str->sidemove = saveg_read8();

  // short angleturn;
  str->angleturn = saveg_read16();

  // short consistancy;
  str->consistancy = saveg_read16();

  // byte chatchar;
  str->chatchar = saveg_read8();

  // byte buttons;
  str->buttons = saveg_read8();
}

static void saveg_write_ticcmd_t(ticcmd_t *str) {

  // signed char forwardmove;
  saveg_write8(str->forwardmove);

  // signed char sidemove;
  saveg_write8(str->sidemove);

  // short angleturn;
  saveg_write16(str->angleturn);

  // short consistancy;
  saveg_write16(str->consistancy);

  // byte chatchar;
  saveg_write8(str->chatchar);

  // byte buttons;
  saveg_write8(str->buttons);
}

//
// pspdef_t
//

static void saveg_read_pspdef_t(pspdef_t *str) {
  int state;

  // state_t* state;
  state = saveg_read32();

  if (state > 0) {
    str->state = &states[state];
  } else {
    str->state = NULL;
  }

  // int tics;
  str->tics = saveg_read32();

  // fixed_t sx;
  str->sx = saveg_read32();

  // fixed_t sy;
  str->sy = saveg_read32();

  // [crispy] variable weapon sprite bob
  str->dy = 0;
  str->sx2 = str->sx;
  str->sy2 = str->sy;
}

static void saveg_write_pspdef_t(pspdef_t *str) {
  // state_t* state;
  if (str->state) {
    saveg_write32(str->state - states);
  } else {
    saveg_write32(0);
  }

  // int tics;
  saveg_write32(str->tics);

  // fixed_t sx;
  saveg_write32(str->sx);

  // fixed_t sy;
  saveg_write32(str->sy);
}

//
// player_t
//
static void saveg_read_player_t(player_t *str) {
  int i;

  // mobj_t* mo;
  str->mo = static_cast<mobj_t *>(saveg_readp());

  // playerstate_t playerstate;
  str->playerstate = static_cast<playerstate_t>(saveg_read_enum());

  // ticcmd_t cmd;
  saveg_read_ticcmd_t(&str->cmd);

  // fixed_t viewz;
  str->viewz = saveg_read32();

  // fixed_t viewheight;
  str->viewheight = saveg_read32();

  // fixed_t deltaviewheight;
  str->deltaviewheight = saveg_read32();

  // fixed_t bob;
  str->bob = saveg_read32();
  // [crispy] variable player view bob
  str->bob2 = str->bob;

  // int health;
  str->health = saveg_read32();
  // [crispy] negative player health
  str->neghealth = str->health;

  // int armorpoints;
  str->armorpoints = saveg_read32();

  // int armortype;
  str->armortype = saveg_read32();

  // int powers[NUMPOWERS];
  for (i = 0; i < NUMPOWERS; ++i) {
    str->powers[i] = saveg_read32();
  }

  // boolean cards[NUMCARDS];
  for (i = 0; i < NUMCARDS; ++i) {
    str->cards[i] = saveg_read32();
  }

  // boolean backpack;
  str->backpack = saveg_read32();

  // int frags[MAXPLAYERS];
  for (i = 0; i < MAXPLAYERS; ++i) {
    str->frags[i] = saveg_read32();
  }

  // weapontype_t readyweapon;
  str->readyweapon = static_cast<weapontype_t>(saveg_read_enum());

  // weapontype_t pendingweapon;
  str->pendingweapon = static_cast<weapontype_t>(saveg_read_enum());

  // boolean weaponowned[NUMWEAPONS];
  for (i = 0; i < NUMWEAPONS; ++i) {
    str->weaponowned[i] = saveg_read32();
  }

  // int ammo[NUMAMMO];
  for (i = 0; i < NUMAMMO; ++i) {
    str->ammo[i] = saveg_read32();
  }

  // int maxammo[NUMAMMO];
  for (i = 0; i < NUMAMMO; ++i) {
    str->maxammo[i] = saveg_read32();
  }

  // int attackdown;
  str->attackdown = saveg_read32();

  // int usedown;
  str->usedown = saveg_read32();

  // int cheats;
  str->cheats = saveg_read32();

  // int refire;
  str->refire = saveg_read32();

  // int killcount;
  str->killcount = saveg_read32();

  // int itemcount;
  str->itemcount = saveg_read32();

  // int secretcount;
  str->secretcount = saveg_read32();

  // char* message;
  str->message = static_cast<const char *>(saveg_readp());

  // int damagecount;
  str->damagecount = saveg_read32();

  // int bonuscount;
  str->bonuscount = saveg_read32();

  // mobj_t* attacker;
  str->attacker = static_cast<mobj_t *>(saveg_readp());

  // int extralight;
  str->extralight = saveg_read32();

  // int fixedcolormap;
  str->fixedcolormap = saveg_read32();

  // int colormap;
  str->colormap = saveg_read32();

  // pspdef_t psprites[NUMPSPRITES];
  for (i = 0; i < NUMPSPRITES; ++i) {
    saveg_read_pspdef_t(&str->psprites[i]);
  }

  // boolean didsecret;
  str->didsecret = saveg_read32();
}

static void saveg_write_player_t(player_t *str) {
  int i;

  // mobj_t* mo;
  saveg_writep(str->mo);

  // playerstate_t playerstate;
  saveg_write_enum(str->playerstate);

  // ticcmd_t cmd;
  saveg_write_ticcmd_t(&str->cmd);

  // fixed_t viewz;
  saveg_write32(str->viewz);

  // fixed_t viewheight;
  saveg_write32(str->viewheight);

  // fixed_t deltaviewheight;
  saveg_write32(str->deltaviewheight);

  // fixed_t bob;
  saveg_write32(str->bob);

  // int health;
  saveg_write32(str->health);

  // int armorpoints;
  saveg_write32(str->armorpoints);

  // int armortype;
  saveg_write32(str->armortype);

  // int powers[NUMPOWERS];
  for (i = 0; i < NUMPOWERS; ++i) {
    saveg_write32(str->powers[i]);
  }

  // boolean cards[NUMCARDS];
  for (i = 0; i < NUMCARDS; ++i) {
    saveg_write32(str->cards[i]);
  }

  // boolean backpack;
  saveg_write32(str->backpack);

  // int frags[MAXPLAYERS];
  for (i = 0; i < MAXPLAYERS; ++i) {
    saveg_write32(str->frags[i]);
  }

  // weapontype_t readyweapon;
  saveg_write_enum(str->readyweapon);

  // weapontype_t pendingweapon;
  saveg_write_enum(str->pendingweapon);

  // boolean weaponowned[NUMWEAPONS];
  for (i = 0; i < NUMWEAPONS; ++i) {
    saveg_write32(str->weaponowned[i]);
  }

  // int ammo[NUMAMMO];
  for (i = 0; i < NUMAMMO; ++i) {
    saveg_write32(str->ammo[i]);
  }

  // int maxammo[NUMAMMO];
  for (i = 0; i < NUMAMMO; ++i) {
    saveg_write32(str->maxammo[i]);
  }

  // int attackdown;
  saveg_write32(str->attackdown);

  // int usedown;
  saveg_write32(str->usedown);

  // int cheats;
  saveg_write32(str->cheats);

  // int refire;
  saveg_write32(str->refire);

  // int killcount;
  saveg_write32(str->killcount);

  // int itemcount;
  saveg_write32(str->itemcount);

  // int secretcount;
  saveg_write32(str->secretcount);

  // char* message;
  saveg_writep(str->message);

  // int damagecount;
  saveg_write32(str->damagecount);

  // int bonuscount;
  saveg_write32(str->bonuscount);

  // mobj_t* attacker;
  saveg_writep(str->attacker);

  // int extralight;
  saveg_write32(str->extralight);

  // int fixedcolormap;
  saveg_write32(str->fixedcolormap);

  // int colormap;
  saveg_write32(str->colormap);

  // pspdef_t psprites[NUMPSPRITES];
  for (i = 0; i < NUMPSPRITES; ++i) {
    saveg_write_pspdef_t(&str->psprites[i]);
  }

  // boolean didsecret;
  saveg_write32(str->didsecret);
}

template <typename T> static T *saveg_read();

//
// ceiling_t
//
template <> ceiling_t *saveg_read() {
  bool has_thinker = savegame_read_thinker_space();
  auto *ceiling = znew<ceiling_t>(has_thinker);

  ceiling->type = savegame_read<ceiling_e>();
  ceiling->sector = savegame_read_sector();
  ceiling->bottomheight = saveg_read32();
  ceiling->topheight = saveg_read32();
  ceiling->speed = saveg_read32();
  ceiling->crush = saveg_read32();
  ceiling->direction = saveg_read32();
  ceiling->tag = saveg_read32();
  ceiling->olddirection = saveg_read32();

  return ceiling;
}

static void saveg_write(ceiling_t *str) {
  savegame_write_thinker_space(str->has_any_action());
  savegame_write(str->type);
  savegame_write(str->sector);
  saveg_write32(str->bottomheight);
  saveg_write32(str->topheight);
  saveg_write32(str->speed);
  saveg_write32(str->crush);
  saveg_write32(str->direction);
  saveg_write32(str->tag);
  saveg_write32(str->olddirection);
}

//
// vldoor_t
//
template <> vldoor_t *saveg_read() {
  savegame_read_thinker_space();
  auto *door = znew<vldoor_t>();
  door->type = savegame_read<vldoor_e>();
  door->sector = savegame_read_sector();
  door->topheight = saveg_read32();
  door->speed = saveg_read32();
  door->direction = saveg_read32();
  door->topwait = saveg_read32();
  door->topcountdown = saveg_read32();

  return door;
}

static void saveg_write(vldoor_t *str) {
  savegame_write_thinker_space(true);
  savegame_write(str->type);
  savegame_write(str->sector);
  saveg_write32(str->topheight);
  saveg_write32(str->speed);
  saveg_write32(str->direction);
  saveg_write32(str->topwait);
  saveg_write32(str->topcountdown);
}

//
// floormove_t
//
template <> floormove_t *saveg_read() {
  savegame_read_thinker_space();
  auto *floor = znew<floormove_t>();
  floor->type = savegame_read<floor_e>();
  floor->crush = saveg_read32();
  floor->sector = savegame_read_sector();
  floor->direction = saveg_read32();
  floor->newspecial = saveg_read32();
  floor->texture = saveg_read16();
  floor->floordestheight = saveg_read32();
  floor->speed = saveg_read32();

  return floor;
}

static void saveg_write(floormove_t *str) {
  savegame_write_thinker_space(true);
  savegame_write(str->type);
  saveg_write32(str->crush);
  savegame_write(str->sector);
  saveg_write32(str->direction);
  saveg_write32(str->newspecial);
  saveg_write16(str->texture);
  saveg_write32(str->floordestheight);
  saveg_write32(str->speed);
}

//
// plat_t
//
template <> plat_t *saveg_read() {
  bool has_thinker = savegame_read_thinker_space();
  auto *platform = znew<plat_t>(has_thinker);
  platform->sector = savegame_read_sector();
  platform->speed = saveg_read32();
  platform->low = saveg_read32();
  platform->high = saveg_read32();
  platform->wait = saveg_read32();
  platform->count = saveg_read32();
  platform->status = savegame_read<plat_e>();
  platform->oldstatus = savegame_read<plat_e>();
  platform->crush = saveg_read32();
  platform->tag = saveg_read32();
  platform->type = savegame_read<plattype_e>();

  return platform;
}

static void saveg_write(plat_t *str) {
  savegame_write_thinker_space(str->has_any_action());
  savegame_write(str->sector);
  saveg_write32(str->speed);
  saveg_write32(str->low);
  saveg_write32(str->high);
  saveg_write32(str->wait);
  saveg_write32(str->count);
  savegame_write(str->status);
  savegame_write(str->oldstatus);
  saveg_write32(str->crush);
  saveg_write32(str->tag);
  savegame_write(str->type);
}

//
// lightflash_t
//
template <> lightflash_t *saveg_read() {
  savegame_read_thinker_space();
  auto *sector = savegame_read_sector();
  auto count = saveg_read32();
  auto maxlight = saveg_read32();
  auto minlight = saveg_read32();
  auto maxtime = saveg_read32();
  auto mintime = saveg_read32();

  return znew<lightflash_t>(basic_light{sector, maxlight, minlight}, count,
                            maxtime, mintime);
}

static void saveg_write(lightflash_t *str) {
  savegame_write_thinker_space(true);
  savegame_write(str->sector);
  saveg_write32(str->count);
  saveg_write32(str->maxlight);
  saveg_write32(str->minlight);
  saveg_write32(str->maxtime);
  saveg_write32(str->mintime);
}

//
// strobe_t
//
template <> strobe_t *saveg_read() {
  savegame_read_thinker_space();

  auto *sector = savegame_read_sector();
  auto count = saveg_read32();
  auto minlight = saveg_read32();
  auto maxlight = saveg_read32();
  auto darktime = saveg_read32();
  auto brighttime = saveg_read32();

  return znew<strobe_t>(basic_light{sector, maxlight, minlight}, count,
                        darktime, brighttime);
}

static void saveg_write(strobe_t *str) {
  savegame_write_thinker_space(true);
  savegame_write(str->sector);
  saveg_write32(str->count);
  saveg_write32(str->minlight);
  saveg_write32(str->maxlight);
  saveg_write32(str->darktime);
  saveg_write32(str->brighttime);
}

//
// glow_t
//
template <> glow_t *saveg_read() {
  savegame_read_thinker_space();
  auto* sector = savegame_read_sector();
  auto minlight = saveg_read32();
  auto maxlight = saveg_read32();
  auto direction = savegame_read<glow_t::direction>();

  return znew<glow_t>(basic_light{sector, maxlight, minlight}, direction);
}

static void saveg_write(glow_t *str) {
  savegame_write_thinker_space(false);
  savegame_write(str->sector);
  saveg_write32(str->minlight);
  saveg_write32(str->maxlight);
  savegame_write(str->dir);
}

template <typename T> static void saveg_read_special() {
  saveg_read_pad();
  auto *special = saveg_read<T>();
  special->sector->specialdata = special;
  thinker_list::instance.push_back(special);
}

template <typename T> static void saveg_read_light() {
  saveg_read_pad();
  auto *special = saveg_read<T>();
  thinker_list::instance.push_back(special);
}

template <typename T> static T *saveg_read_special_movable() {
  saveg_read_pad();
  auto *special = saveg_read<T>();
  special->sector->specialdata = special;

  if (!special->has_any_action())
    special->stop_moving();

  thinker_list::instance.push_back(special);
  return special;
}

//
// Write the header for a savegame
//

void P_WriteSaveGameHeader(char *description) {
  char name[VERSIONSIZE];
  int i;

  for (i = 0; description[i] != '\0'; ++i)
    saveg_write8(description[i]);
  for (; i < SAVESTRINGSIZE; ++i)
    saveg_write8(0);

  memset(name, 0, sizeof(name));
  M_snprintf(name, sizeof(name), "version %i", G_VanillaVersionCode());

  for (i = 0; i < VERSIONSIZE; ++i)
    saveg_write8(name[i]);

  saveg_write8(gameskill);
  saveg_write8(gameepisode);
  saveg_write8(gamemap);

  for (i = 0; i < MAXPLAYERS; i++)
    saveg_write8(playeringame[i]);

  saveg_write8((leveltime >> 16) & 0xff);
  saveg_write8((leveltime >> 8) & 0xff);
  saveg_write8(leveltime & 0xff);
}

//
// Read the header for a savegame
//

boolean P_ReadSaveGameHeader(void) {
  int i;
  byte a, b, c;
  char vcheck[VERSIONSIZE];
  char read_vcheck[VERSIONSIZE];

  // skip the description field

  for (i = 0; i < SAVESTRINGSIZE; ++i)
    saveg_read8();

  for (i = 0; i < VERSIONSIZE; ++i)
    read_vcheck[i] = saveg_read8();

  memset(vcheck, 0, sizeof(vcheck));
  M_snprintf(vcheck, sizeof(vcheck), "version %i", G_VanillaVersionCode());
  if (strcmp(read_vcheck, vcheck) != 0)
    return false; // bad version

  gameskill = static_cast<skill_t>(saveg_read8());
  gameepisode = saveg_read8();
  gamemap = saveg_read8();

  for (i = 0; i < MAXPLAYERS; i++)
    playeringame[i] = saveg_read8();

  // get the times
  a = saveg_read8();
  b = saveg_read8();
  c = saveg_read8();
  leveltime = (a << 16) + (b << 8) + c;

  return true;
}

//
// Read the end of file marker.  Returns true if read successfully.
//

boolean P_ReadSaveGameEOF(void) {
  int value;

  value = saveg_read8();

  return value == SAVEGAME_EOF;
}

//
// Write the end of file marker
//

void P_WriteSaveGameEOF(void) { saveg_write8(SAVEGAME_EOF); }

//
// P_ArchivePlayers
//
void P_ArchivePlayers(void) {
  int i;

  for (i = 0; i < MAXPLAYERS; i++) {
    if (!playeringame[i])
      continue;

    saveg_write_pad();

    saveg_write_player_t(&players[i]);
  }
}

//
// P_UnArchivePlayers
//
void P_UnArchivePlayers(void) {
  int i;

  for (i = 0; i < MAXPLAYERS; i++) {
    if (!playeringame[i])
      continue;

    saveg_read_pad();

    saveg_read_player_t(&players[i]);

    // will be set when unarc thinker
    players[i].mo = NULL;
    players[i].message = NULL;
    players[i].attacker = NULL;
  }
}

//
// P_ArchiveWorld
//
void P_ArchiveWorld(void) {
  int i;
  int j;
  sector_t *sec;
  line_t *li;
  side_t *si;

  // do sectors
  for (i = 0, sec = sectors; i < numsectors; i++, sec++) {
    saveg_write16(sec->floorheight >> FRACBITS);
    saveg_write16(sec->ceilingheight >> FRACBITS);
    saveg_write16(sec->floorpic);
    saveg_write16(sec->ceilingpic);
    saveg_write16(sec->lightlevel);
    saveg_write16(sec->special); // needed?
    saveg_write16(sec->tag);     // needed?
  }

  // do lines
  for (i = 0, li = lines; i < numlines; i++, li++) {
    saveg_write16(li->flags);
    saveg_write16(li->special);
    saveg_write16(li->tag);
    for (j = 0; j < 2; j++) {
      if (li->sidenum[j] == NO_INDEX) // [crispy] extended nodes
        continue;

      si = &sides[li->sidenum[j]];

      saveg_write16(si->textureoffset >> FRACBITS);
      saveg_write16(si->rowoffset >> FRACBITS);
      saveg_write16(si->toptexture);
      saveg_write16(si->bottomtexture);
      saveg_write16(si->midtexture);
    }
  }
}

//
// P_UnArchiveWorld
//
void P_UnArchiveWorld(void) {
  int i;
  int j;
  sector_t *sec;
  line_t *li;
  side_t *si;

  // do sectors
  for (i = 0, sec = sectors; i < numsectors; i++, sec++) {
    // [crispy] add overflow guard for the flattranslation[] array
    short floorpic, ceilingpic;
    extern int numflats;
    sec->floorheight = saveg_read16() << FRACBITS;
    sec->ceilingheight = saveg_read16() << FRACBITS;
    floorpic = saveg_read16();
    ceilingpic = saveg_read16();
    sec->lightlevel = saveg_read16();
    sec->special = saveg_read16(); // needed?
    sec->tag = saveg_read16();     // needed?
    sec->specialdata = 0;
    sec->soundtarget = 0;
    // [crispy] add overflow guard for the flattranslation[] array
    if (floorpic >= 0 && floorpic < numflats) {
      sec->floorpic = floorpic;
    }
    if (ceilingpic >= 0 && ceilingpic < numflats) {
      sec->ceilingpic = ceilingpic;
    }
  }

  // do lines
  for (i = 0, li = lines; i < numlines; i++, li++) {
    li->flags = saveg_read16();
    li->special = saveg_read16();
    li->tag = saveg_read16();
    for (j = 0; j < 2; j++) {
      if (li->sidenum[j] == NO_INDEX) // [crispy] extended nodes
        continue;
      si = &sides[li->sidenum[j]];
      si->textureoffset = saveg_read16() << FRACBITS;
      si->rowoffset = saveg_read16() << FRACBITS;
      si->toptexture = saveg_read16();
      si->bottomtexture = saveg_read16();
      si->midtexture = saveg_read16();
    }
  }
}

//
// Thinkers
//
enum thinkerclass_t {
  tc_end,
  tc_mobj

};

//
// P_ArchiveThinkers
//
void P_ArchiveThinkers(void) {
  // save off the current thinkers
  for (auto *th : thinker_list::instance) {
    if (auto *const mo = thinker_cast<mobj_t>(th); mo) {
      saveg_write8(tc_mobj);
      saveg_write_pad();
      saveg_write_mobj_t(mo);

      continue;
    }

    // I_Error ("P_ArchiveThinkers: Unknown thinker function");
  }

  // add a terminating marker
  saveg_write8(tc_end);
}

//
// P_UnArchiveThinkers
//
void P_UnArchiveThinkers(void) {
  byte tclass;

  // remove all the current thinkers
  for (auto *currentthinker : thinker_list::instance) {
    if (auto *mo = thinker_cast<mobj_t>(currentthinker); mo)
      P_RemoveMobj(mo);
    else
      Z_Free(currentthinker);
  }
  thinker_list::instance = {};

  // read in saved thinkers
  while (1) {
    tclass = saveg_read8();
    switch (tclass) {
    case tc_end:
      return; // end of list

    case tc_mobj: {
      saveg_read_pad();
      auto *mobj = saveg_read_mobj_t();

      // [crispy] restore mobj->target and mobj->tracer fields
      // mobj->target = NULL;
      // mobj->tracer = NULL;
      P_SetThingPosition(mobj);
      mobj->info = &mobjinfo[mobj->type];
      // [crispy] killough 2/28/98: Fix for falling down into a wall after
      // savegame loaded
      //	    mobj->floorz = mobj->subsector->sector->floorheight;
      //	    mobj->ceilingz = mobj->subsector->sector->ceilingheight;

      thinker_list::instance.push_back(mobj);
      break;
    }

    default:
      I_Error("Unknown tclass %i in savegame", tclass);
    }
  }
}

// [crispy] after all the thinkers have been restored, replace all indices in
// the mobj->target and mobj->tracers fields by the corresponding current
// pointers again
void P_RestoreTargets(void) {
  for (auto *th : thinker_list::instance) {
    if (auto *mo = thinker_cast<mobj_t>(th); mo) {
      mo->target = thinker_list::instance.mobj_at(
          reinterpret_cast<uintptr_t>(mo->target));
      mo->tracer = thinker_list::instance.mobj_at(
          reinterpret_cast<uintptr_t>(mo->tracer));
    }
  }

  if (restoretargets_fail) {
    fprintf(stderr,
            "P_RestoreTargets: Failed to restore %d/%d target pointers.\n",
            restoretargets_fail,
            static_cast<int>(std::distance(thinker_list::instance.begin(),
                                           thinker_list::instance.end())));
    restoretargets_fail = 0;
  }
}

//
// P_ArchiveSpecials
//
enum specials_e {
  tc_ceiling,
  tc_door,
  tc_floor,
  tc_plat,
  tc_flash,
  tc_strobe,
  tc_glow,
  tc_endspecials
};

specials_e specialstype(ceiling_t *) { return tc_ceiling; }
specials_e specialstype(vldoor_t *) { return tc_door; }
specials_e specialstype(floormove_t *) { return tc_floor; }
specials_e specialstype(plat_t *) { return tc_plat; }
specials_e specialstype(lightflash_t *) { return tc_flash; }
specials_e specialstype(strobe_t *) { return tc_strobe; }
specials_e specialstype(glow_t *) { return tc_glow; }

template <typename T> void saveg_write_special(T *special) {
  saveg_write8(specialstype(special));
  saveg_write_pad();
  saveg_write(special);
}

//
// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
//
void P_ArchiveSpecials(void) {
  // save off the current thinkers
  for (auto *th : thinker_list::instance) {
    if (auto *movable = thinker_cast<movable_obj>(th);
        movable && !movable->has_any_action()) {
      auto ceilIt = std::find(activeceilings.begin(), activeceilings.end(),
                              reinterpret_cast<ceiling_t *>(th));
      if (ceilIt != activeceilings.end())
        saveg_write_special(*ceilIt);

      // [crispy] save plats in statis
      auto platIt = std::find(std::begin(activeplats), std::end(activeplats),
                              reinterpret_cast<plat_t *>(th));
      if (platIt != std::end(activeplats))
        saveg_write_special(reinterpret_cast<plat_t *>(th));
    } else if (auto *ceiling = thinker_cast<ceiling_t>(th); ceiling) {
      saveg_write_special(ceiling);
    } else if (auto *door = thinker_cast<vldoor_t>(th); door) {
      saveg_write_special(door);
    } else if (auto *floor = thinker_cast<floormove_t>(th); floor) {
      saveg_write_special(floor);
    } else if (auto *plat = thinker_cast<plat_t>(th); plat) {
      saveg_write_special(plat);
    } else if (auto *flash = thinker_cast<lightflash_t>(th); flash) {
      saveg_write_special(flash);
    } else if (auto *strobe = thinker_cast<strobe_t>(th); strobe) {
      saveg_write_special(strobe);
    } else if (auto *glow = thinker_cast<glow_t>(th); glow) {
      saveg_write_special(glow);
    }
  }

  // add a terminating marker
  saveg_write8(tc_endspecials);
}

//
// P_UnArchiveSpecials
//
void P_UnArchiveSpecials(void) {
  byte tclass;

  // read in saved thinkers
  while (1) {
    tclass = saveg_read8();

    switch (tclass) {
    case tc_endspecials:
      return; // end of list

    case tc_ceiling: {
      auto *ceiling = saveg_read_special_movable<ceiling_t>();
      P_AddActiveCeiling(ceiling);
      break;
    }

    case tc_door: {
      saveg_read_special<vldoor_t>();
      break;
    }

    case tc_floor: {
      saveg_read_special<floormove_t>();
      break;
    }

    case tc_plat: {
      auto *plat = saveg_read_special_movable<plat_t>();
      P_AddActivePlat(plat);
      break;
    }

    case tc_flash: {
      saveg_read_light<lightflash_t>();
      break;
    }

    case tc_strobe: {
      saveg_read_light<strobe_t>();
      break;
    }

    case tc_glow: {
      saveg_read_light<glow_t>();
      break;
    }

    default:
      I_Error("P_UnarchiveSpecials:Unknown tclass %i "
              "in savegame",
              tclass);
    }
  }
}
