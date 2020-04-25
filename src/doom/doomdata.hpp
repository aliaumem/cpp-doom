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
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//

#ifndef __DOOMDATA__
#define __DOOMDATA__

// The most basic types we use, portability.
#include "doomtype.hpp"

// Some global defines, that configure the game.
#include "doomdef.hpp"



//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
  ML_LABEL,		// A separator, name, ExMx or MAPxx
  ML_THINGS,		// Monsters, items..
  ML_LINEDEFS,		// LineDefs, from editing
  ML_SIDEDEFS,		// SideDefs, from editing
  ML_VERTEXES,		// Vertices, edited and BSP splits generated
  ML_SEGS,		// LineSegs, from LineDefs split by BSP
  ML_SSECTORS,		// SubSectors, list of LineSegs
  ML_NODES,		// BSP nodes
  ML_SECTORS,		// Sectors, from editing
  ML_REJECT,		// LUT, sector-sector visibility	
  ML_BLOCKMAP		// LUT, motion clipping, walls/grid element
};


// A single Vertex.
PACKED_STRUCT(mapvertex_t
{
  short		x;
  short		y;
});


// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
PACKED_STRUCT(mapsidedef_t
{
  short		textureoffset;
  short		rowoffset;
  char		toptexture[8];
  char		bottomtexture[8];
  char		midtexture[8];
  // Front sector, towards viewer.
  short		sector;
});



// A LineDef, as used for editing, and as input
// to the BSP builder.
PACKED_STRUCT(maplinedef_t
{
  unsigned short		v1; // [crispy] extended nodes
  unsigned short		v2; // [crispy] extended nodes
  unsigned short		flags; // [crispy] extended nodes
  short		special;
  short		tag;
  // sidenum[1] will be -1 (NO_INDEX) if one sided
  unsigned short		sidenum[2]; // [crispy] extended nodes
});

// [crispy] allow loading of Hexen-format maps
// taken from chocolate-doom/src/hexen/xddefs.h:63-75
PACKED_STRUCT(maplinedef_hexen_t
{
    short v1;
    short v2;
    short flags;
    byte special;
    byte arg1;
    byte arg2;
    byte arg3;
    byte arg4;
    byte arg5;
    short sidenum[2];
});

//
// LineDef attributes.
//

// Solid, is an obstacle.
#define ML_BLOCKING		1

// Blocks monsters only.
#define ML_BLOCKMONSTERS	2

// Backside will not be present at all
//  if not two sided.
#define ML_TWOSIDED		4

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures allways have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).

// upper texture unpegged
#define ML_DONTPEGTOP		8

// lower texture unpegged
#define ML_DONTPEGBOTTOM	16	

// In AutoMap: don't map as two sided: IT'S A SECRET!
#define ML_SECRET		32

// Sound rendering: don't let sound cross two of these.
#define ML_SOUNDBLOCK		64

// Don't draw on the automap at all.
#define ML_DONTDRAW		128

// Set if already seen, thus drawn in automap.
#define ML_MAPPED		256




// Sector definition, from editing.
PACKED_STRUCT(mapsector_t
{
  short		floorheight;
  short		ceilingheight;
  char		floorpic[8];
  char		ceilingpic[8];
  short		lightlevel;
  short		special;
  short		tag;
});

// SubSector, as generated by BSP.
PACKED_STRUCT(mapsubsector_t
{
  unsigned short		numsegs; // [crispy] extended nodes
  // Index of first one, segs are stored sequentially.
  unsigned short		firstseg; // [crispy] extended nodes
});

// [crispy] allow loading of maps with DeePBSP nodes
// taken from prboom-plus/src/doomdata.h:163-166
PACKED_STRUCT(mapsubsector_deepbsp_t
{
    unsigned short numsegs;
    int firstseg;
});

// [crispy] allow loading of maps with ZDBSP nodes
// taken from prboom-plus/src/doomdata.h:168-170
PACKED_STRUCT(mapsubsector_zdbsp_t
{
    unsigned int numsegs;
});

// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
PACKED_STRUCT(mapseg_t
{
  unsigned short		v1; // [crispy] extended nodes
  unsigned short		v2; // [crispy] extended nodes
  short		angle;		
  unsigned short		linedef; // [crispy] extended nodes
  short		side;
  short		offset;
});

// [crispy] allow loading of maps with DeePBSP nodes
// taken from prboom-plus/src/doomdata.h:183-190
PACKED_STRUCT(mapseg_deepbsp_t
{
    int v1;
    int v2;
    unsigned short angle;
    unsigned short linedef;
    short side;
    unsigned short offset;
});

// [crispy] allow loading of maps with ZDBSP nodes
// taken from prboom-plus/src/doomdata.h:192-196
PACKED_STRUCT(mapseg_zdbsp_t
{
    unsigned int v1, v2;
    unsigned short linedef;
    unsigned char side;
});


// BSP node structure.

// Indicate a leaf.
#define	NF_SUBSECTOR	0x80000000 // [crispy] extended nodes
#define	NO_INDEX	((unsigned short)-1) // [crispy] extended nodes

PACKED_STRUCT(mapnode_t
{
  // Partition line from (x,y) to x+dx,y+dy)
  short		x;
  short		y;
  short		dx;
  short		dy;

  // Bounding box for each child,
  // clip against view frustum.
  short		bbox[2][4];

  // If NF_SUBSECTOR its a subsector,
  // else it's a node of another subtree.
  unsigned short	children[2];


});
// [crispy] allow loading of maps with DeePBSP nodes
// taken from prboom-plus/src/doomdata.h:216-225
PACKED_STRUCT(mapnode_deepbsp_t
{
    short x;
    short y;
    short dx;
    short dy;
    short bbox[2][4];
    int children[2];
});

// [crispy] allow loading of maps with ZDBSP nodes
// taken from prboom-plus/src/doomdata.h:227-136
PACKED_STRUCT(mapnode_zdbsp_t
{
    short x;
    short y;
    short dx;
    short dy;
    short bbox[2][4];
    int children[2];
});



// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
PACKED_STRUCT(mapthing_t
{
    short		x;
    short		y;
    short		angle;
    short		type;
    short		options;
});

// [crispy] allow loading of Hexen-format maps
// taken from chocolate-doom/src/hexen/xddefs.h:134-149
PACKED_STRUCT(mapthing_hexen_t
{
    short tid;
    short x;
    short y;
    short height;
    short angle;
    short type;
    short options;
    byte special;
    byte arg1;
    byte arg2;
    byte arg3;
    byte arg4;
    byte arg5;
});




#endif			// __DOOMDATA__
