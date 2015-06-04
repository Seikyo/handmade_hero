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

internal void
DebugPoint(game_offscreen_buffer *Buffer, int32 X, int32 Y, uint32 Color, int CrossSize)
{
	if(X < 0 || Y < 0 || X > Buffer->Width || Y > Buffer->Height)
	{
		return;
	}

	uint32 *Pixel = (uint32 *)((uint8 *)Buffer->Memory + 
								Y * Buffer->Pitch + 
								X * Buffer->BytesPerPixel);
	
	*Pixel = Color;
	for(int dC = 1; dC < CrossSize; dC++)
	{
		if(X > dC)
		{
			*(Pixel - dC) = Color;
		}
		if(X < Buffer->Width - dC) 
		{
			*(Pixel + dC) = Color;
		}
		if(Y > dC)
		{
 			*(uint32 *)((uint8 *)Pixel - dC * Buffer->Pitch ) = Color;
 		}
 		if(Y < Buffer->Height - dC)
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

#pragma pack(push, 1)
struct bitmap_header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 HeaderSize;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
	uint32 Compression;
	uint32 ImageSize;
	int32 HRes;
	int32 VRes;
	uint32 ColorsUsed;
	uint32 ColorsImportant;
	uint32 RMask;
	uint32 GMask;
	uint32 BMask;
	uint32 AMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
	loaded_bitmap Result = {};
	debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);
	// NOTE: 0xAARRGGBB in memory
	if(ReadResult.ContentsSize != 0)
	{
		bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
		uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
		Result.Pixels = Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		bit_scan_result RShift = FindLeastSignificantSetBit(Header->RMask);
		bit_scan_result GShift = FindLeastSignificantSetBit(Header->GMask);
		bit_scan_result BShift = FindLeastSignificantSetBit(Header->BMask);
		bit_scan_result AShift = FindLeastSignificantSetBit(Header->AMask);

		Assert(RShift.Found);
		Assert(GShift.Found);
		Assert(BShift.Found);
		Assert(AShift.Found);

		uint32 *SourceDest = Pixels;
		for(int32 Y = 0; Y < Header->Height; Y++)
		{
			for(int32 X = 0; X < Header->Width; X++)
			{
				uint32 C = *SourceDest;
				*SourceDest = ( (((C >> AShift.Index) & 0xFF) << 24) |
								(((C >> RShift.Index) & 0xFF) << 16) |
								(((C >> GShift.Index) & 0xFF) <<  8) |
								(((C >> BShift.Index) & 0xFF) <<  0) );
				++SourceDest;
			}
		}
	}
	return Result;
}

