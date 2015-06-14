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
DrawRectangle(game_offscreen_buffer *ScreenBuffer, v2 vMin, v2 vMax, float32 R, float32 G, float32 B, float32 A = 1.0f)
{
	int32 MinX = RoundFloat32ToInt32(vMin.X);
	int32 MinY = RoundFloat32ToInt32(vMin.Y);
	int32 MaxX = RoundFloat32ToInt32(vMax.X);
	int32 MaxY = RoundFloat32ToInt32(vMax.Y);

	if(MinX < 0) { MinX = 0; }
	if(MinY < 0) { MinY = 0; }
	if(MaxX > ScreenBuffer->Width) { MaxX = ScreenBuffer->Width; }
	if(MaxY > ScreenBuffer->Height) { MaxY = ScreenBuffer->Height; }

#define ALPHA_BLEND_RECTANGLE 1
#ifndef ALPHA_BLEND_RECTANGLE
	uint32 Color = ( (TruncateFloat32ToInt32(R * 255.f) << 16) |
					 (TruncateFloat32ToInt32(G * 255.f) <<  8) |
					 (TruncateFloat32ToInt32(B * 255.f) <<  0));
#else
	float32 SrcR = R * 255.f;
	float32 SrcG = G * 255.f;
	float32 SrcB = B * 255.f;
#endif

	uint8 *Row = (uint8 *)ScreenBuffer->Memory + 
				  MinY * ScreenBuffer->Pitch +
				  MinX * ScreenBuffer->BytesPerPixel;

	for(int Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = MinX; X < MaxX; ++X)
		{
#ifndef ALPHA_BLEND_RECTANGLE
			*Pixel++ = Color;
#else
			float32 DestA = (float32)((*Pixel >> 24) & 0xFF);
			float32 DestR = (float32)((*Pixel >> 16) & 0xFF);
			float32 DestG = (float32)((*Pixel >>  8) & 0xFF);
			float32 DestB = (float32)((*Pixel >>  0) & 0xFF);

			float32 PixelR = (1.f - A) * DestR + A * SrcR;
			float32 PixelG = (1.f - A) * DestG + A * SrcG;
			float32 PixelB = (1.f - A) * DestB + A * SrcB;

			*Pixel++ = ( ((uint32)(PixelR + 0.5f) << 16) |
					     ((uint32)(PixelG + 0.5f) <<  8) |
					     ((uint32)(PixelB + 0.5f) <<  0) );
#endif
		}
		Row += ScreenBuffer->Pitch;
	}
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

		bit_scan_result RScan = FindLeastSignificantSetBit(Header->RMask);
		bit_scan_result GScan = FindLeastSignificantSetBit(Header->GMask);
		bit_scan_result BScan = FindLeastSignificantSetBit(Header->BMask);
		bit_scan_result AScan = FindLeastSignificantSetBit(Header->AMask);

		Assert(RScan.Found);
		Assert(GScan.Found);
		Assert(BScan.Found);
		Assert(AScan.Found);

		uint32 RShift = 16 - (int32)RScan.Index;
		uint32 GShift =  8 - (int32)GScan.Index;
		uint32 BShift =  0 - (int32)BScan.Index;
		uint32 AShift = 24 - (int32)AScan.Index;

		uint32 *SourceDest = Pixels;
		for(int32 Y = 0; Y < Header->Height; Y++)
		{
			for(int32 X = 0; X < Header->Width; X++)
			{
				uint32 C = *SourceDest;
#if 0
				*SourceDest = ( (((C >> AScan.Index) & 0xFF) << 24) |
								(((C >> RScan.Index) & 0xFF) << 16) |
								(((C >> GScan.Index) & 0xFF) <<  8) |
								(((C >> BScan.Index) & 0xFF) <<  0) );
#else
				*SourceDest = ( RotateLeft(C & Header->RMask, RShift) |
								RotateLeft(C & Header->GMask, GShift) |
								RotateLeft(C & Header->BMask, BShift) |
								RotateLeft(C & Header->AMask, AShift) );
#endif
				++SourceDest;
			}
		}
	}
	return Result;
}

