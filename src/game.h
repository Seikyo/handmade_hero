#ifndef GAME_H
#define GAME_H

#include "game_platform.h"

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

struct memory_area
{
	memory_index Size;
	uint8* Base;
	memory_index Used;
};

internal void
InitializeArea(memory_area *Area, memory_index Size, uint8 *Base)
{
	Area->Size = Size;
	Area->Base = Base;
	Area->Used = 0;
}

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

struct high_entity
{
	v2 Pos; // NOTE: Relative to the camera
	v2 dPos;
	uint32 AbsTileZ;
	uint32 FacingDirection;

	float32 Z;
	float32 dZ;

	uint32 LowEntityIndex;
};

enum entity_type
{
	EntityType_Null,
	EntityType_Hero,
	EntityType_Wall,
};

struct low_entity
{
	entity_type Type;

	tile_map_position Pos;
	float32 Width;
	float32 Height;

	// NOTE: This is for "Stairs"
	bool32 Collides;
	int32 dAbsTileZ;

	uint32 HighEntityIndex;
};

struct entity
{
	uint32 LowIndex;
	low_entity *Low;
	high_entity *High;
};

struct game_state
{
	memory_area WorldArea;
	world *World;

	uint32 CameraFollowingEntityIndex;
	tile_map_position CameraPos;

	uint32 PlayerIndexForController[ArrayCount( ((game_input *)0)->Controllers )];

	uint32 LowEntityCount;
	low_entity LowEntities[4096];

	uint32 HighEntityCount;
	high_entity HighEntities_[256];

	loaded_bitmap BackDrop;
	loaded_bitmap Shadow;
	hero_bitmaps HeroBitmaps[4];
};

#endif
