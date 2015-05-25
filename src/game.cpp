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

internal int32
RoundFloat32ToInt32(float32 FloatValue)
{
	int32 Result = (int32)(FloatValue + 0.5f);
	return Result;
}

internal uint32
RoundFloat32ToUInt32(float32 FloatValue)
{
	uint32 Result = (uint32)(FloatValue + 0.5f);
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

	if(MinX < 0) MinX = 0;
	if(MinY < 0) MinY = 0;
	if(MaxX > Buffer->Width) MaxX = Buffer->Width;
	if(MaxY > Buffer->Height) MaxY = Buffer->Height;

	uint32 Color = ( (RoundFloat32ToUInt32(R*255.f) << 16) |
				     (RoundFloat32ToUInt32(G*255.f) << 8) |
				     (RoundFloat32ToUInt32(B*255.f) << 0) );

	uint8 *Row = (uint8 *)Buffer->Memory + 
				   MinX * Buffer->BytesPerPixel + 
				   MinY * Buffer->Pitch;

	for(int Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = MinX; X < MaxX + 10; ++X)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}
		
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
	Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == 
		(ArrayCount(Input->Controllers[0].Buttons) - 1));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		Memory->IsInitialized = true;
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
			GameState->PlayerX += dPlayerX * Input->dTForFrame * Speed;
			GameState->PlayerY += dPlayerY * Input->dTForFrame * Speed;
		}
	}

	uint32 TileMap[9][17] = {
		{1, 1, 1,  1, 1, 1,  1, 1, 0,  1, 1, 1,  1, 1, 1,  1, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1},
		{1, 1, 1,  1, 1, 1,  1, 1, 0,  1, 1, 1,  1, 1, 1,  1, 1},
	};

	float32 UpperLeftX = 0.f;
	float32 UpperLeftY = 0.f;
	float32 TileWidth = 56.47f;
	float32 TileHeight = 60.f;

	DrawRectangle(ScreenBuffer, 
				  0.0f, 0.0f, (float32)ScreenBuffer->Width, (float32)ScreenBuffer->Height,
				  1.0f, 0.5f, 0.0f);

	for(int Row = 0; Row < 9; ++Row)
	{
		for(int Column = 0; Column < 17; ++Column)
		{
			uint32 TileID = TileMap[Row][Column];
			float32 Gray = 0.5f;
			if(TileID == 1) Gray = 1.0f;
			float32 MinX = UpperLeftX + (float32)Column * TileWidth;
			float32 MinY = UpperLeftY + (float32)Row * TileHeight;
			float32 MaxX = MinX + TileWidth;
			float32 MaxY = MinY + TileHeight;
			DrawRectangle(ScreenBuffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
		}
	}

	float32 PlayerR = 1.0f;
	float32 PlayerG = 0.0f;
	float32 PlayerB = 1.0f;
	float32 PlayerWidth = 0.75f * TileWidth;
	float32 PlayerHeight = TileHeight;
	float32 PlayerTop = GameState->PlayerY - PlayerHeight;
	float32 PlayerLeft = GameState->PlayerX - 0.5f * PlayerWidth;
	DrawRectangle(ScreenBuffer,
				  PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight,
				  PlayerR, PlayerG, PlayerB);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}