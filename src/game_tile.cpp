#include  "game_tile.h"

inline void
RecanonicalizeCoord(tile_map *TileMap, uint32 *TileCoord, float32 *TileRelCoord)
{
	int32 Offset   = RoundFloat32ToInt32(*TileRelCoord / TileMap->TileSideInMeters);
	*TileCoord    += Offset;
	*TileRelCoord -= Offset * TileMap->TileSideInMeters;

	Assert(*TileRelCoord >= -0.5f * TileMap->TileSideInMeters);
	Assert(*TileRelCoord <=  0.5f * TileMap->TileSideInMeters);
}

inline tile_map_position
RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos)
{
	tile_map_position Result = Pos;

	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.OffsetX);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.OffsetY);

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
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ)
{
	tile_chunk *TileChunk = 0;
	if( TileChunkX >= 0 && TileChunkX < TileMap->TileChunkCountX &&
		TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY &&
		TileChunkZ >= 0 && TileChunkZ < TileMap->TileChunkCountZ)
	{
		TileChunk = &TileMap->TileChunks[
			TileChunkZ * TileMap->TileChunkCountX * TileMap->TileChunkCountY + 
			TileChunkY * TileMap->TileChunkCountX + 
			TileChunkX];
	}
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
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
	uint32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
	bool32 Empty = ( (TileChunkValue == 1) || 
					 (TileChunkValue == 3) || 
					 (TileChunkValue == 4) );

	return Empty;
}

internal void
SetTileValue(memory_area *Area, tile_map *TileMap, 
		     uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);

	Assert(TileChunk);
	if(!TileChunk->Tiles)
	{
		uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
		TileChunk->Tiles = PushArray(Area, TileCount, uint32);
		for(uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
		{
			TileChunk->Tiles[TileIndex] = 1;
		}
	}

	SetTileValue(TileMap, TileChunk, ChunkPos.TileRelX, ChunkPos.TileRelY, TileValue);
}