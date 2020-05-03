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
// DESCRIPTION:  none
//	Implements special effects:
//	Texture animation, height or lighting changes
//	 according to adjacent sectors, respective
//	 utility functions, etc.
//

#ifndef __P_SPEC__
#define __P_SPEC__

#include <array>

//
// End-level timer (-TIMER option)
//
extern boolean levelTimer;
extern int levelTimeCount;

//      Define values for map objects
#define MO_TELEPORTMAN 14

// at game start
void P_InitPicAnims(void);

// at map load
void P_SpawnSpecials(void);

// every tic
void P_UpdateSpecials(void);

// when needed
boolean P_UseSpecialLine(mobj_t *thing, line_t *line, int side);

void P_ShootSpecialLine(mobj_t *thing, line_t *line);

void P_CrossSpecialLine(int linenum, int side, mobj_t *thing);

// [crispy] more MBF code pointers
void P_CrossSpecialLinePtr(line_t *line, int side, mobj_t *thing);

void P_PlayerInSpecialSector(player_t *player);

int twoSided(int sector, int line);

sector_t *getSector(int currentSector, int line, int side);

side_t *getSide(int currentSector, int line, int side);

fixed_t P_FindLowestFloorSurrounding(sector_t *sec);
fixed_t P_FindHighestFloorSurrounding(sector_t *sec);

fixed_t P_FindNextHighestFloor(sector_t *sec, int currentheight);

fixed_t P_FindLowestCeilingSurrounding(sector_t *sec);
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec);

int P_FindSectorFromLineTag(line_t *line, int start);

int P_FindMinSurroundingLight(sector_t *sector, int max);

sector_t *getNextSector(line_t *line, sector_t *sec);

//
// SPECIAL
//
int EV_DoDonut(line_t *line);

//
// P_LIGHTS
//
struct fireflicker_t : mobj_thinker {
  fireflicker_t();
  sector_t *sector;
  int count;
  int maxlight;
  int minlight;
};

void T_FireFlicker(fireflicker_t *);

inline fireflicker_t::fireflicker_t() : mobj_thinker(T_FireFlicker) {}

template <> struct thinker_trait<fireflicker_t> {
  constexpr const static auto func = T_FireFlicker;
};

struct lightflash_t : mobj_thinker {
  lightflash_t();

  sector_t *sector;
  int count;
  int maxlight;
  int minlight;
  int maxtime;
  int mintime;
};

struct strobe_t : mobj_thinker {
  strobe_t();
  sector_t *sector;
  int count;
  int minlight;
  int maxlight;
  int darktime;
  int brighttime;
};

struct glow_t : mobj_thinker {
  glow_t();
  sector_t *sector;
  int minlight;
  int maxlight;
  int direction;
};

#define GLOWSPEED 8
#define STROBEBRIGHT 5
#define FASTDARK 15
#define SLOWDARK 35

void P_SpawnFireFlicker(sector_t *sector);
void T_LightFlash(lightflash_t *flash);
void P_SpawnLightFlash(sector_t *sector);
void T_StrobeFlash(strobe_t *flash);

void P_SpawnStrobeFlash(sector_t *sector, int fastOrSlow, int inSync);

void EV_StartLightStrobing(line_t *line);
void EV_TurnTagLightsOff(line_t *line);

void EV_LightTurnOn(line_t *line, int bright);

void T_Glow(glow_t *g);
void P_SpawnGlowingLight(sector_t *sector);

template <> struct thinker_trait<lightflash_t> {
  constexpr const static auto func = T_LightFlash;
};

template <> struct thinker_trait<strobe_t> {
  constexpr const static auto func = T_StrobeFlash;
};

template <> struct thinker_trait<glow_t> {
  constexpr const static auto func = T_Glow;
};

inline lightflash_t::lightflash_t() : mobj_thinker(T_LightFlash) {}

inline strobe_t::strobe_t() : mobj_thinker(T_StrobeFlash) {}

inline glow_t::glow_t() : mobj_thinker(T_Glow) {}

//
// P_SWITCH
//
// [crispy] add PACKEDATTR for reading SWITCHES lumps from memory
typedef PACKED_STRUCT({
  char name1[9];
  char name2[9];
  short episode;
}) switchlist_t;

