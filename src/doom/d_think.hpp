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
//  MapObj data. Map Objects or mobjs are actors, entities,
//  thinker, take-your-pick... anything that moves, acts, or
//  suffers state changes of more or less violent nature.
//


#ifndef __D_THINK__
#define __D_THINK__





//
// Experimental stuff.
// To compile this as "ANSI C with classes"
//  we will need to handle the various
//  action functions cleanly.
//

struct mobj_t;
struct player_t;
struct pspdef_t;

typedef  void (*actionf_v)();
typedef  void (*actionf_p1)(mobj_t *mo );
typedef  void (*actionf_p2)(player_t *player, pspdef_t *psp );
typedef  void (*actionf_p3)(mobj_t *mo, player_t *player, pspdef_t *psp); // [crispy] let pspr action pointers get called from mobj states

union actionf_t
{
    actionf_v	acv_;
    actionf_p1	acp1_;
    actionf_p2	acp2_;
    actionf_p3	acp3_; // [crispy] let pspr action pointers get called from mobj states
public:

    auto& acv() { return acv_; }
    auto acv() const { return acv_; }
    auto& acp1() { return acp1_; }
    auto& acp2() { return acp2_; }
    auto& acp3() { return acp3_; }

    actionf_t(int f) : acv_{reinterpret_cast<actionf_v>(f)} {}
    actionf_t() : acv_{nullptr} {}
    actionf_t(actionf_v f) : acv_{f} {}
    actionf_t(actionf_p1 f) : acp1_{f} {}
    actionf_t(actionf_p2 f) : acp2_{f} {}
    actionf_t(actionf_p3 f) : acp3_{f} {}

    constexpr actionf_t& operator=(actionf_v const& f)
    {
        acv_ = f;
        return *this;
    }
};





// Historically, "think_t" is yet another
//  function pointer to a routine to handle
//  an actor.
typedef actionf_t  think_t;


// Doubly linked list of actors.
typedef struct thinker_s
{
    struct thinker_s*	prev;
    struct thinker_s*	next;
    think_t		function;
    
} thinker_t;



#endif
