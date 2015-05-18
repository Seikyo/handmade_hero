#ifndef GAME_H
#define GAME_H

/*
Compiler flags
	- INTERNAL:
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
    return Result;
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
	bool32 IsConnected;
	bool32 IsAnalog;
	float32 StickAverageX;
	float32 StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;
			
			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoudler;

			game_button_state Back;
			game_button_state Start;
		};
	};
};

struct game_input
{
	// TODO: Insert clock values here
	game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input *Result = &Input->Controllers[ControllerIndex];
	return Result; 
}


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
	game_memory *Memory,
	game_input *Input,
	game_offscreen_buffer *ScreenBuffer
);

// WARNING: < 1ms !
internal void GameGetSoundSamples(
	game_memory *Memory,
	game_sound_output_buffer *SoundBuffer
);

#if INTERNAL
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