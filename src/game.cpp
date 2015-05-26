#include "game.h"

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

inline int32
RoundFloat32ToInt32(float32 FloatValue)
{
	int32 Result = (int32)(FloatValue + 0.5f);
	return Result;
}

inline uint32
RoundFloat32ToUInt32(float32 FloatValue)
{
	uint32 Result = (uint32)(FloatValue + 0.5f);
	return Result;
}

inline int32
TruncateFloat32ToInt32(float32 FloatValue)
{
	int32 Result = (uint32)(FloatValue);
	return Result;
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

struct tile_map
{
	int32 CountX;
	int32 CountY;

	float32 UpperLeftX;
	float32 UpperLeftY;
	float32 TileWidth;
	float32 TileHeight;

	uint32 *Tiles;
};

internal void
DebugPoint(game_offscreen_buffer *Buffer, int32 X, int32 Y, uint32 Color, int CrossSize)
{
	uint32 *Pixel = (uint32 *)((uint8 *)Buffer->Memory + 
					Y * Buffer->Pitch + 
					X * Buffer->BytesPerPixel);
	
	*Pixel = Color;
	for(int dC = 1; dC < CrossSize; dC++)
	{
		*(Pixel - dC) = Color;
		*(Pixel + dC) = Color;
 		*(uint32 *)((uint8 *)Pixel - dC * Buffer->Pitch ) = Color;
 		*(uint32 *)((uint8 *)Pixel + dC * Buffer->Pitch ) = Color;
	}
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, float32 TestX, float32 TestY)
{
	bool32 Empty = false;

	int32 TileMapX = TruncateFloat32ToInt32((TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
	int32 TileMapY = TruncateFloat32ToInt32((TestY - TileMap->UpperLeftY) / TileMap->TileHeight);

	bool32 IsValid = false;

	if( (TileMapX >= 0) && (TileMapX < TileMap->CountX) &&
		(TileMapY >= 0) && (TileMapY < TileMap->CountY) )
	{
		uint32 TileMapValue = TileMap->Tiles[TileMapY * TileMap->CountX + TileMapX];
		Empty = (TileMapValue == 0);
	}

	return Empty;
}		
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
	Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == 
		(ArrayCount(Input->Controllers[0].Buttons) - 1));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
	uint32 Tiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1,  1, 1, 1,  1, 1, 0,  1, 1, 1,  1, 1, 1,  1, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 1,  0, 1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{0, 0, 1,  1, 1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 1, 1,  1, 1, 1,  1, 1, 0,  1, 1, 1,  1, 1, 1,  1, 1},
	};

	tile_map TileMap;
	TileMap.CountX = TILE_MAP_COUNT_X;
	TileMap.CountY = TILE_MAP_COUNT_Y;
	TileMap.UpperLeftX = -30.0f;
	TileMap.UpperLeftY =   0.0f;
	TileMap.TileWidth  =  60.0f;
	TileMap.TileHeight =  60.0f;
	TileMap.Tiles = (uint32 *)Tiles;

	float32 PlayerWidth = 0.75f * TileMap.TileWidth;
	float32 PlayerHeight = TileMap.TileHeight;

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		Memory->IsInitialized = true;
		GameState->PlayerX = 100.f;
		GameState->PlayerY = 100.f;
	}


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
			if(Controller->MoveUp.EndedDown)
			{
				dPlayerY = -1.f;
			}
			if(Controller->MoveDown.EndedDown)
			{
				dPlayerY = 1.f;
			}
			if(Controller->MoveLeft.EndedDown)
			{
				dPlayerX = -1.f;
			}
			if(Controller->MoveRight.EndedDown)
			{
				dPlayerX = 1.f;
			}

			float32 Speed = 64.f;
			float32 NewPlayerX = GameState->PlayerX + dPlayerX * Input->dTForFrame * Speed;
			float32 NewPlayerY = GameState->PlayerY + dPlayerY * Input->dTForFrame * Speed;

			if( IsTileMapPointEmpty(&TileMap, NewPlayerX, NewPlayerY) &&
				IsTileMapPointEmpty(&TileMap, NewPlayerX + 0.5f * PlayerWidth, NewPlayerY) &&
				IsTileMapPointEmpty(&TileMap, NewPlayerX - 0.5f * PlayerWidth, NewPlayerY) )
			{
				GameState->PlayerX = NewPlayerX;
				GameState->PlayerY = NewPlayerY;
			}
		}
	}

	for(int Row = 0; Row < TILE_MAP_COUNT_Y; ++Row)
	{
		for(int Column = 0; Column < TILE_MAP_COUNT_X; ++Column)
		{
			uint32 TileID = Tiles[Row][Column];
			float32 Gray = 0.5f;
			if(TileID == 1)
			{
				Gray = 1.0f;
			}
			float32 MinX = TileMap.UpperLeftX + (float32)Column * TileMap.TileWidth;
			float32 MinY = TileMap.UpperLeftY + (float32)Row * TileMap.TileHeight;
			float32 MaxX = MinX + TileMap.TileWidth;
			float32 MaxY = MinY + TileMap.TileHeight;
			DrawRectangle(ScreenBuffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
		}
	}

	float32 PlayerR = 1.0f;
	float32 PlayerG = 1.0f;
	float32 PlayerB = 0.0f;
	float32 PlayerTop = GameState->PlayerY - PlayerHeight;
	float32 PlayerLeft = GameState->PlayerX - 0.5f * PlayerWidth;
	DrawRectangle(ScreenBuffer,
				  PlayerLeft, PlayerTop, 
				  PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight,
				  PlayerR, PlayerG, PlayerB);

	DebugPoint(ScreenBuffer, 
			   TruncateFloat32ToInt32(GameState->PlayerX),
			   TruncateFloat32ToInt32(GameState->PlayerY),
			   0xFFFF0000, 3);

	DebugPoint(ScreenBuffer, 
			   TruncateFloat32ToInt32(PlayerLeft),
			   TruncateFloat32ToInt32(PlayerTop + PlayerHeight),
			   0xFF00FF00, 3);

	DebugPoint(ScreenBuffer, 
			   TruncateFloat32ToInt32(PlayerLeft + PlayerWidth),
			   TruncateFloat32ToInt32(GameState->PlayerY),
			   0xFF0000FF, 3);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}