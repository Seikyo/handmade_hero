#ifndef GAME_WIN32
#define GAME_WIN32

struct win32_offscreen_buffer
{
    // NOTE: Pixels are always 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    uint32 RunningSampleIndex;
    int BytesPerSample;
    int SecondaryBufferSize;
    float32 tSine;
    int LatencySampleCount;
};

#endif