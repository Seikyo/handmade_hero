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
	int32 AlignX = 0, int32 AlignY = 0, float32 CAlpha = 1.f)
{
	FloatX -= (float32)AlignX;
	FloatY -= (float32)AlignY;

	int32 MinX = RoundFloat32ToInt32(FloatX);
	int32 MinY = RoundFloat32ToInt32(FloatY);
	int32 MaxX = MinX + Bitmap->Width;
	int32 MaxY = MinY + Bitmap->Height;

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
			A *= CAlpha;

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

inline low_entity *
GetLowEntity(game_state *GameState, uint32 Index)
{
	low_entity *Result = {};

	if((Index > 0) && (Index < GameState->LowEntityCount))
	{
		Result = GameState->LowEntities + Index;
	}

	return Result;
}

inline high_entity *
MakeEntityHighFrequency(game_state *GameState, uint32 LowIndex)
{
	high_entity *EntityHigh = 0;
	low_entity  *EntityLow = &GameState->LowEntities[LowIndex];
	if(EntityLow->HighEntityIndex)
	{
		EntityHigh = GameState->HighEntities_ + EntityLow->HighEntityIndex;
	}
	else
	{
		if(GameState->HighEntityCount < ArrayCount(GameState->HighEntities_))
		{
			uint32 HighIndex = GameState->HighEntityCount++;
			EntityHigh = GameState->HighEntities_ + HighIndex;

			// NOTE: Map the entity into camera space
			tile_map_difference Diff = Substract(GameState->World->TileMap, &EntityLow->Pos, &GameState->CameraPos);
			EntityHigh->Pos = Diff.dXY;
			EntityHigh->dPos = V2(0,0);
			EntityHigh->AbsTileZ = EntityLow->Pos.AbsTileZ;
			EntityHigh->FacingDirection = 0;
			EntityHigh->LowEntityIndex = LowIndex;

			EntityLow->HighEntityIndex = HighIndex;
		}
		else
		{
			InvalidCodePath;
		}
	}

	return EntityHigh;
}

inline entity
GetHighEntity(game_state *GameState, uint32 LowIndex)
{
	entity Result = {};

	if((LowIndex > 0) && (LowIndex < GameState->LowEntityCount))
	{
		Result.LowIndex = LowIndex;
		Result.Low = GameState->LowEntities + LowIndex;
		Result.High = MakeEntityHighFrequency(GameState,LowIndex);
	}

	return Result;
}


inline void 
MakeEntityLowFrequency(game_state *GameState, uint32 LowIndex)
{
	low_entity  *EntityLow = &GameState->LowEntities[LowIndex];
	uint32 HighIndex = EntityLow->HighEntityIndex;
	if(HighIndex)
	{
		uint32 LastHighIndex = GameState->HighEntityCount - 1;
		if(HighIndex != LastHighIndex)
		{
			high_entity *LastEntity = GameState->HighEntities_ + LastHighIndex;
			high_entity *DeletedEntity = GameState->HighEntities_ + HighIndex;

			*DeletedEntity = *LastEntity;
			GameState->LowEntities[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
		}
		--GameState->HighEntityCount;
		EntityLow->HighEntityIndex = 0;
	}
}

inline void
OffsetAndCheckFrequencyByArea(game_state *GameState, v2 Offset, rectangle2 CameraBounds)
{
	for(uint32 EntityIndex = 1;
		EntityIndex < GameState->HighEntityCount;
		)
	{
		high_entity *High = GameState->HighEntities_ + EntityIndex;

		High->Pos += Offset;
		if(IsInRectangle(CameraBounds, High->Pos))
		{
			++EntityIndex;
		}
		else
		{
			MakeEntityLowFrequency(GameState, High->LowEntityIndex);
		}
	}
}

internal uint32
AddLowEntity(game_state *GameState, entity_type Type)
{
	Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
	uint32 EntityIndex = GameState->LowEntityCount++;

	GameState->LowEntities[EntityIndex] = {};
	GameState->LowEntities[EntityIndex].Type = Type;

	return EntityIndex;
}

internal uint32
AddPlayer(game_state *GameState)
{
	uint32 EntityIndex = AddLowEntity(GameState, EntityType_Hero);
	low_entity *EntityLow = GetLowEntity(GameState, EntityIndex);

	EntityLow->Pos = GameState->CameraPos;
	EntityLow->Pos.Offset_.X = 0.f;
	EntityLow->Pos.Offset_.Y = 0.f;
	EntityLow->Height = 0.5f;
	EntityLow->Width  = 1.0f;
	EntityLow->Collides = true;

	if(GameState->CameraFollowingEntityIndex == 0)
	{
		GameState->CameraFollowingEntityIndex =  EntityIndex;
	}

	return EntityIndex;
}

internal uint32
AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	uint32 EntityIndex = AddLowEntity(GameState, EntityType_Wall);
	low_entity *EntityLow = GetLowEntity(GameState, EntityIndex);

	EntityLow->Pos.AbsTileX = AbsTileX;
	EntityLow->Pos.AbsTileY = AbsTileY;
	EntityLow->Pos.AbsTileZ = AbsTileZ;
	EntityLow->Height = GameState->World->TileMap->TileSideInMeters;
	EntityLow->Width  = GameState->World->TileMap->TileSideInMeters;
	EntityLow->Collides = true;

	return EntityIndex;
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

	for(uint32 Iteration = 0;
		Iteration < 4;
		++Iteration)
	{
		float32 tMin = 1.0f;
		v2 WallNormal = {};
		uint32 HitHighEntityIndex = 0;

		v2 DesiredPosition = Entity.High->Pos + PlayerDelta;

		for(uint32 TestHighEntityIndex = 1;
			TestHighEntityIndex < GameState->HighEntityCount;
			++TestHighEntityIndex)
		{
			if(TestHighEntityIndex != Entity.Low->HighEntityIndex)
			{
				entity TestEntity;
				TestEntity.High = GameState->HighEntities_ + TestHighEntityIndex;
				TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
				TestEntity.Low = GameState->LowEntities + TestEntity.LowIndex;
				if(TestEntity.Low->Collides)
				{
					float32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
					float32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;

					v2 MinCorner = -0.5f * v2{DiameterW, DiameterH};
					v2 MaxCorner =  0.5f * v2{DiameterW, DiameterH};

					v2 Rel = Entity.High->Pos - TestEntity.High->Pos;

		            if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
		                        &tMin, MinCorner.Y, MaxCorner.Y))
		            {
		                WallNormal = v2{-1, 0};
		                HitHighEntityIndex = TestHighEntityIndex;
		            }
		        
		            if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
		                        &tMin, MinCorner.Y, MaxCorner.Y))
		            {
		                WallNormal = v2{1, 0};
		                HitHighEntityIndex = TestHighEntityIndex;
		            }
		        
		            if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
		                        &tMin, MinCorner.X, MaxCorner.X))
		            {
		                WallNormal = v2{0, -1};
		                HitHighEntityIndex = TestHighEntityIndex;
		            }
		        
		            if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
		                        &tMin, MinCorner.X, MaxCorner.X))
		            {
		                WallNormal = v2{0, 1};
		                HitHighEntityIndex = TestHighEntityIndex;
		            }
	        	}
        	}
		}

		Entity.High->Pos += tMin * PlayerDelta;
		if(HitHighEntityIndex)
		{
			Entity.High->dPos = Entity.High->dPos - 1 * Inner(Entity.High->dPos, WallNormal) * WallNormal;
			PlayerDelta = DesiredPosition - Entity.High->Pos;
			PlayerDelta = PlayerDelta - 1 * Inner(PlayerDelta, WallNormal) * WallNormal;

			high_entity *HitHigh = GameState->HighEntities_ + HitHighEntityIndex;
			low_entity *HitLow = GameState->LowEntities + HitHigh->LowEntityIndex;
			Entity.High->AbsTileZ += Entity.Low->dAbsTileZ;
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

	Entity.Low->Pos = MapIntoTileSpace(TileMap, GameState->CameraPos, Entity.High->Pos);
}

