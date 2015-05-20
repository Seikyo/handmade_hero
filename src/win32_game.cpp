#include "game.h"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_game.h"

// TODO: Globals for now
global_variable bool GlobalRunning;
global_variable bool GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerformanceCounterFrequency;

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

debug_read_file_result
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

void
DEBUGPlatformFreeFileMemory(void *Memory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

bool32 
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

inline FILETIME
Win32GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime = {};
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = FindFirstFile(Filename, &FindData);
    if(FindHandle != INVALID_HANDLE_VALUE)
    {
        LastWriteTime = FindData.ftLastWriteTime; 
        FindClose(FindHandle);
    }

    return LastWriteTime;
}

internal win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName)
{
    win32_game_code Result = {};

    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
    CopyFile(SourceDLLName, TempDLLName, FALSE);
    Result.GameCodeDLL = LoadLibraryA(TempDLLName);

    if(Result.GameCodeDLL)
    {
        Result.UpdateAndRender = (game_update_and_render *)GetProcAddress
            (Result.GameCodeDLL,"GameUpdateAndRender");
        Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress
            (Result.GameCodeDLL,"GameGetSoundSamples");

        Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }

    if(!Result.IsValid)
    {
        Result.UpdateAndRender = GameUpdateAndRenderStub;
        Result.GetSoundSamples = GameGetSoundSamplesStub;
    }
    return Result;
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = GameUpdateAndRenderStub;
    GameCode->GetSoundSamples = GameGetSoundSamplesStub;
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
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
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
    Buffer->BytesPerPixel = BytesPerPixel;

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
            if(WParam == TRUE)
            {
                SetLayeredWindowAttributes(Window, RGB(0,0,0), 255, LWA_ALPHA);
            }
            else
            {
                SetLayeredWindowAttributes(Window, RGB(0,0,0), 64, LWA_ALPHA);
            }
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert("Wrong keyboard handler");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
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
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    Assert(NewState->EndedDown != IsDown);
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;
}

internal void
Win32ProcessXInputDigitalButton(game_button_state *OldState, game_button_state *NewState, 
                                DWORD XInputButtonState, DWORD ButtonBit)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal float32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    float32 Result = 0;

    if(Value < -DeadZoneThreshold)
    {
        Result = (float32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    }
    else if (Value > DeadZoneThreshold)
    {
        Result = (float32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }

    return Result;
}

internal void
Win32BeginRecordingInput(win32_state *Win32State, int InputRecordingIndex)
{
    Win32State->InputRecordingIndex = InputRecordingIndex;

    char *Filename = "game.sav";
    Win32State->RecordingHandle = 
        CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    // NOTE: Cannot write file over 4Gb in a single write
    DWORD BytesToWrite = (DWORD)Win32State->TotalSize;
    Assert(BytesToWrite < 0xFFFFFFFF);
    DWORD BytesWritten;
    WriteFile(Win32State->RecordingHandle, Win32State->GameMemoryBlock, BytesToWrite, &BytesWritten, 0);
}

internal void
Win32EndRecordingInput(win32_state *Win32State)
{
    CloseHandle(Win32State->RecordingHandle);
    Win32State->InputRecordingIndex = 0;
}

internal void
Win32BeginInputPlayBack(win32_state *Win32State, int InputPlayingIndex)
{
    Win32State->InputPlayingIndex = InputPlayingIndex;

    char *Filename = "game.sav";
    Win32State->PlayBackHandle = 
        CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    DWORD BytesToRead = (DWORD)Win32State->TotalSize;
    Assert(BytesToRead < 0xFFFFFFFF);
    DWORD BytesRead;
    ReadFile(Win32State->PlayBackHandle, Win32State->GameMemoryBlock, BytesToRead, &BytesRead, 0);
}

internal void
Win32EndInputPlayBack(win32_state *Win32State)
{
    CloseHandle(Win32State->PlayBackHandle);
    Win32State->InputPlayingIndex = 0;
}

internal void
Win32RecordInput(win32_state *Win32State, game_input *NewInput)
{
    DWORD BytesWritten;
    WriteFile(Win32State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlayBackInput(win32_state *Win32State, game_input *NewInput)
{
    DWORD BytesRead;
    if(ReadFile(Win32State->PlayBackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
    {
        if(BytesRead == 0)
        {
            // NOTE: We've hit the end of the stream, go back to the beginning
            int PlayingIndex = Win32State->InputPlayingIndex;
            Win32EndInputPlayBack(Win32State);
            Win32BeginInputPlayBack(Win32State, PlayingIndex);
        }
    }
}

internal void
Win32ProcessPendingMessages(win32_state *Win32State, game_controller_input *KeyboardController)
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 VKCode = (uint32)Message.wParam;
                bool WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool IsDown =  ((Message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown)
                {
                    if(VKCode == 'Z')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    }
                    else if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoudler, IsDown);
                    }
                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                    }
                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                    }
                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
                    else if(VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
#if INTERNAL
                    else if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
                    else if(VKCode == 'L')
                    {
                        if(IsDown)
                        {
                            if(Win32State->InputRecordingIndex == 0)
                            {
                                Win32BeginRecordingInput(Win32State, 1);
                            }
                            else
                            {
                                Win32EndRecordingInput(Win32State);
                                Win32BeginInputPlayBack(Win32State, 1);
                            }
                        }
                    }
#endif
                }
                bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
                if((VKCode == VK_F4) && AltKeyWasDown)
                {
                    GlobalRunning = false;
                }
            } break;
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline float32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    float32 Result = ((float32)(End.QuadPart - Start.QuadPart) / (float32)GlobalPerformanceCounterFrequency);
    return Result;
}