internal void
DrawBitmap(game_offscreen_buffer *ScreenBuffer, 
	loaded_bitmap *Bitmap, float32 FloatX, float32 FloatY,
	int32 AlignX = 0, int32 AlignY = 0)
{
	FloatX -= (float32)AlignX;
	FloatY -= (float32)AlignY;

	int32 MinX = RoundFloat32ToInt32(FloatX);
	int32 MinY = RoundFloat32ToInt32(FloatY);
	int32 MaxX = RoundFloat32ToInt32(FloatX + Bitmap->Width);
	int32 MaxY = RoundFloat32ToInt32(FloatY + Bitmap->Height);

	int32 SourceOffsetX = 0;
	int32 SourceOffsetY = 0;
	if(MinX < 0) 
	{ 
		SourceOffsetX = -MinX;
		MinX = 0;
	}
	if(MinY < 0) 
	{ 
		SourceOffsetY = -MinY;
		MinY = 0; 
	}
	if(MaxX > ScreenBuffer->Width) 
	{ 
		MaxX = ScreenBuffer->Width;
	}
	if(MaxY > ScreenBuffer->Height) 
	{ 
		MaxY = ScreenBuffer->Height;
	}

	uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height -1);
	SourceRow += -Bitmap->Width * SourceOffsetY + SourceOffsetX;
	uint8 *DestRow = (uint8 *)ScreenBuffer->Memory + 
					 MinY * ScreenBuffer->Pitch +
					 MinX * ScreenBuffer->BytesPerPixel;
	for(int32 Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Dest = (uint32 *)DestRow;
		uint32 *Source = SourceRow;
		for(int32 X = MinX; X < MaxX; ++X)
		{
			// NOTE: Alpha Blending
			float32 A 	 = (float32)((*Source >> 24) & 0xFF) / 255.f;
			float32 SrcR = (float32)((*Source >> 16) & 0xFF);
			float32 SrcG = (float32)((*Source >>  8) & 0xFF);
			float32 SrcB = (float32)((*Source >>  0) & 0xFF);

			float32 DestR = (float32)((*Dest >> 16) & 0xFF);
			float32 DestG = (float32)((*Dest >>  8) & 0xFF);
			float32 DestB = (float32)((*Dest >>  0) & 0xFF);

			float32 R = (1.f - A) * DestR + A * SrcR;
			float32 G = (1.f - A) * DestG + A * SrcG;
			float32 B = (1.f - A) * DestB + A * SrcB;

			*Dest = ( ((uint32)(R + 0.5f) << 16) |
					  ((uint32)(G + 0.5f) <<  8) |
					  ((uint32)(B + 0.5f) <<  0) );

			++Dest;
			++Source;
		}
		DestRow += ScreenBuffer->Pitch;
		SourceRow -= Bitmap->Width;
	}
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
		GameState->BackDrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, 
										   "test/test_background.bmp");

		hero_bitmaps *Bitmap;

		Bitmap = &GameState->HeroBitmaps[0];
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;

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

		GameState->CameraPos.AbsTileX = 17/2;
		GameState->CameraPos.AbsTileY =  9/2;

		GameState->PlayerPos.AbsTileX = 1;
		GameState->PlayerPos.AbsTileY = 3;
		GameState->PlayerPos.OffsetX = 5.f;
		GameState->PlayerPos.OffsetY = 5.f;

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

			if(Controller->MoveUp.EndedDown) 	
			{ 
				GameState->HeroFacingDirection = 1;
				dPlayerY =  1.f;
			}
			if(Controller->MoveDown.EndedDown) 	
			{ 
				GameState->HeroFacingDirection = 3;
				dPlayerY = -1.f; 
			}
			if(Controller->MoveRight.EndedDown) 
			{ 
				GameState->HeroFacingDirection = 0;
				dPlayerX =  1.f; 
			}
			if(Controller->MoveLeft.EndedDown) 	
			{ 
				GameState->HeroFacingDirection = 2;
				dPlayerX = -1.f; 
			}

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
			GameState->CameraPos.AbsTileZ = GameState->PlayerPos.AbsTileZ;

			tile_map_difference Diff = Substract(TileMap, &GameState->PlayerPos, &GameState->CameraPos);

			if(Diff.dX > (9.0f * TileMap->TileSideInMeters))
			{
				GameState->CameraPos.AbsTileX += 17;
			}
			else if(Diff.dX < -(9.0f * TileMap->TileSideInMeters))
			{
				GameState->CameraPos.AbsTileX -= 17;
			}

			if(Diff.dY > (5.0f * TileMap->TileSideInMeters))
			{
				GameState->CameraPos.AbsTileY += 9;
			}
			else if(Diff.dY < -(5.0f * TileMap->TileSideInMeters))
			{
				GameState->CameraPos.AbsTileY -= 9;
			}
		}
	}

	DrawBitmap(ScreenBuffer, &GameState->BackDrop, 0, 0);

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
			uint32 Column = RelColumn + GameState->CameraPos.AbsTileX;
			uint32 Row 	  = RelRow + GameState->CameraPos.AbsTileY;
			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->CameraPos.AbsTileZ);

			if(TileID > 1)
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
				if( (Column == GameState->CameraPos.AbsTileX) && 
					(Row == GameState->CameraPos.AbsTileY) )
				{
					Gray = 0.f;
				}

				float32 CenX = ScreenCenterX - MetersToPixels * GameState->CameraPos.OffsetX + ((float32)RelColumn) * TileSideInPixels;
				float32 CenY = ScreenCenterY + MetersToPixels * GameState->CameraPos.OffsetY - ((float32)RelRow) * TileSideInPixels;
				float32 MinX = CenX - 0.5f * TileSideInPixels;
				float32 MinY = CenY - 0.5f * TileSideInPixels;
				float32 MaxX = CenX + 0.5f * TileSideInPixels;
				float32 MaxY = CenY + 0.5f * TileSideInPixels;
				DrawRectangle(ScreenBuffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
			}
		}
	}
	
	tile_map_difference Diff = Substract(TileMap, &GameState->PlayerPos, &GameState->CameraPos);

	float32 PlayerR = 1.0f;
	float32 PlayerG = 1.0f;
	float32 PlayerB = 0.0f; 
	float32 PlayerGroundPointX = ScreenCenterX + MetersToPixels * Diff.dX;
	float32 PlayerGroundPointY = ScreenCenterY - MetersToPixels * Diff.dY;
	float32 PlayerLeft = PlayerGroundPointX - MetersToPixels *  0.5f * PlayerWidth;
	float32 PlayerTop  = PlayerGroundPointY - MetersToPixels *  PlayerHeight;

	DrawRectangle(ScreenBuffer,
				  PlayerLeft, PlayerTop, 
				  PlayerLeft + MetersToPixels * PlayerWidth, 
				  PlayerTop + MetersToPixels * PlayerHeight,
				  PlayerR, PlayerG, PlayerB);

	hero_bitmaps *HeroBitmap = &GameState->HeroBitmaps[GameState->HeroFacingDirection];
	DrawBitmap(ScreenBuffer, &HeroBitmap->Torso, PlayerGroundPointX, PlayerGroundPointY, HeroBitmap->AlignX, HeroBitmap->AlignY);
	DrawBitmap(ScreenBuffer, &HeroBitmap->Cape,  PlayerGroundPointX, PlayerGroundPointY, HeroBitmap->AlignX, HeroBitmap->AlignY);
	DrawBitmap(ScreenBuffer, &HeroBitmap->Head,  PlayerGroundPointX, PlayerGroundPointY, HeroBitmap->AlignX, HeroBitmap->AlignY);

	DebugPoint(ScreenBuffer, 
			   FloorFloat32ToInt32(PlayerLeft + MetersToPixels * 0.5f * PlayerWidth),
			   FloorFloat32ToInt32(PlayerTop + MetersToPixels * PlayerHeight),
			   0xFFFF0000, 3);

	DebugPoint(ScreenBuffer, 
			   FloorFloat32ToInt32(PlayerLeft),
			   FloorFloat32ToInt32(PlayerTop + MetersToPixels * PlayerHeight),
			   0xFF00FF00, 3);

	DebugPoint(ScreenBuffer, 
			   FloorFloat32ToInt32(PlayerLeft + MetersToPixels * PlayerWidth),
			   FloorFloat32ToInt32(PlayerTop + MetersToPixels * PlayerHeight),
			   0xFF0000FF, 3);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}