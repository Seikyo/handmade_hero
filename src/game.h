#ifndef GAME_H
#define GAME_H

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

/*
	TODO: Services that the platform layer provides to the game
*/

/*
	NOTE: Services that the game provides to the platform layer
	(this may expand in the future - sound on separate thread ...)
*/
// FOUR THINGS -> Timing, Controller/Keyboard input, Bitmap buffer to use, Sound buffer to user

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
	game_controller_input Controllers[4];
};

internal void GameUpdateAndRender(
	game_input *Input,
	game_offscreen_buffer *ScreenBuffer, 
	game_sound_output_buffer *SoundBuffer
);

#endif