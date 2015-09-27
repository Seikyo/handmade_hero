#include  "game_tile.h"

#define TILE_CHUNK_SAFE_MARGIN 256

internal void
InitializeTileMap(tile_map *TileMap, float32 TileSideInMeters)
{
	TileMap->ChunkShift = 4; 
	TileMap->ChunkMask  = (1 << TileMap->ChunkShift) - 1;
	TileMap->ChunkDim   = (1 << TileMap->ChunkShift);
	TileMap->TileSideInMeters = TileSideInMeters;

	for(uint32 TileChunkIndex = 0;
		TileChunkIndex < ArrayCount(TileMap->TileChunkHash);
		++TileChunkIndex)
	{
		TileMap->TileChunkHash[TileChunkIndex].TileChunkX = 0;
	}
}

inline void
RecanonicalizeCoord(tile_map *TileMap, uint32 *TileCoord, float32 *TileRelCoord)
{
	int32 Offset   = RoundFloat32ToInt32(*TileRelCoord / TileMap->TileSideInMeters);
	*TileCoord    += Offset;
	*TileRelCoord -= Offset * TileMap->TileSideInMeters;

	Assert(*TileRelCoord > -0.5f * TileMap->TileSideInMeters);
	Assert(*TileRelCoord <  0.5f * TileMap->TileSideInMeters);
}

inline tile_map_position
MapIntoTileSpace(tile_map *TileMap, tile_map_position Pos, v2 Offset)
{
	tile_map_position Result = Pos;

	Result.Offset_ += Offset;
	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset_.X);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset_.Y);

	return Result;
}

inline bool32
AreOnSameTile(tile_map_position *A, tile_map_position *B)
{
	bool32 Result = ( (A->AbsTileX == B->AbsTileX) &&
					  (A->AbsTileY == B->AbsTileY) &&
					  (A->AbsTileZ == B->AbsTileZ) );
	return Result;
}

inline tile_chunk *
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ, memory_area *Area = 0)
{
	tile_chunk *TileChunk = 0;

	Assert(TileChunkX < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));
	Assert(TileChunkY < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));
	Assert(TileChunkZ < (UINT32_MAX - TILE_CHUNK_SAFE_MARGIN));
	Assert(TileChunkX > TILE_CHUNK_SAFE_MARGIN);
	Assert(TileChunkY > TILE_CHUNK_SAFE_MARGIN);
	Assert(TileChunkZ > TILE_CHUNK_SAFE_MARGIN);

	// TODO: Better hash function
	uint32 HashValue = 19*TileChunkX + 7*TileChunkY + 3*TileChunkZ;
	uint32 HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
	Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));

	tile_chunk *Chunk = TileMap->TileChunkHash + HashSlot;
	do {
		if( (TileChunkX == Chunk->TileChunkX) && 
			(TileChunkY == Chunk->TileChunkY) && 
			(TileChunkZ == Chunk->TileChunkZ) )
		{
			break;
		}

		if(Area && (TileChunkX == 0) && (!Chunk->NextInHash))
		{
			Chunk->NextInHash = PushSize(Area, tile_chunk);
			Chunk->TileChunkX = 0;
			Chunk = Chunk->NextInHash;
		}

		if(Area && (Chunk->TileChunkX == 0))
		{
			uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;

			Chunk->TileChunkX = TileChunkX;
			Chunk->TileChunkY = TileChunkY;
			Chunk->TileChunkZ = TileChunkZ;

			Chunk->Tiles = PushArray(Area, TileCount, uint32);
			for(uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
			{
				Chunk->Tiles[TileIndex] = 1;
			}
			Chunk->NextInHash = 0;

			break;
		}

		Chunk = Chunk->NextInHash;
	} while(Chunk);

	return TileChunk;
}

inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY)
{
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);

	uint32 TileChunkValue = TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX];
	return TileChunkValue;
}

inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	tile_chunk_position Result;
	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.TileRelX = AbsTileX & TileMap->ChunkMask;
	Result.TileRelY = AbsTileY & TileMap->ChunkMask;

	return Result;
}

inline uint32
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY)
{
	uint32 TileChunkValue = 0;

	if(TileChunk && TileChunk->Tiles)
	{
		TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TileX, TileY);
	}

	return TileChunkValue;
}

internal uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
	uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.TileRelX, ChunkPos.TileRelY);

	return TileChunkValue;
}

internal uint32
GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
	uint32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);

	return TileChunkValue;
}

inline void
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk,
					  uint32 TileX, uint32 TileY, uint32 TileValue)
{
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);

	TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX] = TileValue;
}

inline void
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk,
			 uint32 TileX, uint32 TileY, uint32 TileValue)
{
	if(TileChunk && TileChunk->Tiles)
	{
		SetTileValueUnchecked(TileMap, TileChunk, TileX, TileY, TileValue);
	}
}

internal bool32
IsTileValueEmpty(uint32 TileValue)
{
	bool32 Empty = ( (TileValue == 1) || 
					 (TileValue == 3) || 
					 (TileValue == 4) );
	return Empty;
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
	uint32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
	bool32 Empty =  IsTileValueEmpty(TileChunkValue);

	return Empty;
}

internal void
SetTileValue(memory_area *Area, tile_map *TileMap, 
		     uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ, Area);
	SetTileValue(TileMap, TileChunk, ChunkPos.TileRelX, ChunkPos.TileRelY, TileValue);
}

inline tile_map_difference
Substract(tile_map *TileMap, tile_map_position *A, tile_map_position *B)
{
	tile_map_difference Result;

	v2 dTileXY = { (float32)A->AbsTileX - (float32)B->AbsTileX,
				   (float32)A->AbsTileY - (float32)B->AbsTileY };
	float32 dTileZ = (float32)A->AbsTileZ - (float32)B->AbsTileZ;

	Result.dXY = TileMap->TileSideInMeters * dTileXY + (A->Offset_ - B->Offset_);
	Result.dZ = TileMap->TileSideInMeters * dTileZ;

	return Result;
}

inline tile_map_position
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	tile_map_position Result = {};

	Result.AbsTileX = AbsTileX;
	Result.AbsTileY = AbsTileY;
	Result.AbsTileZ = AbsTileZ;

	return Result;
}