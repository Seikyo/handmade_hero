#ifndef GAME_TILE_H
#define GAME_TILE_H

struct tile_map_difference
{
	v2 dXY;
	float32 dZ;
};

struct tile_map_position
{
	// NOTE: Fixed point tile locations
	// High bits are the tile chunk index
	// Low  bits are the tile index in the chunk
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;

	// NOTE: Tile relatives position
	v2 Offset_;
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
	uint32 TileChunkX;
	uint32 TileChunkY;
	uint32 TileChunkZ;

	uint32 *Tiles;

	tile_chunk *NextInHash;
};

struct tile_map
{
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	float32 TileSideInMeters;

	tile_chunk TileChunkHash[4096];
};

#endif