internal void
Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer, int X, int Top, int Bottom, uint32 Color)
{
    if(Top <= 0)
    {
        Top = 0;
    }
    if(Bottom >= BackBuffer->Height)
    {
        Bottom = BackBuffer->Height;
    }
    if( (X >= 0) && (X < BackBuffer->Width))
    {
        uint8 *Pixel = (uint8 *)BackBuffer->Memory + 
                       X * BackBuffer->BytesPerPixel + 
                       Top * BackBuffer->Pitch;
        for(int Y = Top; Y < Bottom; ++Y)
        {
            *(uint32 *)Pixel = Color;
            Pixel += BackBuffer->Pitch;
        }
    }
}

internal void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer,
                           win32_sound_output *SoundOutput,
                           float32 C, int PadX, int Top, int Bottom,
                           DWORD Value, uint32 Color)
{
    float32 XFloat32 = (C * (float32)Value);
    int X = PadX + (int)XFloat32;
    Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer,
                      int MarkerCount, win32_debug_time_marker *Markers,
                      int CurrentMarker,
                      win32_sound_output *SoundOutput, float32 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;

    int LineHeight = 64;

    float32 C = (float32)(BackBuffer->Width - 2 * PadX) / (float32)SoundOutput->SecondaryBufferSize;
    for(int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
    {
        win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0xFFFFFF00;
        DWORD PlayWindowColor = 0xFFFF00FF;

        int Top = PadY;
        int Bottom = PadY + LineHeight;
        if(MarkerIndex == CurrentMarker)
        {
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            int FirstTop = Top;

            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);

            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);

            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
        }
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480*SoundOutput->BytesPerSample, PlayWindowColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
    }
}

