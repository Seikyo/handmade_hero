#ifndef GAME_H
#define GAME_H

/*
Compiler flags
	- INTERNAL:
		0 - Release build
		1 - Developper build

	- GAME_ASSERTS:
		0 - Ignore asserts
		1 - Enable asserts
*/

#include "game_platform.h"

struct memory_area
{
	memory_index Size;
	uint8* Base;
	memory_index Used;
};

#define PushSize(Area, type) (type *)PushSize_(Area, sizeof(type))
#define PushArray(Area, Count, type) (type *)PushSize_(Area, (Count)*sizeof(type))
void *
PushSize_(memory_area *Area, memory_index Size)
{
	Assert((Area->Used + Size) <= Area->Size);

	void *Result =  Area->Base + Area->Used;
	Area->Used += Size;
	return Result;
}

#include "game_math.h"
#include "game_intrinsics.h"
#include "game_tile.h"

struct world
{
	tile_map *TileMap;
};
struct loaded_bitmap
{
	int32 Width;
	int32 Height;
	uint32 *Pixels;
};

struct hero_bitmaps
{
	int32 AlignX;
	int32 AlignY;

	loaded_bitmap Head;
	loaded_bitmap Cape;
	loaded_bitmap Torso;
};

struct game_state
{
	memory_area WorldArea;
	world *World;

	tile_map_position CameraPos;
	tile_map_position PlayerPos;

	loaded_bitmap BackDrop;
	uint32 HeroFacingDirection;
	hero_bitmaps HeroBitmaps[4];
};

#endif