enum bwhere_e {
  top,
  middle,
  bottom

};

struct button_t {
  line_t *line;
  bwhere_e where;
  int btexture;
  int btimer;
  degenmobj_t *soundorg;
};

// max # of wall switches in a level
#define MAXSWITCHES 50

// 4 players, 4 buttons each at once, max.
#define MAXBUTTONS 16

// 1 second, in ticks.
#define BUTTONTIME 35

extern button_t *buttonlist;
extern int maxbuttons;

void P_ChangeSwitchTexture(line_t *line, int useAgain);

void P_InitSwitchList(void);

//
// P_PLATS
//
enum plat_e {
  up,
  down,
  waiting,
  in_stasis

};

enum plattype_e {
  perpetualRaise,
  downWaitUpStay,
  raiseAndChange,
  raiseToNearestAndChange,
  blazeDWUS

};

struct plat_t;
extern void T_PlatRaise(plat_t *);

struct plat_t : mobj_thinker {
  plat_t(bool has_thinker = true)
      : mobj_thinker(has_thinker ? T_PlatRaise : nullptr) {}

  void start_moving() { set_thinker(T_PlatRaise); }
  void stop_moving() { reset(); }

  sector_t *sector;
  fixed_t speed;
  fixed_t low;
  fixed_t high;
  int wait;
  int count;
  plat_e status;
  plat_e oldstatus;
  boolean crush;
  int tag;
  plattype_e type;
};

#define PLATWAIT 3
#define PLATSPEED FRACUNIT
#define MAXPLATS 30 * 256

extern std::array<plat_t *, MAXPLATS> activeplats;

void T_PlatRaise(plat_t *plat);

template <> struct thinker_trait<plat_t> {
  constexpr const static auto func = T_PlatRaise;
};

int EV_DoPlat(line_t *line, plattype_e type, int amount);

void P_AddActivePlat(plat_t *plat);
void P_RemoveActivePlat(plat_t *plat);
void EV_StopPlat(line_t *line);
void P_ActivateInStasis(int tag);

//
// P_DOORS
//
enum vldoor_e {
  vld_normal,
  vld_close30ThenOpen,
  vld_close,
  vld_open,
  vld_raiseIn5Mins,
  vld_blazeRaise,
  vld_blazeOpen,
  vld_blazeClose

};

struct vldoor_t;
extern void T_VerticalDoor(vldoor_t *);

struct vldoor_t : mobj_thinker {
  vldoor_e type;
  sector_t *sector;
  fixed_t topheight;
  fixed_t speed;

  vldoor_t() : mobj_thinker(T_VerticalDoor) {}

  // 1 = up, 0 = waiting at top, -1 = down
  int direction;

  // tics to wait at the top
  int topwait;
  // (keep in case a door going down is reset)
  // when it reaches 0, start going down
  int topcountdown;

  // void think() override;
};

extern void T_VerticalDoor(vldoor_t *);

template <> struct thinker_trait<vldoor_t> {
  constexpr const static auto func = T_VerticalDoor;
};

#define VDOORSPEED FRACUNIT * 2
#define VDOORWAIT 150

void EV_VerticalDoor(line_t *line, mobj_t *thing);

int EV_DoDoor(line_t *line, vldoor_e type);

int EV_DoLockedDoor(line_t *line, vldoor_e type, mobj_t *thing);

void T_VerticalDoor(vldoor_t *door);
void P_SpawnDoorCloseIn30(sector_t *sec);

void P_SpawnDoorRaiseIn5Mins(sector_t *sec, int secnum);

#if 0 // UNUSED
//
//      Sliding doors...
//
typedef enum
{
    sd_opening,
    sd_waiting,
    sd_closing

} sd_e;



typedef enum
{
    sdt_openOnly,
    sdt_closeOnly,
    sdt_openAndClose

} sdt_e;




typedef struct
{
    thinker_t	thinker;
    sdt_e	type;
    line_t*	line;
    int		frame;
    int		whichDoorIndex;
    int		timer;
    sector_t*	frontsector;
    sector_t*	backsector;
    sd_e	 status;

} slidedoor_t;