internal void
SetCamera(game_state *GameState, tile_map_position NewCameraPos)
{
	tile_map *TileMap = GameState->World->TileMap;

	tile_map_difference dCameraPos = Substract(TileMap, &NewCameraPos, &GameState->CameraPos);
	GameState->CameraPos = NewCameraPos;

	// TODO: Numbers taken randomly atm
	uint32 TileSpanX = 17 * 3; 
	uint32 TileSpanY = 9 * 3;
	rectangle2 CameraBounds = RectCenterHalfDim( V2(0, 0), 
												 TileMap->TileSideInMeters * V2( (float32)TileSpanX, 
												 								 (float32)TileSpanY) );
	v2 EntityOffsetForFrame = -dCameraPos.dXY;
	OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);

	// TODO: Move entities into high set here

	uint32 MinTileX = NewCameraPos.AbsTileX - TileSpanX / 2;
	uint32 MinTileY = NewCameraPos.AbsTileX + TileSpanX / 2;
	uint32 MaxTileX = NewCameraPos.AbsTileY - TileSpanY / 2;
	uint32 MaxTileY = NewCameraPos.AbsTileY + TileSpanY / 2;
	for(uint32 EntityIndex = 1;
		EntityIndex < GameState->LowEntityCount;
		++EntityIndex)
	{
		low_entity *Low = GameState->LowEntities + EntityIndex;
		if(Low->HighEntityIndex == 0)
		{
#if 0			
			if( Low->Pos.AbsTileZ == NewCameraPos.AbsTileZ &&
				Low->Pos.AbsTileX >= MinTileX &&
				Low->Pos.AbsTileX <= MaxTileX &&
				Low->Pos.AbsTileY <= MinTileY &&
				Low->Pos.AbsTileY >= MaxTileY )
#endif				
			{
				MakeEntityHighFrequency(GameState, EntityIndex);
			}
		}
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
		// NOTE: Reserve entity slot 0 for null entity
		AddLowEntity(GameState, EntityType_Null);
		GameState->HighEntityCount = 1;

		GameState->BackDrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, 
										   "test/test_background.bmp");
		GameState->Shadow = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, 
										   "test/test_hero_shadow.bmp");

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

		InitializeTileMap(TileMap, 1.4f);

		uint32 RandomNumberIndex = 0;
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenBaseX = (INT16_MAX / TilesPerWidth) / 2;
		uint32 ScreenBaseY = (INT16_MAX / TilesPerHeight) / 2;
		uint32 ScreenBaseZ = INT16_MAX / 2;
