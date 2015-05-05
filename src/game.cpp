#include "game.h"

internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	local_persist float32 tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 * SampleOut = SoundBuffer->Samples;
	for(int SampleIndex=0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		float32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.f * Pi32 * 1.f / (float32)WavePeriod;
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
            uint8 Blue = (X + BlueOffset);
            uint8 Green = (Y + GreenOffset);
            uint8 Red = (Blue - Green);

            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void		
GameUpdateAndRender(game_input *Input,
					game_offscreen_buffer *ScreenBuffer, 
					game_sound_output_buffer *SoundBuffer)
{
	local_persist int BlueOffset = 0;
	local_persist int GreenOffset = 0;
	local_persist int ToneHz = 256;

	game_controller_input *Input0 = &Input->Controllers[0];
	if(Input0->IsAnalog)
	{
		BlueOffset += (int)4.f*(Input0->EndX);
		ToneHz = 256 + (int)(128.f*(Input0->EndY));
	}
	else
	{
	}

	if(Input0->Down.EndedDown)
	{
		GreenOffset += 1;
	}

	GameOutputSound(SoundBuffer, ToneHz);
	RenderWeirGradient(ScreenBuffer, BlueOffset, GreenOffset);
}