#include "game.h"
#include "game_tile.cpp"
#include "game_random.h"

internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, game_state *GameState)
{
	int16 ToneVolume = 3000;
	int ToneHz 		 = 440;
	int WavePeriod   = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 * SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
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
				     (RoundFloat32ToUInt32(G*255.f) <<  8) |
				     (RoundFloat32ToUInt32(B*255.f) <<  0) );

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

// internal void
// DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
// {
// 	debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);
// 	int Y = 5;
// }

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

internal void
InitializeArea(memory_area *Area, memory_index Size, uint8 *Base)
{
	Area->Size = Size;
	Area->Base = Base;
	Area->Used = 0;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
	Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == 
		(ArrayCount(Input->Controllers[0].Buttons) - 1));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	float32 PlayerHeight = 1.4f;
	float32 PlayerWidth  = 0.75f * PlayerHeight;

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		// DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
		InitializeArea(&GameState->WorldArea,
					   Memory->PermanentStorageSize - sizeof(game_state),
					   (uint8 *)Memory->PermanentStorage + sizeof(game_state));

		GameState->World = PushSize(&GameState->WorldArea, world);
		world *World = GameState->World;
		World->TileMap = PushSize(&GameState->WorldArea, tile_map);

		tile_map *TileMap = World->TileMap;

		TileMap->ChunkShift = 4; 
		TileMap->ChunkMask  = (1 << TileMap->ChunkShift) - 1;
		TileMap->ChunkDim   = (1 << TileMap->ChunkShift);

		TileMap->TileChunkCountX = 128;
		TileMap->TileChunkCountY = 128;
		TileMap->TileChunkCountZ = 2;
		TileMap->TileChunks = PushArray(&GameState->WorldArea, 
										TileMap->TileChunkCountX * 
										TileMap->TileChunkCountY * 
										TileMap->TileChunkCountZ, 
										tile_chunk);

		TileMap->TileSideInMeters = 1.4f;

		uint32 RandomNumberIndex = 0;

		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenX = 0;
		uint32 ScreenY = 0;
		uint32 AbsTileZ = 0;

		GameState->PlayerPos.AbsTileX = 1 + 3 * TilesPerWidth;
		GameState->PlayerPos.AbsTileY = 3 + TilesPerHeight;
		GameState->PlayerPos.OffsetX = 0.5f;
		GameState->PlayerPos.OffsetY = 0.5f;

		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;
		for(uint32 ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex)
		{
			// TODO: Random number generator
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));

			uint32 RandomChoice;
			if(DoorUp || DoorDown)
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
			}
			else 
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
			}

			bool32 CreatedZDoor = false;
			if(RandomChoice == 2)
			{
				CreatedZDoor = true;
				if(AbsTileZ == 0)
				{
					DoorUp = true;
				}
				if(AbsTileZ == 1)
				{
					DoorDown = true;
				}
			}
			else if(RandomChoice == 1)
			{

				DoorRight = true;
			}
			else
			{
				DoorTop = true;
			}

			for(uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
			{
				for(uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
				{
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

					uint32 TileValue = 1;
					if( (TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight / 2))) )
					{
						TileValue = 2;
					}

					if( (TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight / 2))) )
					{
						TileValue = 2;
					}

					if( (TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth / 2))) )
					{
						TileValue = 2;
					}

					if( (TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth / 2))) )
					{
						TileValue = 2;
					}

					if(TileX == 10 && TileY == 6)
					{
						if(DoorUp)
						{
							TileValue = 3;
						}
						if(DoorDown)
						{
							TileValue = 4;
						}
					}

					SetTileValue(&GameState->WorldArea, World->TileMap,
								 AbsTileX, AbsTileY, AbsTileZ, TileValue);
				}
			}

			DoorLeft = DoorRight;
			DoorBottom = DoorTop;

			if(CreatedZDoor)
			{
				DoorUp = !DoorUp;
				DoorDown = !DoorDown;
			}
			else
			{
				DoorDown = false;
				DoorUp = false;
			}

			DoorRight = false;
			DoorTop = false;

			if(RandomChoice == 2)
			{
				if(AbsTileZ == 0)
				{
					AbsTileZ = 1;
				}
				else if(AbsTileZ == 1)
				{
					AbsTileZ = 0;
				}
			}
			else if(RandomChoice == 1)
			{
				ScreenX += 1;
			}
			else
			{
				ScreenY += 1;
			}
		}

		Memory->IsInitialized = true;
	}

	world *World = GameState->World;
	tile_map *TileMap = World->TileMap;

	int32 TileSideInPixels = 60;
	float32 MetersToPixels = (float32)TileSideInPixels / TileMap->TileSideInMeters;

	float32 LowerLeftX = -((float32)TileSideInPixels / 2.f);
	float32 LowerLeftY =  (float32)ScreenBuffer->Height;

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

			if(Controller->MoveUp.EndedDown) 	{ dPlayerY =  1.f; }
			if(Controller->MoveDown.EndedDown) 	{ dPlayerY = -1.f; }
			if(Controller->MoveRight.EndedDown) { dPlayerX =  1.f; }
			if(Controller->MoveLeft.EndedDown) 	{ dPlayerX = -1.f; }

			float32 PlayerSpeed = 3.0f;

			if(Controller->ActionUp.EndedDown)
			{
			 	PlayerSpeed = 10.f;
			}

			tile_map_position NewPlayerPos = GameState->PlayerPos;
			NewPlayerPos.OffsetX += dPlayerX * Input->dTForFrame * PlayerSpeed;
			NewPlayerPos.OffsetY += dPlayerY * Input->dTForFrame * PlayerSpeed;
			NewPlayerPos = RecanonicalizePosition(TileMap, NewPlayerPos);

			tile_map_position NewPlayerLeft = NewPlayerPos;
			NewPlayerLeft.OffsetX -= 0.5f * PlayerWidth;
			NewPlayerLeft = RecanonicalizePosition(TileMap, NewPlayerLeft);

			tile_map_position NewPlayerRight = NewPlayerPos;
			NewPlayerRight.OffsetX += 0.5f * PlayerWidth;
			NewPlayerRight = RecanonicalizePosition(TileMap, NewPlayerRight);

			if( IsTileMapPointEmpty(TileMap, NewPlayerPos) &&
				IsTileMapPointEmpty(TileMap, NewPlayerLeft) &&
				IsTileMapPointEmpty(TileMap, NewPlayerRight) )
			{
				if(!AreOnSameTile(&GameState->PlayerPos, &NewPlayerPos))
				{
					uint32 NewTileValue = GetTileValue(TileMap, NewPlayerPos);
					if(NewTileValue == 3)
					{
						++NewPlayerPos.AbsTileZ;
					}
					else if(NewTileValue == 4)
					{
						--NewPlayerPos.AbsTileZ;
					}
				}
				GameState->PlayerPos = NewPlayerPos;
			}
		}
	}

	DrawRectangle(ScreenBuffer,
				  0.f, 0.f, 
				  (float32)ScreenBuffer->Width, (float32)ScreenBuffer->Height,
				  1.0f, 0.0f, 0.1f);

	float32 ScreenCenterX = 0.5f * (float32)ScreenBuffer->Width;
	float32 ScreenCenterY = 0.5f * (float32)ScreenBuffer->Height;

	for(int32 RelRow = -10;
		RelRow < 10;
		++RelRow)
	{
		for(int32 RelColumn = -20;
			RelColumn < 20;
			 ++RelColumn)
		{
			uint32 Column = RelColumn + GameState->PlayerPos.AbsTileX;
			uint32 Row 	  = RelRow + GameState->PlayerPos.AbsTileY;
			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerPos.AbsTileZ);

			if(TileID > 0)
			{
				float32 Gray  = 0.5f;
				if(TileID == 2)
				{
					Gray = 1.0f;
				}
				if(TileID > 2)
				{
					Gray = 0.25f;
				}
				if( (Column == GameState->PlayerPos.AbsTileX) && 
					(Row == GameState->PlayerPos.AbsTileY) )
				{
					Gray = 0.f;
				}

				float32 CenX = ScreenCenterX - MetersToPixels * GameState->PlayerPos.OffsetX + ((float32)RelColumn) * TileSideInPixels;
				float32 CenY = ScreenCenterY + MetersToPixels * GameState->PlayerPos.OffsetY - ((float32)RelRow) * TileSideInPixels;
				float32 MinX = CenX - 0.5f * TileSideInPixels;
				float32 MinY = CenY - 0.5f * TileSideInPixels;
				float32 MaxX = CenX + 0.5f * TileSideInPixels;
				float32 MaxY = CenY + 0.5f * TileSideInPixels;
				DrawRectangle(ScreenBuffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
			}
		}
	}

	float32 PlayerR = 1.0f;
	float32 PlayerG = 1.0f;
	float32 PlayerB = 0.0f; 
	float32 PlayerLeft = ScreenCenterX - MetersToPixels *  0.5f * PlayerWidth;
	float32 PlayerTop  = ScreenCenterY - MetersToPixels *  PlayerHeight;

	DrawRectangle(ScreenBuffer,
				  PlayerLeft, PlayerTop, 
				  PlayerLeft + MetersToPixels * PlayerWidth, 
				  PlayerTop + MetersToPixels * PlayerHeight,
				  PlayerR, PlayerG, PlayerB);

	// DebugPoint(ScreenBuffer, 
	// 		   FloorFloat32ToInt32(PlayerLeft + MetersToPixels * 0.5f * PlayerWidth),
	// 		   FloorFloat32ToInt32(PlayerTop + MetersToPixels * PlayerHeight),
	// 		   0xFFFF0000, 3);

	// DebugPoint(ScreenBuffer, 
	// 		   FloorFloat32ToInt32(PlayerLeft),
	// 		   FloorFloat32ToInt32(PlayerTop + MetersToPixels * PlayerHeight),
	// 		   0xFF00FF00, 3);

	// DebugPoint(ScreenBuffer, 
	// 		   FloorFloat32ToInt32(PlayerLeft + MetersToPixels * PlayerWidth),
	// 		   FloorFloat32ToInt32(PlayerTop + MetersToPixels * PlayerHeight),
	// 		   0xFF0000FF, 3);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}