typedef struct
{
    char	frontFrame1[9];
    char	frontFrame2[9];
    char	frontFrame3[9];
    char	frontFrame4[9];
    char	backFrame1[9];
    char	backFrame2[9];
    char	backFrame3[9];
    char	backFrame4[9];
    
} slidename_t;



typedef struct
{
    int             frontFrames[4];
    int             backFrames[4];

} slideframe_t;



// how many frames of animation
#define SNUMFRAMES 4

#define SDOORWAIT 35 * 3
#define SWAITTICS 4

// how many diff. types of anims
#define MAXSLIDEDOORS 5

void P_InitSlidingDoorFrames(void);

void
EV_SlidingDoor
( line_t*	line,
  mobj_t*	thing );
#endif

//
// P_CEILNG
//
enum ceiling_e {
  lowerToFloor,
  raiseToHighest,
  lowerAndCrush,
  crushAndRaise,
  fastCrushAndRaise,
  silentCrushAndRaise

};

struct ceiling_t;
extern void T_MoveCeiling(ceiling_t *);

struct ceiling_t : mobj_thinker {
  ceiling_t(bool has_thinker = true) : mobj_thinker{has_thinker ? T_MoveCeiling : nullptr} {}

  void start_moving() { set_thinker(T_MoveCeiling); }
  void stop_moving() { reset(); }

  ceiling_e type;
  sector_t *sector;
  fixed_t bottomheight;
  fixed_t topheight;
  fixed_t speed;
  boolean crush;

  // 1 = up, 0 = waiting, -1 = down
  int direction;

  // ID
  int tag;
  int olddirection;

  // void think() override;
};

#define CEILSPEED FRACUNIT
#define CEILWAIT 150
#define MAXCEILINGS 30

extern std::array<ceiling_t *, MAXCEILINGS> activeceilings;

int EV_DoCeiling(line_t *line, ceiling_e type);

void T_MoveCeiling(ceiling_t *ceiling);
void P_AddActiveCeiling(ceiling_t *c);
void P_RemoveActiveCeiling(ceiling_t *c);
int EV_CeilingCrushStop(line_t *line);
void P_ActivateInStasisCeiling(line_t *line);

template <> struct thinker_trait<ceiling_t> {
  constexpr const static auto func = T_MoveCeiling;
};

//
// P_FLOOR
//
enum floor_e {
  // lower floor to highest surrounding floor
  lowerFloor,

  // lower floor to lowest surrounding floor
  lowerFloorToLowest,

  // lower floor to highest surrounding floor VERY FAST
  turboLower,

  // raise floor to lowest surrounding CEILING
  raiseFloor,

  // raise floor to next highest surrounding floor
  raiseFloorToNearest,

  // raise floor to shortest height texture around it
  raiseToTexture,

  // lower floor to lowest surrounding floor
  //  and change floorpic
  lowerAndChange,

  raiseFloor24,
  raiseFloor24AndChange,
  raiseFloorCrush,

  // raise to next highest floor, turbo-speed
  raiseFloorTurbo,
  donutRaise,
  raiseFloor512

};

enum stair_e {
  build8, // slowly build by 8
  turbo16 // quickly build by 16

};

struct floormove_t;
extern void T_MoveFloor(floormove_t *);

struct floormove_t : mobj_thinker {
  floormove_t() : mobj_thinker(T_MoveFloor) {}
  floormove_t(actionf_t action) : mobj_thinker(action) {}

  floor_e type;
  boolean crush;
  sector_t *sector;
  int direction;
  int newspecial;
  short texture;
  fixed_t floordestheight;
  fixed_t speed;
};

#define FLOORSPEED FRACUNIT

enum result_e {
  ok,
  crushed,
  pastdest

};

result_e T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest,
                     boolean crush, int floorOrCeiling, int direction);

int EV_BuildStairs(line_t *line, stair_e type);

int EV_DoFloor(line_t *line, floor_e floortype);

void T_MoveFloor(floormove_t *floor);

template <> struct thinker_trait<floormove_t> {
  constexpr const static auto func = T_MoveFloor;
};

//
// P_TELEPT
//
int EV_Teleport(line_t *line, int side, mobj_t *thing);

#endif