// Centered world
#if 1
		uint32 ScreenX = ScreenBaseX;
		uint32 ScreenY = ScreenBaseY;
		uint32 AbsTileZ = ScreenBaseZ;
#else
		uint32 ScreenX = 0;
		uint32 ScreenY = 0;
		uint32 AbsTileZ = 0;
#endif

		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;
		for(uint32 ScreenIndex = 0; ScreenIndex < 2; ++ScreenIndex)
		{
			// TODO: Random number generator
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));

			uint32 RandomChoice;
			// if(DoorUp || DoorDown)
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
			}
			// else 
			// {
			// 	RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
			// }

			bool32 CreatedZDoor = false;
			if(RandomChoice == 2)
			{
				CreatedZDoor = true;
				if(AbsTileZ == ScreenBaseZ)
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

					SetTileValue(&GameState->WorldArea, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);

					if(TileValue == 2)
					{
						AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
					}
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
				if(AbsTileZ == ScreenBaseZ)
				{
					AbsTileZ = ScreenBaseZ + 1;
				}
				else
				{
					AbsTileZ = ScreenBaseZ;
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

		tile_map_position NewCameraPos = {};
		NewCameraPos.AbsTileX = ScreenBaseX * TilesPerWidth + 17/2;
		NewCameraPos.AbsTileY = ScreenBaseY * TilesPerHeight + 9/2;
		NewCameraPos.AbsTileZ = ScreenBaseZ;
		SetCamera(GameState, NewCameraPos);

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
		uint32 LowIndex = GameState->PlayerIndexForController[ControllerIndex];
		if(LowIndex == 0)
		{
			if(Controller->Start.EndedDown)
			{
				uint32 EntityIndex = AddPlayer(GameState);
				GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
			}
		}
		else
		{
			entity ControllingEntity = GetHighEntity(GameState, LowIndex);
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
	}

	entity CameraFollowingEntity = GetHighEntity(GameState, GameState->CameraFollowingEntityIndex);
	if(CameraFollowingEntity.High)
	{
		tile_map_position NewCameraPos = GameState->CameraPos;
		NewCameraPos.AbsTileZ = CameraFollowingEntity.Low->Pos.AbsTileZ;
#if 1
		tile_map_difference Diff = Substract(TileMap, &CameraFollowingEntity.Low->Pos, &GameState->CameraPos);
		if(CameraFollowingEntity.High->Pos.X > (9.0f * TileMap->TileSideInMeters))
		{
			NewCameraPos.AbsTileX += 17;
		}
		else if(CameraFollowingEntity.High->Pos.X < -(9.0f * TileMap->TileSideInMeters))
		{
			NewCameraPos.AbsTileX -= 17;
		}

		if(CameraFollowingEntity.High->Pos.Y > (5.0f * TileMap->TileSideInMeters))
		{
			NewCameraPos.AbsTileY += 9;
		}
		else if(CameraFollowingEntity.High->Pos.Y < -(5.0f * TileMap->TileSideInMeters))
		{
			NewCameraPos.AbsTileY -= 9;
		}
#else
		tile_map_difference Diff = Substract(TileMap, &CameraFollowingEntity.Low->Pos, &GameState->CameraPos);
		if(CameraFollowingEntity.High->Pos.X > (1.0f * TileMap->TileSideInMeters))
		{
			NewCameraPos.AbsTileX += 1;
		}
		else if(CameraFollowingEntity.High->Pos.X < -(1.0f * TileMap->TileSideInMeters))
		{
			NewCameraPos.AbsTileX -= 1;
		}

		if(CameraFollowingEntity.High->Pos.Y > (1.0f * TileMap->TileSideInMeters))
		{
			NewCameraPos.AbsTileY += 1;
		}
		else if(CameraFollowingEntity.High->Pos.Y < -(1.0f * TileMap->TileSideInMeters))
		{
			NewCameraPos.AbsTileY -= 1;
		}
#endif
		SetCamera(GameState, NewCameraPos);
	}

	DrawBitmap(ScreenBuffer, &GameState->BackDrop, 0, 0);

	float32 ScreenCenterX = 0.5f * (float32)ScreenBuffer->Width;
	float32 ScreenCenterY = 0.5f * (float32)ScreenBuffer->Height;

#if 0
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
#endif

	for(uint32 HighEntityIndex = 0;
		HighEntityIndex < GameState->HighEntityCount;
		++HighEntityIndex)
	{
		// TODO: Culling of entities based on Z
		high_entity *HighEntity = GameState->HighEntities_ + HighEntityIndex;
		low_entity *LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;

		float32 dt = Input->dTForFrame;
		float32 ddZ = -9.8f;
		HighEntity->Z = 0.5f * ddZ * Square(dt)  + HighEntity->dZ * dt + HighEntity->Z;
		HighEntity->dZ = ddZ * dt + HighEntity->dZ;

		if(HighEntity->Z < 0)
		{
			HighEntity->Z = 0;
		}

		float32 CAlpha = 1.0f - HighEntity->Z;
		if(CAlpha < 0.f)
		{
			CAlpha = 0.f;
		}

		float32 PlayerR = 1.0f;
		float32 PlayerG = 0.0f;
		float32 PlayerB = 1.0f; 
		float32 Z = -HighEntity->Z * MetersToPixels;
		v2 PlayerGroundPoint = { ScreenCenterX + MetersToPixels * HighEntity->Pos.X,
								 ScreenCenterY - MetersToPixels * HighEntity->Pos.Y };
		v2 PlayerLeftTop = { PlayerGroundPoint.X - MetersToPixels *  0.5f * LowEntity->Width,
							 PlayerGroundPoint.Y - MetersToPixels *  0.5f * LowEntity->Height };
		v2 EntityWidthHeight = { LowEntity->Width, LowEntity->Height };

		if(LowEntity->Type == EntityType_Hero)
		{
			hero_bitmaps *HeroBitmap = &GameState->HeroBitmaps[HighEntity->FacingDirection];
			DrawBitmap(ScreenBuffer, &GameState->Shadow, PlayerGroundPoint.X, PlayerGroundPoint.Y, HeroBitmap->AlignX, HeroBitmap->AlignY, CAlpha);
			DrawBitmap(ScreenBuffer, &HeroBitmap->Torso, PlayerGroundPoint.X, PlayerGroundPoint.Y + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);
			DrawBitmap(ScreenBuffer, &HeroBitmap->Cape,  PlayerGroundPoint.X, PlayerGroundPoint.Y + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);
			DrawBitmap(ScreenBuffer, &HeroBitmap->Head,  PlayerGroundPoint.X, PlayerGroundPoint.Y + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);
			DrawRectangle(ScreenBuffer,
						  PlayerLeftTop, 
						  PlayerLeftTop + MetersToPixels * EntityWidthHeight,
						  PlayerR, PlayerG, PlayerB, 0.3f);
		}
		else
		{
			DrawRectangle(ScreenBuffer,
						  PlayerLeftTop, 
						  PlayerLeftTop + MetersToPixels * EntityWidthHeight,
						  PlayerR, PlayerG, PlayerB);
		}


		// DebugPoint(ScreenBuffer, 
		// 		   FloorFloat32ToInt32(PlayerLeftTop.X + MetersToPixels * 0.5f * LowEntity->Width),
		// 		   FloorFloat32ToInt32(PlayerLeftTop.Y + MetersToPixels * LowEntity->Height),
		// 		   0xFFFF0000, 3);

		// DebugPoint(ScreenBuffer, 
		// 		   FloorFloat32ToInt32(PlayerLeftTop.X),
		// 		   FloorFloat32ToInt32(PlayerLeftTop.Y + MetersToPixels * LowEntity->Height),
		// 		   0xFF00FF00, 3);

		// DebugPoint(ScreenBuffer, 
		// 		   FloorFloat32ToInt32(PlayerLeftTop.X + MetersToPixels * LowEntity->Width),
		// 		   FloorFloat32ToInt32(PlayerLeftTop.Y + MetersToPixels * LowEntity->Height),
		// 		   0xFF0000FF, 3);
	}
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}