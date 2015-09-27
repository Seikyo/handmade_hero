#ifndef GAME_PLATFORM_H
#define GAME_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSCV && !COMPILER_LLVM
	#if _MSC_VER
	#undef  COMPILER_MSVC
	#define COMPILER_MSVC 1
	#else
	#undef  COMPILER_LLVM
	#define COMPILER_LLVM 1
	#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#endif

#include <stdint.h>
#include <stddef.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index;

typedef int32 bool32;
typedef float float32;
typedef double float64;

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#if GAME_ASSERTS
#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath");

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

typedef struct thread_context
{
	int Placeholder;
} thread_context;

#if INTERNAL

	typedef struct debug_read_file_result
	{
		uint32 ContentsSize;
		void *Contents;
	} debug_read_file_result;

	#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *Filename)
	typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

	#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *Thread, char *Filename, uint32 MemorySize, void *Memory)
	typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

	#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
	typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#endif

/*
	TODO: Services that the platform layer provides to the game
*/

/*
	NOTE: Services that the game provides to the platform layer
	(this may expand in the future - sound on separate thread ...)
*/
typedef struct game_offscreen_buffer
{
    // NOTE: Pixels are always 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
} game_offscreen_buffer;

typedef struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
} game_sound_output_buffer;

typedef struct game_button_state
{
	int HalfTransitionCount;
	bool32 EndedDown;
} game_button_state;

typedef struct game_controller_input
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
} game_controller_input;

typedef struct game_input
{
	game_button_state MouseButtons[5];
	int32 MouseX, MouseY, MouseZ;

	float32 dTForFrame;
	game_controller_input Controllers[5];
} game_input;

typedef struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE:  REQUIRED to be cleared to zero

	uint64 TransientStorageSize;
	void *TransientStorage; // NOTE:  REQUIRED to be cleared to zero

	debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
	debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
	debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
} game_memory;


#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_offscreen_buffer *ScreenBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// WARNING: < 1ms !
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input *Result = &Input->Controllers[ControllerIndex];
	return Result; 
}

#ifdef __cplusplus
}
#endif

#endif