internal void 
ChangeEntityResidence(game_state *GameState, uint32 Index, entity_residence Residence)
{
	if(Residence == EntityResidence_High)
	{
		if(GameState->EntityResidence[Index] != EntityResidence_High)
		{
			dormant_entity  *EntityDormant = &GameState->DormantEntities[Index];
			high_entity  *EntityHigh = &GameState->HighEntities[Index];

			// NOTE: Map the entity into camera space
			tile_map_difference Diff = Substract(GameState->World->TileMap, &EntityDormant->Pos, &GameState->CameraPos);
			EntityHigh->Pos = Diff.dXY;
			EntityHigh->dPos = V2(0,0);
			EntityHigh->AbsTileZ = EntityDormant->Pos.AbsTileZ;
			EntityHigh->FacingDirection = 0;
		}
	}

	GameState->EntityResidence[Index] = Residence;
}

internal entity
GetEntity(game_state *GameState, entity_residence Residence, uint32 Index)
{
	entity Entity = {};

	if((Index > 0) && (Index < GameState->EntityCount))
	{
		if(GameState->EntityResidence[Index] < Residence)
		{
			ChangeEntityResidence(GameState, Index, Residence);
			Assert(GameState->EntityResidence[Index] >= Residence);
		}

		Entity.Residence = Residence;
		Entity.Dormant = &GameState->DormantEntities[Index];
		Entity.Low = &GameState->LowEntities[Index];
		Entity.High = &GameState->HighEntities[Index];
	}

	return Entity;
}

internal uint32
AddEntity(game_state *GameState)
{
	uint32 EntityIndex = GameState->EntityCount++;

	Assert(GameState->EntityCount < ArrayCount(GameState->DormantEntities));
	Assert(GameState->EntityCount < ArrayCount(GameState->LowEntities));
	Assert(GameState->EntityCount < ArrayCount(GameState->HighEntities));

	GameState->EntityResidence[EntityIndex] = EntityResidence_Dormant;
	GameState->DormantEntities[EntityIndex] = {};
	GameState->LowEntities[EntityIndex] = {};
	GameState->HighEntities[EntityIndex] = {};

	return EntityIndex;
}

internal void
InitializePlayer(game_state *GameState, uint32 EntityIndex)
{
	entity Entity = GetEntity(GameState, EntityResidence_Dormant, EntityIndex);

	Entity.Dormant->Pos.AbsTileX = 1;
	Entity.Dormant->Pos.AbsTileY = 3;
	Entity.Dormant->Pos.Offset_.X = 0.f;
	Entity.Dormant->Pos.Offset_.Y = 0.f;
	Entity.Dormant->Height = 0.5f;
	Entity.Dormant->Width  = 1.0f;
	Entity.Dormant->Collides = true;

	ChangeEntityResidence(GameState, EntityIndex, EntityResidence_High);

	if(GetEntity(GameState, EntityResidence_Dormant, GameState->CameraFollowingEntityIndex).Residence == EntityResidence_Nonexistent)
	{
		GameState->CameraFollowingEntityIndex =  EntityIndex;
	}
}

internal bool32
TestWall(float32 WallX, float32 RelX, float32 RelY, float32 PlayerDeltaX, float32 PlayerDeltaY,
		 float32 *tMin, float32 MinY, float32 MaxY)
{
	bool32 Hit = false;

	float32 tEpsilon = 0.001f;
	if(PlayerDeltaX != 0.0f)
	{
		float32 tResult = (WallX - RelX) / PlayerDeltaX;
		float32 Y = RelY + tResult * PlayerDeltaY;
		if((tResult >= 0.0f) && (*tMin > tResult))
		{
			if((Y >= MinY) && (Y <= MaxY))
			{
				// *tMin = Maximum(0.0f, tResult);
				*tMin = Maximum(0.0f, tResult - tEpsilon);
				Hit = true;
			}
		}
	}

	return Hit;
}