internal void 
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
    for(size_t Index = 0; Index < SourceACount; ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for(size_t Index = 0; Index < SourceBCount; ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

int CALLBACK 
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    char EXEFileName[MAX_PATH];
    DWORD SizeOfFileName = GetModuleFileNameA(0, EXEFileName, sizeof(EXEFileName));
    char *OnePastLastSlash = EXEFileName;
    for(char *Scan = EXEFileName; *Scan; ++Scan)
    {
        if(*Scan == '\\')
        {
            OnePastLastSlash = Scan + 1;
        }
    }
    char SourceGameCodeDLLFilename[] = "game.dll";
    char SourceGameCodeDLLFullPath[MAX_PATH];
    CatStrings(OnePastLastSlash - EXEFileName, EXEFileName,
               sizeof(SourceGameCodeDLLFilename) - 1, SourceGameCodeDLLFilename,
               sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFilename[] = "game_temp.dll";
    char TempGameCodeDLLFullPath[MAX_PATH];
    CatStrings(OnePastLastSlash - EXEFileName, EXEFileName,
               sizeof(TempGameCodeDLLFilename) - 1, TempGameCodeDLLFilename,
               sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

    LARGE_INTEGER PerformanceCounterFrequencyResult;
    QueryPerformanceFrequency(&PerformanceCounterFrequencyResult);
    GlobalPerformanceCounterFrequency = PerformanceCounterFrequencyResult.QuadPart;

    // Set Windows scheduler to 1 ms
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    Win32LoadXInput();

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "GameWindowClass";

#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz / 2)
    
    float32 TargetSecondsPerFrame = 1.f / (float32)GameUpdateHz;

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            WS_EX_TOPMOST|WS_EX_LAYERED,
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
            // TODO: Get rid of LantencySampleCount
            SoundOutput.LatencySampleCount = 3 * (SoundOutput.SamplesPerSecond / GameUpdateHz);
            SoundOutput.SafetyBytes = ((SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameUpdateHz) / 3;

            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, 
                                                   MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#if INTERNAL
            LPVOID BaseAddress = 0;
#else
            LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#endif

            win32_state Win32State = {};

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes(1);
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;

            Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

            if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();

                int DebugTimeMarkerIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[GameUpdateHz/2] = {0};

                DWORD AudioLatencyBytes = 0;
                DWORD AudioLatencySeconds = 0;
                bool32 SoundIsValid = false;

                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                uint32 LoadCounter = 120;

                GlobalRunning = true;
                uint64 LastCycleCount = __rdtsc();
                while(GlobalRunning)
                {
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                        LoadCounter = 0;
                    }

                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    game_controller_input ZeroController = {};
                    *NewKeyboardController = ZeroController;
                    NewKeyboardController->IsConnected = true;
                    for(int ButtonIndex = 0; 
                        ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                        ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

                    if(!GlobalPause)
                    {
                        // TODO: Should we poll this more frequently
                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                        {
                            MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                        }

                        for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
                        {
                            DWORD OurControllerIndex = ControllerIndex + 1;
                            game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                            game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

                            XINPUT_STATE ControllerState;
                            if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                            {
                                NewController->IsConnected = true;
                                XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                                NewController->IsAnalog = true;
                                NewController->StickAverageX = Win32ProcessXInputStickValue(
                                    Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                NewController->StickAverageY = Win32ProcessXInputStickValue(
                                    Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                                if(NewController->StickAverageX != 0.0f ||
                                   NewController->StickAverageY != 0.0f)
                                {
                                    NewController->IsAnalog = true;
                                }

                                if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                                {
                                    NewController->StickAverageY = 1.f;
                                    NewController->IsAnalog = false;
                                }
                                if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                                {
                                    NewController->StickAverageY = -1.f;
                                    NewController->IsAnalog = false;
                                }
                                if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                                {
                                    NewController->StickAverageX = -1.f;
                                    NewController->IsAnalog = false;
                                }

                                if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                                {
                                    NewController->StickAverageX = 1.f;
                                    NewController->IsAnalog = false;
                                }

                                float32 Theshold = 0.5f;
                                Win32ProcessXInputDigitalButton(&OldController->MoveLeft,
                                                                &NewController->MoveLeft,
                                                                (NewController->StickAverageX < -Theshold)? 1: 0, 1);
                                Win32ProcessXInputDigitalButton(&OldController->MoveRight,
                                                                &NewController->MoveRight,
                                                                (NewController->StickAverageX > Theshold)? 1: 0, 1);
                                Win32ProcessXInputDigitalButton(&OldController->MoveDown,
                                                                &NewController->MoveDown,
                                                                (NewController->StickAverageY < -Theshold)? 1: 0, 1);
                                Win32ProcessXInputDigitalButton(&OldController->MoveUp,
                                                                &NewController->MoveUp,
                                                                (NewController->StickAverageY > Theshold)? 1: 0, 1);

                                Win32ProcessXInputDigitalButton(&OldController->ActionDown, 
                                                                &NewController->ActionDown,
                                                                Pad->wButtons, XINPUT_GAMEPAD_A);
                                Win32ProcessXInputDigitalButton(&OldController->ActionRight, 
                                                                &NewController->ActionRight, 
                                                                Pad->wButtons, XINPUT_GAMEPAD_B);
                                Win32ProcessXInputDigitalButton(&OldController->ActionLeft,
                                                                &NewController->ActionLeft,
                                                                Pad->wButtons, XINPUT_GAMEPAD_X);
                                Win32ProcessXInputDigitalButton(&OldController->ActionUp,
                                                                &NewController->ActionUp,
                                                                Pad->wButtons, XINPUT_GAMEPAD_Y);
                                Win32ProcessXInputDigitalButton(&OldController->LeftShoulder,
                                                                &NewController->LeftShoulder,
                                                                Pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
                                Win32ProcessXInputDigitalButton(&OldController->RightShoudler,
                                                                &NewController->RightShoudler,
                                                                Pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);

                                Win32ProcessXInputDigitalButton(&OldController->Start,
                                                                &NewController->Start,
                                                                Pad->wButtons, XINPUT_GAMEPAD_START);
                                Win32ProcessXInputDigitalButton(&OldController->Back,
                                                                &NewController->Back,
                                                                Pad->wButtons, XINPUT_GAMEPAD_BACK);
                            }
                            else
                            {
                                NewController->IsConnected = false;
                            }
                        }

                        game_offscreen_buffer ScreenBuffer = {};
                        ScreenBuffer.Memory = GlobalBackBuffer.Memory;
                        ScreenBuffer.Width = GlobalBackBuffer.Width;
                        ScreenBuffer.Height = GlobalBackBuffer.Height;
                        ScreenBuffer.Pitch = GlobalBackBuffer.Pitch;
                        ScreenBuffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;

                        if(Win32State.InputRecordingIndex)
                        {
                            Win32RecordInput(&Win32State, NewInput);
                        }

                        if(Win32State.InputPlayingIndex)
                        {
                            Win32PlayBackInput(&Win32State, NewInput);
                        }

                        Game.UpdateAndRender(&GameMemory, NewInput, &ScreenBuffer);

                        LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                        float32 FromBeginToAudioSeconds = 1000.f * Win32GetSecondsElapsed(LastCounter, AudioWallClock);

                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                        {
                            if(!SoundIsValid)
                            {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true;   
                            }

                            DWORD ByteToLock = 
                                ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);

                            DWORD ExpectedSoundBytesPerFrame = 
                                ((SoundOutput.SamplesPerSecond  * SoundOutput.BytesPerSample) / GameUpdateHz);
                            float32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame)*(float32)ExpectedSoundBytesPerFrame);


                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;

                            DWORD SafeWriteCursor = WriteCursor;
                            if(SafeWriteCursor < PlayCursor)
                            {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;

                            bool32 AudioCardIsLowLatency = SafeWriteCursor < ExpectedFrameBoundaryByte;

                            DWORD TargetCursor = 0;
                            if(AudioCardIsLowLatency)
                            {
                                TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
                            }
                            else 
                            {
                                TargetCursor = WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes;
                            }
                            TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

                            DWORD BytesToWrite = 0;
                            if(ByteToLock > TargetCursor)
                            {
                                BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                                BytesToWrite += TargetCursor;
                            }
                            else 
                            {
                                BytesToWrite = TargetCursor - ByteToLock;
                            }

                            // Sound Buffer
                            game_sound_output_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;
                            Game.GetSoundSamples(&GameMemory, &SoundBuffer);
#if INTERNAL
                            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = ByteToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if(UnwrappedWriteCursor < PlayCursor)
                            {
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            DWORD AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;

                            char TextBuffer[256] = {0};
                            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                        "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u\n",
                                        ByteToLock, TargetCursor, BytesToWrite,
                                        PlayCursor, WriteCursor, AudioLatencyBytes);
                            OutputDebugStringA(TextBuffer);
#endif
                            Win32FillSoundBuffer(&SoundOutput, ByteToLock,  BytesToWrite, &SoundBuffer);
                        }
                        else
                        {
                            SoundIsValid = false;
                        }

                        // Timer
                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        float32 SecondsElapsedForWork = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        float32 SecondsElapsedForFrame = SecondsElapsedForWork;
                        if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            if(SleepIsGranular)
                            {
                                DWORD SleepMS = (DWORD)(1000.f * (TargetSecondsPerFrame - SecondsElapsedForFrame) - 1);
                                if(SleepMS > 0)
                                {
                                    Sleep(SleepMS);
                                }
                            }

                            float32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());

                            if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                // TODO: Log missed sleep
                            }

                            while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            }
                        }
                        else
                        {
                            // TODO: Log missed frame rate
                        }

                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        float32 MsPerFrame = 1000.f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter = EndCounter;

                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
#if INTERNAL
                        // TODO: Wrong for the first time for DebugTimeMarkerIndex - 1
                        Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers,
                                              DebugTimeMarkerIndex - 1,
                                              &SoundOutput, TargetSecondsPerFrame);
#endif
                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);

                        FlipWallClock = Win32GetWallClock();
#if INTERNAL
                        {
                            // NOTE: Debug code
                            DWORD PlayCursor;
                            DWORD WriteCursor;
                            if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                            {
                                Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
                                win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                                Marker->FlipPlayCursor = PlayCursor;
                                Marker->FlipWriteCursor = WriteCursor;
                            }
                        }
#endif

                        game_input *Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;

                        uint64 EndCycleCount = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        float64 FramePerSecond = 0.f;
                        float64 MegaCyclesPerFrame = (int32)(CyclesElapsed / (1000*1000));
                        
                        //Display frame speed
                        char Buffer[256];
                        _snprintf_s(Buffer, sizeof(Buffer), 
                                    "%.02fms/f, %.02ff/s, %.02fmc/f\n", 
                                    MsPerFrame, FramePerSecond, MegaCyclesPerFrame);
                        OutputDebugStringA(Buffer);
#if INTERNAL
                        ++DebugTimeMarkerIndex;
                        if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                        {
                            DebugTimeMarkerIndex = 0;
                        }
#endif
                    }
                }
            }
            else
            {
                // TODO: Log
            }
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