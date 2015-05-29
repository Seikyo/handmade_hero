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

#include "game_helpers.h"
#include "game_tile.h"

struct world
{
	tile_map *TileMap;
};

struct game_state
{
	memory_area WorldArea;
	world *World;
	tile_map_position PlayerPos;
};

#endif
