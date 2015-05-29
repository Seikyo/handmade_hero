#ifndef GAME_TILE_H
#define GAME_TILE_H

struct tile_map_position
{
	// NOTE: Fixed point tile locations
	// High bits are the tile chunk index
	// Low  bits are the tile index in the chunk
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;

	// NOTE: Tile relatives position
	float32 OffsetX;
	float32 OffsetY;
};

struct tile_chunk_position
{
	uint32 TileChunkX;
	uint32 TileChunkY;
	uint32 TileChunkZ;

	uint32 TileRelX;
	uint32 TileRelY;
};

struct tile_chunk
{
	uint32 *Tiles;
};

struct tile_map
{
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	float32 TileSideInMeters;

	// TODO: Real sparseness
	uint32 TileChunkCountX;
	uint32 TileChunkCountY;
	uint32 TileChunkCountZ;
	tile_chunk *TileChunks;
};

#endif