#ifndef GAME_H
#define GAME_H

/*
Compiler flags
	- GAME_RELEASE:
		0 - Release build
		1 - Developper build

	- GAME_ASSERTS:
		0 - Ignore asserts
		1 - Enable asserts
*/

#if GAME_ASSERTS
#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Value;
}

/*
	TODO: Services that the platform layer provides to the game
*/

/*
	NOTE: Services that the game provides to the platform layer
	(this may expand in the future - sound on separate thread ...)
*/
struct game_offscreen_buffer
{
    // NOTE: Pixels are always 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

struct game_button_state
{
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
{
	bool32 IsAnalog;

	float32 StartX;
	float32 StartY;

	float32 MinX;
	float32 MinY;

	float32 MaxX;
	float32 MaxY;

	float32 EndX;
	float32 EndY;
	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoudler;
		};
	};
};

struct game_input
{
	// TODO: Insert clock values here
	game_controller_input Controllers[4];
};

struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE:  REQUIRED to be cleared to zero

	uint64 TransientStorageSize;
	void *TransientStorage; // NOTE:  REQUIRED to be cleared to zero
};

struct game_state
{
	int ToneHz;
	int GreenOffset;
	int BlueOffset;
};

internal void GameUpdateAndRender(
	game_input *Input,
	game_offscreen_buffer *ScreenBuffer, 
	game_sound_output_buffer *SoundBuffer
);

#if GAME_RELEASE != 1
struct debug_read_file_result
{
	uint32 ContentsSize;
	void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
#endif

#endif