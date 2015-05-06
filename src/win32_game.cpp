#include <stdint.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;
typedef float float32;
typedef double float64;

#include "game.h"
#include "game.cpp"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_game.h"

// TODO: Globals for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// NOTE: DirectSound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *Filename)
{
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
                {
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
        }

        CloseHandle(FileHandle);
    }

    return Result;
}

internal void
DEBUGPlatformFreeFileMemory(void *Memory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

internal bool32 
DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory)
{
    bool32 Result = false;

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            Result = (BytesWritten == MemorySize);
        }
        else
        {

        }

        CloseHandle(FileHandle);
    }

    return Result;
}

internal void
Win32LoadXInput(void)
{
    // Try loading xinput1_4
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary) 
    { 
        // TODO: Diagnostic
        HMODULE XInputLibrary = LoadLibraryA("xinput9_0_1.dll"); 
    }
    if(!XInputLibrary) 
    { 
        // TODO: Diagnostic
        HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll"); 
    }

    // If xinput loaded, link XInputGetState and XInputSetState functions from dll
    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) { XInputGetState = XInputGetStateStub; }

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) { XInputSetState = XInputSetStateStub; }

        // TODO: Diagnostic
    }
    else
    {
        // TODO: Diagnostic
    }
}

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // NOTE: Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if(DSoundLibrary)
    {
        // NOTE: Get a DirectSound object - cooperative
         direct_sound_create *DirectSoundCreate = (direct_sound_create*)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // NOTE: Create a primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        // NOTE: We have finally set the format
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO: Diagnostic
                    }
                }
                else
                {
                    // TODO: Diagnostic
                }
            }
            else
            {
                // TODO: Diagnostic
            }
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            // NOTE: Create a secondary buffer
            if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("Secondary buffer format was set.\n");
            }

            // NOTE: Start it playing
        }
        else
        {
            // TODO: Diagnostic
        }
    }
    else
    {
        // TODO: Diagnostic
    }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO: BulletProof this
    // Maybe don't free first, free after, then free first if that fails

    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;

    // NOTE: When biHeight field is negative Windows treat bitmap as top-down
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = BytesPerPixel * Width * Height;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * BytesPerPixel;

    // TODO: Probably clear this to black
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext,
                           int WindowWidth, int WindowHeight)
{
    // TODO: Aspect ratio correction
    StretchDIBits(DeviceContext,
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK 
Win32MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_SIZE:
        {
        } break;

        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user
            GlobalRunning = false;
        } break;

        case WM_DESTROY:
        {
            // TODO: Handle this as an error - recreate window ?
			GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown =  ((LParam & (1 << 31)) == 0);
            if(WasDown != IsDown)
            {
                if(VKCode == 'W')
                {
                }
                else if(VKCode == 'A')
                {
                }
                else if(VKCode == 'S')
                {
                }
                else if(VKCode == 'D')
                {
                }
                else if(VKCode == 'Q')
                {
                }
                else if(VKCode == 'E')
                {
                }
                else if(VKCode == VK_UP)
                {
                }
                else if(VKCode == VK_LEFT)
                {
                }
                else if(VKCode == VK_DOWN)
                {
                }
                else if(VKCode == VK_RIGHT)
                {
                }
                else if(VKCode == VK_ESCAPE)
                {
                    OutputDebugStringA("ESCAPE: ");
                    if(IsDown)
                    {
                        OutputDebugStringA("IsDown");
                    }
                    if(WasDown)
                    {
                        OutputDebugStringA("WasDown");
                    }
                    OutputDebugStringA("\n");
                }
                else if(VKCode == VK_SPACE)
                {
                }
            }

            bool32 AltKeyWasDown = (LParam & (1 << 29));
            if((VKCode == VK_F4) && AltKeyWasDown)
            {
                GlobalRunning = false;
            }
            /*LParam & (1 << 30);*/
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

internal void
Win32ClearBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size,
                                             0)))
    {
        int8 *DestSample = (int8 *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        DestSample = (int8 *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
                     game_sound_output_buffer *SourceBuffer)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size,
                                             0)))
    {
        // TODO: assert that Region1Size / Region2Size is valid
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16 *DestSample = (int16 *)Region1;
        int16 *SourceSample = SourceBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestSample = (int16 *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32ProcessXInputDigitalButton(game_button_state *OldState, game_button_state *NewState, 
                                DWORD XInputButtonState, DWORD ButtonBit)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

int CALLBACK 
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    LARGE_INTEGER PerformanceCounterFrequencyResult;
    QueryPerformanceFrequency(&PerformanceCounterFrequencyResult);
    int64 PerformanceCounterFrequency = PerformanceCounterFrequencyResult.QuadPart;

    Win32LoadXInput();

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "GameWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Game",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0);

        if(Window)
        {
            // NOTE: Since we specified CS_OWNDC we can use one device context
            // forever cause we are not sharing it with anyone.
            HDC DeviceContext = GetDC(Window);

            // NOTE: Graphic test
            int XOffset = 0;
            int YOffset = 0;

            // NOTE: Sound test
            win32_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, 
                                                   MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#if GAME_RELEASE
            LPVOID BaseAddress = 0;
#else
            LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#endif
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes((uint64)4);

            uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

            if(Samples && GameMemory.PermanentStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter;
                QueryPerformanceCounter(&LastCounter);
    			uint64 LastCycleCount = __rdtsc();

                GlobalRunning = true;
                while(GlobalRunning)
                {

                    MSG Message;
                    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                    {
                        if(Message.message == WM_QUIT)
                        {
                            GlobalRunning = false;
                        }

                        TranslateMessage(&Message);
                        DispatchMessage(&Message);
                    }

                    // TODO: Should we poll this more frequently
                    int MaxControllerCount = XUSER_MAX_COUNT;
                    if(MaxControllerCount > ArrayCount(NewInput->Controllers))
                    {
                        MaxControllerCount = ArrayCount(NewInput->Controllers);
                    }

                    for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
                    {
                        game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
                        game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

                        XINPUT_STATE ControllerState;
                        if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                        {
                            // NOTE: This controller is plugged in
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool Downn = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

                            NewController->IsAnalog = true;
                            NewController->StartX = OldController->EndX;
                            NewController->StartY = OldController->EndY;

                            float32 X;
                            if(Pad->sThumbLX < 0)
                            {
                                X = (float32)Pad->sThumbLX / 32768.f;
                            }
                            else
                            {
                                X = (float32)Pad->sThumbLX / 32767.f;
                            }
                            NewController->MinX = NewController->MaxX = NewController->EndX = X;

                            float32 Y;
                            if(Pad->sThumbLY < 0)
                            {
                                Y = (float32)Pad->sThumbLY / 32768.f;
                            }
                            else
                            {
                                Y = (float32)Pad->sThumbLY / 32767.f;
                            }
                            NewController->MinY = NewController->MaxY = NewController->EndY = Y;

                            Win32ProcessXInputDigitalButton(&OldController->Down, 
                                                            &NewController->Down,
                                                            Pad->wButtons, XINPUT_GAMEPAD_A);
                            Win32ProcessXInputDigitalButton(&OldController->Right, 
                                                            &NewController->Right, 
                                                            Pad->wButtons, XINPUT_GAMEPAD_B);
                            Win32ProcessXInputDigitalButton(&OldController->Left,
                                                            &NewController->Left,
                                                            Pad->wButtons, XINPUT_GAMEPAD_X);
                            Win32ProcessXInputDigitalButton(&OldController->Up,
                                                            &NewController->Up,
                                                            Pad->wButtons, XINPUT_GAMEPAD_Y);
                            Win32ProcessXInputDigitalButton(&OldController->LeftShoulder,
                                                            &NewController->LeftShoulder,
                                                            Pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
                            Win32ProcessXInputDigitalButton(&OldController->RightShoudler,
                                                            &NewController->RightShoudler,
                                                            Pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        }
                        else
                        {
                            // NOTE: This controller is not plugged in
                        }
                    }

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;

                    DWORD ByteToLock;
                    DWORD TargetCursor;
                    DWORD BytesToWrite;
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    bool32 SoundIsValid = false;
                    if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                    {
                        ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % 
                                            SoundOutput.SecondaryBufferSize);
                        TargetCursor = ((PlayCursor + SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample) %
                                              SoundOutput.SecondaryBufferSize);

                        if(ByteToLock == TargetCursor)
                        {
                            BytesToWrite = 0;
                        }
                        else if(ByteToLock > TargetCursor)
                        {
                            BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                            BytesToWrite += TargetCursor;
                        }
                        else 
                        {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }

                        SoundIsValid = true;
                    }

                    // Sound Buffer
                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;

                    // Screen Buffer
                    game_offscreen_buffer ScreenBuffer = {};
                    ScreenBuffer.Memory = GlobalBackBuffer.Memory;
                    ScreenBuffer.Width = GlobalBackBuffer.Width;
                    ScreenBuffer.Height = GlobalBackBuffer.Height;
                    ScreenBuffer.Pitch = GlobalBackBuffer.Pitch;

                    GameUpdateAndRender(&GameMemory, NewInput, &ScreenBuffer, &SoundBuffer);

                    if(SoundIsValid)
                    {
                        Win32FillSoundBuffer(&SoundOutput, ByteToLock,  BytesToWrite, &SoundBuffer);
                    }

                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

                    // Timer
                    uint64 EndCycleCount = __rdtsc();

                    LARGE_INTEGER EndCounter;
                    QueryPerformanceCounter(&EndCounter);

                    uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    int32 MegaCyclesPerFrame = (int32)(CyclesElapsed / (1000*1000));

                    LastCycleCount = EndCycleCount;

                    int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                    int32 MsPerFrame = (int32)((1000 * CounterElapsed) / PerformanceCounterFrequency);
                    int32 FramePerSecond = 1000 / MsPerFrame;

                    LastCounter = EndCounter;
#if 0
                    char Buffer[256];
                    wsprintf(Buffer, "%dms/f, %df/s, %dmc/f\n", MsPerFrame, FramePerSecond, MegaCyclesPerFrame);
                    OutputDebugStringA(Buffer);
#endif
                }
            }
            else
            {
                // TODO: Log
            }

            VirtualFree((&GameMemory)->PermanentStorage, TotalSize, MEM_RELEASE);
        }
        else
        {
            // TODO: Log
        }
    }
    else
    {
        // TOOD: Log
	}

    return(0);
}