internal void
MovePlayer(game_state *GameState, entity Entity, float32 dt, v2 ddPos)
{
	tile_map *TileMap = GameState->World->TileMap;

	float32 ddPLength = LengthSq(ddPos);
	if(ddPLength > 1.0f)
	{
		ddPos *= 1.0f / SquareRoot(ddPLength);
	}

	float32 PlayerSpeed = 50.0f; // m/s^²
	ddPos *= PlayerSpeed;

	ddPos += -8.0f * Entity.High->dPos;

	v2 OldPlayerPos = Entity.High->Pos;
	v2 PlayerDelta = 0.5f * ddPos * Square(dt)  + Entity.High->dPos * dt;
	Entity.High->dPos = ddPos * dt + Entity.High->dPos;
	v2 NewPlayerPos = OldPlayerPos + PlayerDelta;

	float32 tRemaining = 1.0f;
	for(uint32 Iteration = 0;
		Iteration < 4 && tRemaining > 0.0f;
		++Iteration)
	{
		float32 tMin = 1.0f;
		v2 WallNormal = {};
		uint32 HitEntityIndex = 0;

		for(uint32 EntityIndex = 1;
			EntityIndex < GameState->EntityCount;
			++EntityIndex)
		{
			entity TestEntity = GetEntity(GameState, EntityResidence_High, EntityIndex);
			if(TestEntity.High != Entity.High)
			{
				if(TestEntity.Dormant->Collides)
				{
					float32 DiameterW = TestEntity.Dormant->Width + Entity.Dormant->Width;
					float32 DiameterH = TestEntity.Dormant->Height + Entity.Dormant->Height;

					v2 MinCorner = -0.5f * v2{DiameterW, DiameterH};
					v2 MaxCorner =  0.5f * v2{DiameterW, DiameterH};

					v2 Rel = Entity.High->Pos - TestEntity.High->Pos;

		            if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
		                        &tMin, MinCorner.Y, MaxCorner.Y))
		            {
		                WallNormal = v2{-1, 0};
		                HitEntityIndex = EntityIndex;
		            }
		        
		            if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
		                        &tMin, MinCorner.Y, MaxCorner.Y))
		            {
		                WallNormal = v2{1, 0};
		                HitEntityIndex = EntityIndex;
		            }
		        
		            if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
		                        &tMin, MinCorner.X, MaxCorner.X))
		            {
		                WallNormal = v2{0, -1};
		                HitEntityIndex = EntityIndex;
		            }
		        
		            if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
		                        &tMin, MinCorner.X, MaxCorner.X))
		            {
		                WallNormal = v2{0, 1};
		                HitEntityIndex = EntityIndex;
		            }
	        	}
        	}
		}

		Entity.High->Pos += tMin * PlayerDelta;
		if(HitEntityIndex)
		{
			Entity.High->dPos = Entity.High->dPos - 1 * Inner(Entity.High->dPos, WallNormal) * WallNormal;
			PlayerDelta = PlayerDelta - 1 * Inner(PlayerDelta, WallNormal) * WallNormal;
			tRemaining -= tMin * tRemaining;

			entity HitEntity = GetEntity(GameState, EntityResidence_Dormant, HitEntityIndex);
			Entity.High->AbsTileZ += Entity.Dormant->dAbsTileZ;
		}
		else
		{
			break;
		}
	}

	// TODO: Change to using the acceleration vector
	if(Entity.High->dPos.X == 0.0f && Entity.High->dPos.Y == 0.0f)
	{
		// NOTE: Leave facing direction whatever it was
	}
	else if(AbsoluteValue(Entity.High->dPos.X) > AbsoluteValue(Entity.High->dPos.Y))
	{
		if(Entity.High->dPos.X > 0)
		{
			Entity.High->FacingDirection = 0;
		}
		else
		{
			Entity.High->FacingDirection = 2;
		}
	}
	else
	{
		if(Entity.High->dPos.Y > 0)
		{
			Entity.High->FacingDirection = 1;
		}
		else
		{
			Entity.High->FacingDirection = 3;
		}
	}

	Entity.Dormant->Pos = MapIntoTileSpace(TileMap, GameState->CameraPos, Entity.High->Pos);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
	Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == 
		(ArrayCount(Input->Controllers[0].Buttons) - 1));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);


	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		// NOTE: Reserve entity slot 0 for null entity
		uint32 NullEntityIndex = AddEntity(GameState);
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

		GameState->CameraPos.AbsTileX = 17/2;
		GameState->CameraPos.AbsTileY =  9/2;

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

	for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		entity ControllingEntity = GetEntity(GameState, EntityResidence_High,
											  GameState->PlayerIndexForController[ControllerIndex]);
		if(ControllingEntity.Residence != EntityResidence_Nonexistent)
		{
			v2 ddPlayer = {};

			if(Controller->IsAnalog)
			{
				ddPlayer = {Controller->StickAverageX, Controller->StickAverageY};
			}
			else
			{
				if(Controller->MoveUp.EndedDown)
				{
					ddPlayer.Y = 1.0f;
				}
				if(Controller->MoveDown.EndedDown)
				{
					ddPlayer.Y = -1.0f;
				}
				if(Controller->MoveLeft.EndedDown)
				{
					ddPlayer.X = -1.0f;
				}
				if(Controller->MoveRight.EndedDown)
				{
					ddPlayer.X = 1.0f;
				}
			}

			if(Controller->ActionUp.EndedDown && ControllingEntity.High->Z == 0)
			{
				ControllingEntity.High->dZ = 3.0f;
			}

			MovePlayer(GameState, ControllingEntity, Input->dTForFrame, ddPlayer);
		}
		else
		{
			if(Controller->Start.EndedDown)
			{
				uint32 EntityIndex = AddEntity(GameState);
				InitializePlayer(GameState, EntityIndex);
				GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
			}
		}
	}

	v2 EntityOffsetForFrame = {};
	entity CameraFollowingEntity = GetEntity(GameState, EntityResidence_High, GameState->CameraFollowingEntityIndex);
	if(CameraFollowingEntity.Residence != EntityResidence_Nonexistent)
	{
		tile_map_position OldCameraPos = GameState->CameraPos;
		GameState->CameraPos.AbsTileZ = CameraFollowingEntity.Dormant->Pos.AbsTileZ;

		tile_map_difference Diff = Substract(TileMap, &CameraFollowingEntity.Dormant->Pos, &GameState->CameraPos);
		if(CameraFollowingEntity.High->Pos.X > (9.0f * TileMap->TileSideInMeters))
		{
			GameState->CameraPos.AbsTileX += 17;
		}
		else if(CameraFollowingEntity.High->Pos.X < -(9.0f * TileMap->TileSideInMeters))
		{
			GameState->CameraPos.AbsTileX -= 17;
		}

		if(CameraFollowingEntity.High->Pos.Y > (5.0f * TileMap->TileSideInMeters))
		{
			GameState->CameraPos.AbsTileY += 9;
		}
		else if(CameraFollowingEntity.High->Pos.Y < -(5.0f * TileMap->TileSideInMeters))
		{
			GameState->CameraPos.AbsTileY -= 9;
		}

		tile_map_difference dCameraPos = Substract(TileMap, &GameState->CameraPos, &OldCameraPos);
		EntityOffsetForFrame = -dCameraPos.dXY;
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
					Gray = 1.f;
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

				v2 TileSide = { 0.5f * TileSideInPixels, 0.5f * TileSideInPixels };
				v2 Cen = { ScreenCenterX - MetersToPixels * GameState->CameraPos.Offset_.X + ((float32)RelColumn) * TileSideInPixels,
						   ScreenCenterY + MetersToPixels * GameState->CameraPos.Offset_.Y - ((float32)RelRow) * TileSideInPixels };
		    	float32 TileScale = 1.0f;
			  	v2 Min = Cen - TileScale * TileSide;
			  	v2 Max = Cen + TileScale * TileSide;
				DrawRectangle(ScreenBuffer, Min, Max, Gray, Gray, Gray, 0.75f);
			}
		}
	}

	for(uint32 EntityIndex = 0;
		EntityIndex < GameState->EntityCount;
		++EntityIndex)
	{
		// TODO: Culling of entities based on Z
		if(GameState->EntityResidence[EntityIndex] == EntityResidence_High)
		{
			high_entity *HighEntity = &GameState->HighEntities[EntityIndex];
			low_entity *LowEntity = &GameState->LowEntities[EntityIndex];
			dormant_entity *DormantEntity = &GameState->DormantEntities[EntityIndex];

			HighEntity->Pos += EntityOffsetForFrame;

			float32 dt = Input->dTForFrame;
			float32 ddZ = -9.8f;
			HighEntity->Z = 0.5f * ddZ * Square(dt)  + HighEntity->dZ * dt + HighEntity->Z;
			HighEntity->dZ = ddZ * dt + HighEntity->dZ;

			if(HighEntity->Z < 0)
			{
				HighEntity->Z = 0;
			}

			float32 PlayerR = 1.0f;
			float32 PlayerG = 0.0f;
			float32 PlayerB = 1.0f; 
			float32 Z = -HighEntity->Z * MetersToPixels;
			v2 PlayerGroundPoint = { ScreenCenterX + MetersToPixels * HighEntity->Pos.X,
									 ScreenCenterY - MetersToPixels * HighEntity->Pos.Y };
			v2 PlayerLeftTop = { PlayerGroundPoint.X - MetersToPixels *  0.5f * DormantEntity->Width,
								 PlayerGroundPoint.Y - MetersToPixels *  0.5f * DormantEntity->Height };
			v2 EntityWidthHeight = { DormantEntity->Width, DormantEntity->Height };


			DrawRectangle(ScreenBuffer,
						  PlayerLeftTop, 
						  PlayerLeftTop + MetersToPixels * EntityWidthHeight,
						  PlayerR, PlayerG, PlayerB, 0.3f);

			hero_bitmaps *HeroBitmap = &GameState->HeroBitmaps[HighEntity->FacingDirection];
			DrawBitmap(ScreenBuffer, &HeroBitmap->Torso, PlayerGroundPoint.X, PlayerGroundPoint.Y + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);
			DrawBitmap(ScreenBuffer, &HeroBitmap->Cape,  PlayerGroundPoint.X, PlayerGroundPoint.Y + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);
			DrawBitmap(ScreenBuffer, &HeroBitmap->Head,  PlayerGroundPoint.X, PlayerGroundPoint.Y + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);

			DebugPoint(ScreenBuffer, 
					   FloorFloat32ToInt32(PlayerLeftTop.X + MetersToPixels * 0.5f * DormantEntity->Width),
					   FloorFloat32ToInt32(PlayerLeftTop.Y + MetersToPixels * DormantEntity->Height),
					   0xFFFF0000, 3);

			DebugPoint(ScreenBuffer, 
					   FloorFloat32ToInt32(PlayerLeftTop.X),
					   FloorFloat32ToInt32(PlayerLeftTop.Y + MetersToPixels * DormantEntity->Height),
					   0xFF00FF00, 3);

			DebugPoint(ScreenBuffer, 
					   FloorFloat32ToInt32(PlayerLeftTop.X + MetersToPixels * DormantEntity->Width),
					   FloorFloat32ToInt32(PlayerLeftTop.Y + MetersToPixels * DormantEntity->Height),
					   0xFF0000FF, 3);
		}
	}
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}