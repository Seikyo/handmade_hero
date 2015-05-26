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

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#if GAME_ASSERTS
#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Result;
}

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input *Result = &Input->Controllers[ControllerIndex];
	return Result; 
}

struct canonical_position
{
#if 1
	int32 TileMapX;
	int32 TileMapY;

	int32 TileX;
	int32 TileY;
#else 
	uint32 _TileX;
	uint32 _TileY;
#endif
	// NOTE: Tile relatives position
	float32 TileRelX;
	float32 TileRelY;
};

struct tile_map
{
	uint32 *Tiles;
};

struct world
{
	float32 TileSideInMeters;
	int32 TileSideInPixels;
	float32 MetersToPixels;

	int32 TileCountX;
	int32 TileCountY;

	int32 TileMapCountX;
	int32 TileMapCountY;

	float32 UpperLeftX;
	float32 UpperLeftY;

	tile_map *TileMaps;
};

struct game_state
{
	canonical_position PlayerPos;
};

#endif
