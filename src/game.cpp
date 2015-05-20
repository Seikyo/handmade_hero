#include "game.h"

internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, game_state *GameState)
{
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / GameState->ToneHz;

	int16 * SampleOut = SoundBuffer->Samples;
	for(int SampleIndex=0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		float32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		GameState->tSine += 2.f * Pi32 / (float32)WavePeriod;
		if(GameState->tSine > 2.f*Pi32)
		{
			GameState->tSine -= 2.f*Pi32;
		}
	}
}

internal void
RenderWeirGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0; X < Buffer->Width; ++X)
        {
            uint8 Blue = (uint8)(X + BlueOffset);
            uint8 Green = (uint8)(Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
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
		char *Filename = __FILE__;

		debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
		if(File.Contents)
		{
			Memory->DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			Memory->DEBUGPlatformFreeFileMemory(File.Contents);
		}
		GameState->ToneHz = 440;
		GameState->tSine = 0.f;
		Memory->IsInitialized = true;
	}

	for(int ControllerIndex=0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if(Controller->IsAnalog)
		{
			GameState->BlueOffset += (int)(4.f * Controller->StickAverageX);
			GameState->ToneHz = 440 + (int)(128.f * Controller->StickAverageY);
		}
		else
		{
			if(Controller->MoveLeft.EndedDown)
			{
				GameState->BlueOffset -= 1;
			}
			if(Controller->MoveRight.EndedDown)
			{
				GameState->BlueOffset += 1;
			}
		}

		if(Controller->ActionDown.EndedDown)
		{
			GameState->GreenOffset += 1;
		}
	}

	RenderWeirGradient(ScreenBuffer, GameState->BlueOffset, GameState->GreenOffset);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}