#include "game.h"
#include "game_helpers.h"

internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, game_state *GameState)
{
	int16 ToneVolume = 3000;
	int ToneHz = 440;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 * SampleOut = SoundBuffer->Samples;
	for(int SampleIndex=0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
#if 0
		float32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
#else	
		int16 SampleValue = 0;
#endif
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
#if 0
		GameState->tSine += 2.f * Pi32 / (float32)WavePeriod;
		if(GameState->tSine > 2.f*Pi32)
		{
			GameState->tSine -= 2.f*Pi32;
		}
#endif
	}
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
			  float32 FloatMinX, float32 FloatMinY,
			  float32 FloatMaxX, float32 FloatMaxY, 
			  float32 R, float32 G, float32 B)
{
	int32 MinX = RoundFloat32ToInt32(FloatMinX);
	int32 MinY = RoundFloat32ToInt32(FloatMinY);
	int32 MaxX = RoundFloat32ToInt32(FloatMaxX);
	int32 MaxY = RoundFloat32ToInt32(FloatMaxY);

	if(MinX < 0) { MinX = 0; }
	if(MinY < 0) { MinY = 0; }
	if(MaxX > Buffer->Width) { MaxX = Buffer->Width; }
	if(MaxY > Buffer->Height) { MaxY = Buffer->Height; }

	uint32 Color = ( (RoundFloat32ToUInt32(R*255.f) << 16) |
				     (RoundFloat32ToUInt32(G*255.f) << 8) |
				     (RoundFloat32ToUInt32(B*255.f) << 0) );

	uint8 *Row = (uint8 *)Buffer->Memory + 
				   MinY * Buffer->Pitch +
				   MinX * Buffer->BytesPerPixel;

	for(int Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

internal void
DebugPoint(game_offscreen_buffer *Buffer, int32 X, int32 Y, uint32 Color, int CrossSize)
{
	uint32 *Pixel = (uint32 *)((uint8 *)Buffer->Memory + 
					Y * Buffer->Pitch + 
					X * Buffer->BytesPerPixel);
	
	*Pixel = Color;
	for(int dC = 1; dC < CrossSize; dC++)
	{
		if(X >= dC)
		{
			*(Pixel - dC) = Color;
		}
		if(X <= Buffer->Width - dC) 
		{
			*(Pixel + dC) = Color;
		}
		if(Y >= dC)
		{
 			*(uint32 *)((uint8 *)Pixel - dC * Buffer->Pitch ) = Color;
 		}
 		if(Y <= Buffer->Height - dC)
 		{
 			*(uint32 *)((uint8 *)Pixel + dC * Buffer->Pitch ) = Color;
 		}
	}
}

inline tile_map *
GetTileMap(world *World, int32 TileMapX, int32 TileMapY)
{
	tile_map *TileMap = 0;
	if( TileMapX >= 0 && TileMapX < World->TileMapCountX &&
		TileMapY >= 0 && TileMapY < World->TileMapCountY)
	{
		TileMap = &World->TileMaps[TileMapY * World->TileMapCountX + TileMapX];
	}
	return TileMap;
}

inline uint32
GetTileValue(world *World, tile_map *TileMap, int32 TileX, int32 TileY)
{
	Assert(TileMap);
	uint32 TileValue = TileMap->Tiles[TileY * World->TileCountX + TileX];
	return TileValue;
}

inline bool32
IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TileX, int32 TileY)
{
	bool32 Empty = false;

	if(TileMap)
	{
		if( (TileX >= 0) && (TileX < World->TileCountX) &&
			(TileY >= 0) && (TileY < World->TileCountY) )
		{
			uint32 TileMapValue = GetTileValue(World, TileMap, TileX, TileY);
			Empty = (TileMapValue == 0);
		}
	}

	return Empty;
}

inline void
CanonicalizeCoord(world *World, int32 TileCount, int32 *TileMapCoord, int32 *TileCoord, float32 *TileRelCoord)
{
	int32 Offset = FloorFloat32ToInt32(*TileRelCoord / World->TileSideInMeters);
	*TileCoord += Offset;
	*TileRelCoord -= Offset * World->TileSideInMeters;

	Assert(*TileRelCoord >= 0);
	Assert(*TileRelCoord < World->TileSideInMeters);

	if(*TileCoord < 0)
	{
		*TileCoord = *TileCoord + TileCount;
		--*TileMapCoord;
	}
	if(*TileCoord >= TileCount)
	{
		*TileCoord = *TileCoord - TileCount;
		++*TileMapCoord;
	}
}

inline canonical_position
RecanonicalizePosition(world *World, canonical_position Pos)
{
	canonical_position Result = Pos;

	CanonicalizeCoord(World, World->TileCountX, &Result.TileMapX, &Result.TileX, &Result.TileRelX);
	CanonicalizeCoord(World, World->TileCountY, &Result.TileMapY, &Result.TileY, &Result.TileRelY);

	return Result;
}

internal bool32
IsWorldPointEmpty(world *World, canonical_position Pos)
{
	bool32 Empty = false;

	tile_map *TileMap = GetTileMap(World, Pos.TileMapX, Pos.TileMapY);
	Empty = IsTileMapPointEmpty(World, TileMap, Pos.TileX, Pos.TileY);

	return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
	Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == 
		(ArrayCount(Input->Controllers[0].Buttons) - 1));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
	uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 1,  0, 1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 1,  1, 1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 1, 1,  1, 1, 1,  1, 1, 0,  1, 1, 1,  1, 1, 1,  1, 1},
	};

	uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1,  1, 1, 1,  1, 1, 0,  1, 1, 1,  1, 1, 1,  1, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1},
	};

	uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 1, 1,  1, 1, 1,  1, 1, 0,  1, 1, 1,  1, 1, 1,  1, 1},
	};

	uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1,  1, 1, 1,  1, 1, 0,  1, 1, 1,  1, 1, 1,  1, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1},
	};

	tile_map TileMaps[2][2];

	TileMaps[0][0].Tiles = (uint32 *)Tiles00;
	TileMaps[0][1].Tiles = (uint32 *)Tiles10;
	TileMaps[1][0].Tiles = (uint32 *)Tiles01;
	TileMaps[1][1].Tiles = (uint32 *)Tiles11;

	world World;
	World.TileMapCountX = 2;
	World.TileMapCountY = 2;
	World.TileMaps = (tile_map *)TileMaps;

	World.TileCountX = TILE_MAP_COUNT_X;
	World.TileCountY = TILE_MAP_COUNT_Y;
	World.TileSideInMeters = 1.4f;
	World.TileSideInPixels = 60;
	World.MetersToPixels = (float32)World.TileSideInPixels / World.TileSideInMeters;
	World.UpperLeftX = -(World.TileSideInPixels / 2.f);
	World.UpperLeftY =   0.0f;

	float32 PlayerHeight = 1.4f;
	float32 PlayerWidth = 0.75f * PlayerHeight;

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		GameState->PlayerPos.TileMapX = 0;
		GameState->PlayerPos.TileMapY = 0;
		GameState->PlayerPos.TileX = 3;
		GameState->PlayerPos.TileY = 3;
		GameState->PlayerPos.TileRelX = 5.0f;
		GameState->PlayerPos.TileRelY = 5.0f;

		Memory->IsInitialized = true;
	}

	tile_map *TileMap = GetTileMap(&World, GameState->PlayerPos.TileMapX, GameState->PlayerPos.TileMapY);
	Assert(TileMap);

	for(int ControllerIndex=0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if(Controller->IsAnalog)
		{
		}
		else
		{
			float32 dPlayerX = 0.0f;
			float32 dPlayerY = 0.0f;

			if(Controller->MoveUp.EndedDown) 	{ dPlayerY = -1.f; }
			if(Controller->MoveDown.EndedDown) 	{ dPlayerY =  1.f; }
			if(Controller->MoveLeft.EndedDown) 	{ dPlayerX = -1.f; }
			if(Controller->MoveRight.EndedDown) { dPlayerX =  1.f; }

			float32 Speed = 3.0f;

			canonical_position NewPlayerPos = GameState->PlayerPos;
			NewPlayerPos.TileRelX += dPlayerX * Input->dTForFrame * Speed;
			NewPlayerPos.TileRelY += dPlayerY * Input->dTForFrame * Speed;
			NewPlayerPos = RecanonicalizePosition(&World, NewPlayerPos);

			canonical_position NewPlayerLeft = NewPlayerPos;
			NewPlayerLeft.TileRelX -= 0.5f * PlayerWidth;
			NewPlayerLeft = RecanonicalizePosition(&World, NewPlayerLeft);

			canonical_position NewPlayerRight = NewPlayerPos;
			NewPlayerRight.TileRelX += 0.5f * PlayerWidth;
			NewPlayerRight = RecanonicalizePosition(&World, NewPlayerRight);

			if( IsWorldPointEmpty(&World, NewPlayerPos) &&
				IsWorldPointEmpty(&World, NewPlayerLeft) &&
				IsWorldPointEmpty(&World, NewPlayerRight) )
			{
				GameState->PlayerPos = NewPlayerPos;
			}
		}
	}

	DrawRectangle(ScreenBuffer,
				  0.f, 0.f, 
				  (float32)ScreenBuffer->Width, (float32)ScreenBuffer->Height,
				  1.0f, 0.0f, 0.1f);
	for(int Row = 0; Row < TILE_MAP_COUNT_Y; ++Row)
	{
		for(int Column = 0; Column < TILE_MAP_COUNT_X; ++Column)
		{
			uint32 TileID = GetTileValue(&World, TileMap, Column, Row);
			float32 Gray = 0.5f;
			if(TileID == 1)
			{
				Gray = 1.0f;
			}
			if( (Column == GameState->PlayerPos.TileX) && 
				(Row == GameState->PlayerPos.TileY) )
			{
				Gray = 0.f;
			}
			float32 MinX = World.UpperLeftX + ((float32)Column) * World.TileSideInPixels;
			float32 MinY = World.UpperLeftY + ((float32)Row) * World.TileSideInPixels;
			float32 MaxX = MinX + World.TileSideInPixels;
			float32 MaxY = MinY + World.TileSideInPixels;
			DrawRectangle(ScreenBuffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
		}
	}

	float32 PlayerR = 1.0f;
	float32 PlayerG = 1.0f;
	float32 PlayerB = 0.0f; 
	float32 PlayerLeft = World.UpperLeftX + GameState->PlayerPos.TileX * World.TileSideInPixels +
						 World.MetersToPixels * (GameState->PlayerPos.TileRelX - 0.5f * PlayerWidth);
	float32 PlayerTop  = World.UpperLeftY + GameState->PlayerPos.TileY * World.TileSideInPixels +
						 World.MetersToPixels * (GameState->PlayerPos.TileRelY - PlayerHeight);

	DrawRectangle(ScreenBuffer,
				  PlayerLeft, PlayerTop, 
				  PlayerLeft + World.MetersToPixels * PlayerWidth, 
				  PlayerTop + World.MetersToPixels * PlayerHeight,
				  PlayerR, PlayerG, PlayerB);

	DebugPoint(ScreenBuffer, 
			   FloorFloat32ToInt32(PlayerLeft + World.MetersToPixels * 0.5f * PlayerWidth),
			   FloorFloat32ToInt32(PlayerTop + World.MetersToPixels * PlayerHeight),
			   0xFFFF0000, 3);

	DebugPoint(ScreenBuffer, 
			   FloorFloat32ToInt32(PlayerLeft),
			   FloorFloat32ToInt32(PlayerTop + World.MetersToPixels * PlayerHeight),
			   0xFF00FF00, 3);

	DebugPoint(ScreenBuffer, 
			   FloorFloat32ToInt32(PlayerLeft + World.MetersToPixels * PlayerWidth),
			   FloorFloat32ToInt32(PlayerTop + World.MetersToPixels * PlayerHeight),
			   0xFF0000FF, 3);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}