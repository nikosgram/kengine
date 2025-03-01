#include "win32_kengine.h"
#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"
#include "win32_kengine_shared.c"

#if KENGINE_INTERNAL
global debug_event_table GlobalDebugEventTable_;
debug_event_table *GlobalDebugEventTable = &GlobalDebugEventTable_;
#else
global platform_api *Platform;
#endif

global GLuint GlobalBlitTextureHandle;
global wgl_create_context_attribs_arb *wglCreateContextAttribsARB;
global wgl_choose_pixel_format_arb *wglChoosePixelFormatARB;
global wgl_swap_interval_ext *wglSwapIntervalEXT;
global wgl_get_extensions_string_ext *wglGetExtensionsStringEXT;
global b32 OpenGLSupportsSRGBFramebuffer;
global GLuint OpenGLDefaultInternalTextureFormat;
global b32 HardwareRendering = false;

#if KENGINE_INTERNAL
#include "kengine_sort.c"
#else
#include "kengine.h"
#include "kengine.c"
#endif
#include "kengine_renderer_software.c"
#include "kengine_renderer.c"
#include "win32_kengine_opengl.c"

internal LRESULT
Win32MainWindowCallback_(win32_state *Win32State, HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CLOSE:
        {
            Win32State->IsRunning = false;
        } break;
        
        case WM_SIZE:
        {
            RECT ClientRect;
            if(Win32GetClientRect(Window, &ClientRect))
            {
                win32_offscreen_buffer *Backbuffer = &Win32State->Backbuffer;
                Backbuffer->Width = ClientRect.right - ClientRect.left;
                Backbuffer->Height = ClientRect.bottom - ClientRect.top;
                
                if(Backbuffer->Memory)
                {
                    Win32VirtualFree(Backbuffer->Memory, 0, MEM_RELEASE);
                }
                
                Backbuffer->Info.bmiHeader.biSize = sizeof(Backbuffer->Info.bmiHeader);
                Backbuffer->Info.bmiHeader.biWidth = Backbuffer->Width;
                Backbuffer->Info.bmiHeader.biHeight = Backbuffer->Height;
                Backbuffer->Info.bmiHeader.biPlanes = 1;
                Backbuffer->Info.bmiHeader.biBitCount = 32;
                Backbuffer->Info.bmiHeader.biCompression = BI_RGB;
                
                // TODO(kstandbridge): Need to figure out a way we can align this
                //Backbuffer->Pitch = Align16(Backbuffer->Width*BITMAP_BYTES_PER_PIXEL);
                Backbuffer->Pitch = Backbuffer->Width*BITMAP_BYTES_PER_PIXEL;
                
                Backbuffer->Memory = Win32VirtualAlloc(0, sizeof(u32)*Backbuffer->Pitch*Backbuffer->Height,
                                                       MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                
            }
            else
            {
                // TODO(kstandbridge): Error failed to get client rect?
                InvalidCodePath;
            }
            
        } break;
        
        default:
        {
            Result = Win32DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal LRESULT __stdcall
Win32MainWindowCallback(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    win32_state *Win32State = 0;
    
    if(Message == WM_NCCREATE)
    {
        CREATESTRUCTA *CreateStruct = (CREATESTRUCTA *)LParam;
        Win32State = (win32_state *)CreateStruct->lpCreateParams;
        Win32SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)Win32State);
    }
    else
    {
        Win32State = (win32_state *)Win32GetWindowLongPtrA(Window, GWLP_USERDATA);
    }
    
    if(Win32State)
    {        
        Result = Win32MainWindowCallback_(Win32State, Window, Message, WParam, LParam);
    }
    else
    {
        Result = Win32DefWindowProcA(Window, Message, WParam, LParam);
    }
    
    return Result;
    
}

internal void
Win32ProcessPendingMessages(win32_state *Win32State, app_input *Input)
{
    Win32State;
    
    MSG Message;
    while(Win32PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                __debugbreak();
            } break;
            
            case WM_MOUSEWHEEL:
            {
                Input->MouseZ = GET_WHEEL_DELTA_WPARAM(Message.wParam);
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)Message.wParam;
                
                b32 AltKeyWasDown = (Message.lParam & (1 << 29));
                b32 ShiftKeyWasDown = (Win32GetKeyState(VK_SHIFT) & (1 << 15));
                
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1UL << 31)) == 0);
                if(WasDown != IsDown)
                {
                    // TODO(kstandbridge): Process individual keys?
                }
                
                if(IsDown)
                {
                    if((VKCode == VK_F4) && AltKeyWasDown)
                    {
                        Win32State->IsRunning = false;
                    }
                    else if((VKCode >= VK_F1) && (VKCode <= VK_F12))
                    {
                        Input->FKeyPressed[VKCode - VK_F1 + 1] = true;
                    }
                }
                
            } break;
            
            default:
            {
                Win32TranslateMessage(&Message);
                Win32DispatchMessageA(&Message);
            } break;
        }
    }
}



inline void
AppendCString(char *StartAt, char *Text)
{
    while(*Text)
    {
        *StartAt++ = *Text++;
    }
}

internal void
Win32ParseCommandLingArgs(win32_state *Win32State)
{
    char *CommandLingArgs = Win32GetCommandLineA();
    Assert(CommandLingArgs);
    
    char *At = CommandLingArgs;
    char *ParamStart = At;
    u32 ParamLength = 0;
    b32 Parsing = true;
    b32 ExeNameFound = false;
    while(Parsing)
    {
        if((*At == '\0') || IsWhitespace(*At))
        {
            if(*At != '\0')
            {
                while(IsWhitespace(*At))
                {
                    ++At;
                }
            }
            else
            {
                Parsing = false;
            }
            
            if(ExeNameFound)
            {            
                string Parameter;
                Parameter.Size = ParamLength;
                Parameter.Data = (u8 *)ParamStart;
                Parameter;
                // TODO(kstandbridge): Handle parameter
            }
            else
            {
                ExeNameFound = true;
                
#if KENGINE_INTERNAL
                if(*ParamStart == '\"')
                {
                    ++ParamStart;
                    ParamLength -= 2;
                }
                char *LastSlash = At - 1;
                while(*LastSlash != '\\')
                {
                    --ParamLength;
                    --LastSlash;
                }
                ++ParamLength;
                Copy(ParamLength, ParamStart, Win32State->ExeFilePath);
                Win32State->ExeFilePath[ParamLength] = '\0';
                
                Copy(ParamLength, Win32State->ExeFilePath, Win32State->DllFullFilePath);
                AppendCString(Win32State->DllFullFilePath + ParamLength, "\\kengine.dll");
                
                Copy(ParamLength, Win32State->ExeFilePath, Win32State->TempDllFullFilePath);
                AppendCString(Win32State->TempDllFullFilePath + ParamLength, "\\kengine_temp.dll");
                
                Copy(ParamLength, Win32State->ExeFilePath, Win32State->LockFullFilePath);
                AppendCString(Win32State->LockFullFilePath + ParamLength, "\\lock.tmp");
#endif
                
            }
            
            ParamStart = At;
            ParamLength = 1;
            ++At;
        }
        else
        {
            ++ParamLength;
            ++At;
        }
        
    }
}

#define MAX_GLYPH_COUNT 5000
global KERNINGPAIR *GlobalKerningPairs;
global u32 GlobalKerningPairCount;
global TEXTMETRICA GlobalTextMetric;

global HDC FontDeviceContext;
internal loaded_glyph
Win32GetGlyphForCodePoint(memory_arena *Arena, u32 CodePoint)
{
    
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024
    
    local_persist b32 FontInitialized;
    local_persist void *FontBits;
    
    local_persist HFONT FontHandle;
    
    local_persist u32 CurrentGlyphIndex = 0;
    if(!FontInitialized)
    {
        FontDeviceContext = Win32CreateCompatibleDC(Win32GetDC(0));
        
        BITMAPINFO Info;
        ZeroStruct(Info);
        Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
        Info.bmiHeader.biWidth = MAX_FONT_WIDTH;
        Info.bmiHeader.biHeight = MAX_FONT_HEIGHT;
        Info.bmiHeader.biPlanes = 1;
        Info.bmiHeader.biBitCount = 32;
        Info.bmiHeader.biCompression = BI_RGB;
        Info.bmiHeader.biSizeImage = 0;
        Info.bmiHeader.biXPelsPerMeter = 0;
        Info.bmiHeader.biYPelsPerMeter = 0;
        Info.bmiHeader.biClrUsed = 0;
        Info.bmiHeader.biClrImportant = 0;
        HBITMAP Bitmap = Win32CreateDIBSection(FontDeviceContext, &Info, DIB_RGB_COLORS, &FontBits, 0, 0);
        Win32SelectObject(FontDeviceContext, Bitmap);
        Win32SetBkColor(FontDeviceContext, RGB(0, 0, 0));
        
#if 1
        Win32AddFontResourceExA("c:/Windows/Fonts/segoeui.ttf", FR_PRIVATE, 0);
        s32 PointSize = 11;
        s32 FontHeight = -Win32MulDiv(PointSize, Win32GetDeviceCaps(FontDeviceContext, LOGPIXELSY), 72);
        FontHandle = Win32CreateFontA(FontHeight, 0, 0, 0,
                                      FW_NORMAL, // NOTE(kstandbridge): Weight
                                      FALSE, // NOTE(kstandbridge): Italic
                                      FALSE, // NOTE(kstandbridge): Underline
                                      FALSE, // NOTE(kstandbridge): Strikeout
                                      DEFAULT_CHARSET, 
                                      OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, 
                                      ANTIALIASED_QUALITY,
                                      DEFAULT_PITCH|FF_DONTCARE,
                                      "Segoe UI");
#else
        Win32AddFontResourceExA("c:/Windows/Fonts/LiberationMono-Regular.ttf", FR_PRIVATE, 0);
        FontHandle = Win32CreateFontA(20, 0, 0, 0,
                                      FW_NORMAL, // NOTE(kstandbridge): Weight
                                      FALSE, // NOTE(kstandbridge): Italic
                                      FALSE, // NOTE(kstandbridge): Underline
                                      FALSE, // NOTE(kstandbridge): Strikeout
                                      DEFAULT_CHARSET, 
                                      OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, 
                                      ANTIALIASED_QUALITY,
                                      DEFAULT_PITCH|FF_DONTCARE,
                                      "Liberation Mono");
#endif
        
        Win32SelectObject(FontDeviceContext, FontHandle);
        Win32GetTextMetricsA(FontDeviceContext, &GlobalTextMetric);
        
        GlobalKerningPairCount = Win32GetKerningPairsW(FontDeviceContext, 0, 0);
        GlobalKerningPairs = PushArray(Arena, GlobalKerningPairCount, KERNINGPAIR);
        Win32GetKerningPairsW(FontDeviceContext, GlobalKerningPairCount, GlobalKerningPairs);
        
        FontInitialized = true;
    }
    
    loaded_glyph Result;
    ZeroStruct(Result);
    
    Win32SelectObject(FontDeviceContext, FontHandle);
    
    ZeroSize(MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32), FontBits);
    
    wchar_t CheesePoint = (wchar_t)CodePoint;
    
    SIZE Size;
    Win32GetTextExtentPoint32W(FontDeviceContext, &CheesePoint, 1, &Size);
    
    s32 PreStepX = 128;
    
    s32 BoundWidth = Size.cx + 2*PreStepX;
    if(BoundWidth > MAX_FONT_WIDTH)
    {
        BoundWidth = MAX_FONT_WIDTH;
    }
    s32 BoundHeight = Size.cy;
    if(BoundHeight > MAX_FONT_HEIGHT)
    {
        BoundHeight = MAX_FONT_HEIGHT;
    }
    
    //Win32SetBkMode(FontDeviceContext, TRANSPARENT);
    Win32SetTextColor(FontDeviceContext, RGB(255, 255, 255));
    Win32TextOutW(FontDeviceContext, PreStepX, 0, &CheesePoint, 1);
    
    s32 MinX = 10000;
    s32 MinY = 10000;
    s32 MaxX = -10000;
    s32 MaxY = -10000;
    
    u32 *Row = (u32 *)FontBits + (MAX_FONT_HEIGHT - 1)*MAX_FONT_WIDTH;
    for(s32 Y = 0;
        Y < BoundHeight;
        ++Y)
    {
        u32 *Pixel = Row;
        for(s32 X = 0;
            X < BoundWidth;
            ++X)
        {
            
#if KENGINE_SLOW
            COLORREF RefPixel = Win32GetPixel(FontDeviceContext, X, Y);
            Assert(RefPixel == *Pixel);
#endif
            if(*Pixel != 0)
            {
                if(MinX > X)
                {
                    MinX = X;                    
                }
                
                if(MinY > Y)
                {
                    MinY = Y;                    
                }
                
                if(MaxX < X)
                {
                    MaxX = X;                    
                }
                
                if(MaxY < Y)
                {
                    MaxY = Y;                    
                }
            }
            
            ++Pixel;
        }
        Row -= MAX_FONT_WIDTH;
    }
    
    if(MinX <= MaxX)
    {
        s32 Width = (MaxX - MinX) + 1;
        s32 Height = (MaxY - MinY) + 1;
        
        Result.Bitmap.Width = Width + 2;
        Result.Bitmap.Height = Height + 2;
        Result.Bitmap.WidthOverHeight = SafeRatio1((f32)Result.Bitmap.Width, (f32)Result.Bitmap.Height);
        Result.Bitmap.Pitch = Result.Bitmap.Width*BITMAP_BYTES_PER_PIXEL;
        Result.Bitmap.Memory = PushSize(Arena, Result.Bitmap.Height*Result.Bitmap.Pitch);
        
        ZeroSize(Result.Bitmap.Height*Result.Bitmap.Pitch, Result.Bitmap.Memory);
        
        u8 *DestRow = (u8 *)Result.Bitmap.Memory + (Result.Bitmap.Height - 1 - 1)*Result.Bitmap.Pitch;
        u32 *SourceRow = (u32 *)FontBits + (MAX_FONT_HEIGHT - 1 - MinY)*MAX_FONT_WIDTH;
        for(s32 Y = MinY;
            Y <= MaxY;
            ++Y)
        {
            u32 *Source = (u32 *)SourceRow + MinX;
            u32 *Dest = (u32 *)DestRow + 1;
            for(s32 X = MinX;
                X <= MaxX;
                ++X)
            {
                
#if KENGINE_SLOW
                COLORREF Pixel = Win32GetPixel(FontDeviceContext, X, Y);
                Assert(Pixel == *Source);
#else
                u32 Pixel = *Source;
#endif
                
                f32 Gray = (f32)(Pixel & 0xFF);
                v4 Texel = V4(255.0f, 255.0f, 255.0f, Gray);
                Texel = SRGB255ToLinear1(Texel);
                Texel.R *= Texel.A;
                Texel.G *= Texel.A;
                Texel.B *= Texel.A;
                Texel = Linear1ToSRGB255(Texel);
                
                *Dest++ = (((u32)(Texel.A + 0.5f) << 24) |
                           ((u32)(Texel.R + 0.5f) << 16) |
                           ((u32)(Texel.G + 0.5f) << 8) |
                           ((u32)(Texel.B + 0.5f) << 0));
                
                ++Source;
            }
            
            DestRow -= Result.Bitmap.Pitch;
            SourceRow -= MAX_FONT_WIDTH;
        }
        
        Result.Bitmap.AlignPercentage.X = (1.0f) / (f32)Result.Bitmap.Width;
        Result.Bitmap.AlignPercentage.Y = (1.0f + (MaxY - (BoundHeight - GlobalTextMetric.tmDescent))) / (f32)Result.Bitmap.Height;
        
        Result.KerningChange = (f32)(MinX - PreStepX);
        
    }
    
    return Result;
}

internal f32
Win32GetHorizontalAdvance(u32 PrevCodePoint, u32 CodePoint)
{
    s32 ThisWidth;
    Win32GetCharWidth32W(FontDeviceContext, PrevCodePoint, CodePoint, &ThisWidth);
    f32 Result = (f32)ThisWidth; 
    
    for(DWORD KerningPairIndex = 0;
        KerningPairIndex < GlobalKerningPairCount;
        ++KerningPairIndex)
    {
        KERNINGPAIR *Pair = GlobalKerningPairs + KerningPairIndex;
        if((Pair->wFirst == PrevCodePoint) &&
           (Pair->wSecond == CodePoint))
        {
            Result += Pair->iKernAmount;
            break;
        }
    }
    
    return Result;
}

internal f32
Win32GetVerticleAdvance()
{
    f32 Result = (f32)GlobalTextMetric.tmAscent + (f32)GlobalTextMetric.tmDescent + (f32)GlobalTextMetric.tmDescent;
    
    return Result;
}

inline void
ProcessInputMessage(app_button_state *NewState, b32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

#if KENGINE_INTERNAL
internal void
Win32UnloadAppCode(win32_state *Win32State)
{
    if(Win32State->AppLibrary && !Win32FreeLibrary(Win32State->AppLibrary))
    {
        // TODO(kstandbridge): Error freeing app library
        InvalidCodePath;
    }
    Win32State->AppLibrary = 0;
    Win32State->AppUpdateFrame = 0;
}

internal void
Win32LoadAppCode(platform_api *Platform, win32_state *Win32State, FILETIME NewDLLWriteTime)
{
    if(Win32CopyFileA(Win32State->DllFullFilePath, Win32State->TempDllFullFilePath, false))
    {
        Win32State->AppLibrary = Win32LoadLibraryA(Win32State->TempDllFullFilePath);
        if(Win32State->AppLibrary)
        {
            Win32State->AppUpdateFrame = (app_update_frame *)Win32GetProcAddressA(Win32State->AppLibrary, "AppUpdateFrame");
            Assert(Win32State->AppUpdateFrame);
            Win32State->DebugUpdateFrame = (debug_update_frame *)Win32GetProcAddressA(Win32State->AppLibrary, "DebugUpdateFrame");
            Assert(Win32State->DebugUpdateFrame);
            
            if(!Win32State->AppUpdateFrame || !Win32State->DebugUpdateFrame)
            {
                Win32UnloadAppCode(Win32State);
            }
            
            Win32State->LastDLLWriteTime = NewDLLWriteTime;
            Platform->DllReloaded = true;
        }
    }
    else
    {
        // TODO(kstandbridge): Error copying temp dll
        InvalidCodePath;
    }
}
#endif

void __stdcall 
WinMainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
    HINSTANCE Instance = Win32GetModuleHandleA(0);
    
    WNDCLASSEXA WindowClass;
    ZeroStruct(WindowClass);
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "KengineWindowClass";
    
    win32_state Win32State_;
    ZeroStruct(Win32State_);
    win32_state *Win32State = &Win32State_;
    
    LARGE_INTEGER PerfCountFrequencyResult;
    Win32QueryPerformanceFrequency(&PerfCountFrequencyResult);
    Win32State->PerfCountFrequency = (s64)PerfCountFrequencyResult.QuadPart;
    
    
    
    
    
    platform_api Platform_;
    ZeroStruct(&Platform_);
#if KENGINE_INTERNAL
    platform_api *Platform = &Platform_;
    void *BaseAddress = (void *)Terabytes(2);
#else
    Platform = &Platform_;
    void *BaseAddress = 0;
#endif
    
    SYSTEM_INFO SystemInfo;
    Win32GetSystemInfo(&SystemInfo);
    u32 ProcessorCount = SystemInfo.dwNumberOfProcessors;
    
    platform_work_queue PerFrameWorkQueue;
    u32 PerFrameThreadCount = RoundF32ToU32((f32)ProcessorCount*1.5f);
    Win32MakeQueue(&PerFrameWorkQueue, PerFrameThreadCount);
    Platform->PerFrameWorkQueue = &PerFrameWorkQueue;
    
    platform_work_queue BackgroundWorkQueue;
    u32 BackgroundThreadCount = RoundF32ToU32((f32)ProcessorCount/2.0f);
    Win32MakeQueue(&BackgroundWorkQueue, BackgroundThreadCount);
    Platform->BackgroundWorkQueue = &BackgroundWorkQueue;
    
    Platform->AddWorkEntry = Win32AddWorkEntry;
    Platform->CompleteAllWork = Win32CompleteAllWork;
    Platform->GetGlyphForCodePoint = Win32GetGlyphForCodePoint;
    Platform->GetHorizontalAdvance = Win32GetHorizontalAdvance;
    Platform->GetVerticleAdvance = Win32GetVerticleAdvance;;
    
#if KENGINE_INTERNAL
    Platform->DebugEventTable = GlobalDebugEventTable;
#endif
    
    u64 StorageSize = Megabytes(128);
    void *Storage = Win32VirtualAlloc(BaseAddress, StorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Storage);
    InitializeArena(&Win32State->Arena, StorageSize, Storage);
    
    Win32ParseCommandLingArgs(Win32State);
    
    if(Win32RegisterClassExA(&WindowClass))
    {
        Win32State->Window = Win32CreateWindowExA(0,
                                                  WindowClass.lpszClassName,
                                                  "kengine",
                                                  WS_OVERLAPPEDWINDOW,
                                                  CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                                  0, 0, Instance, Win32State);
        if(Win32State->Window)
        {
            Win32ShowWindow(Win32State->Window, SW_SHOW);
            HDC OpenGLDC = Win32GetDC(Win32State->Window);
            HGLRC OpenGLRC = Win32InitOpenGL(OpenGLDC);
            
            u32 PushBufferSize = Megabytes(64);
            void *PushBuffer = Win32VirtualAlloc(0, PushBufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            u32 CurrentSortMemorySize = Kilobytes(64);
            void *SortMemory = Win32VirtualAlloc(0, CurrentSortMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            u32 CurrentClipMemorySize = Kilobytes(64);
            void *ClipMemory = Win32VirtualAlloc(0, CurrentClipMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            LARGE_INTEGER LastCounter = Win32GetWallClock();
            
            app_input Input[2];
            ZeroArray(ArrayCount(Input), Input);
            app_input *NewInput = &Input[0];
            app_input *OldInput = &Input[1];
            
            s32 MonitorRefreshHz = 60;
            s32 Win32RefreshRate = Win32GetDeviceCaps(OpenGLDC, VREFRESH);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            
            // NOTE(kstandbridge): Otherwise Sleep will be ignored for requests less than 50? citation needed
            UINT MinSleepPeriod = 1;
            Win32timeBeginPeriod(MinSleepPeriod);
            
            u32 ExpectedFramesPerUpdate = 1;
            f32 TargetSecondsPerFrame = (f32)ExpectedFramesPerUpdate / MonitorRefreshHz;
            
            Win32State->IsRunning = true;
            while(Win32State->IsRunning)
            {
                
#if KENGINE_INTERNAL
                Platform->DllReloaded = false;
                b32 DllNeedsToBeReloaded = false;
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(Win32State->DllFullFilePath);
                if(Win32CompareFileTime(&NewDLLWriteTime, &Win32State->LastDLLWriteTime) != 0)
                {
                    WIN32_FILE_ATTRIBUTE_DATA Ignored;
                    if(!Win32GetFileAttributesExA(Win32State->LockFullFilePath, GetFileExInfoStandard, &Ignored))
                    {
                        DllNeedsToBeReloaded = true;
                    }
                }
#endif
                
                BEGIN_BLOCK("ProcessPendingMessages");
                NewInput->dtForFrame = TargetSecondsPerFrame;
                NewInput->MouseZ = 0;
                ZeroStruct(NewInput->FKeyPressed);
                Win32ProcessPendingMessages(Win32State, NewInput);
                END_BLOCK();
                
                BEGIN_BLOCK("ProcessMouseInput");
                POINT MouseP;
                Win32GetCursorPos(&MouseP);
                Win32ScreenToClient(Win32State->Window, &MouseP);
                NewInput->MouseX = (f32)MouseP.x;
                NewInput->MouseY = (f32)((Win32State->Backbuffer.Height - 1) - MouseP.y);
                
                NewInput->ShiftDown = (Win32GetKeyState(VK_SHIFT) & (1 << 15));
                NewInput->AltDown = (Win32GetKeyState(VK_MENU) & (1 << 15));
                NewInput->ControlDown = (Win32GetKeyState(VK_CONTROL) & (1 << 15));
                
                // NOTE(kstandbridge): The order of these needs to match the order on enum app_input_mouse_button_type
                DWORD ButtonVKs[MouseButton_Count] =
                {
                    VK_LBUTTON,
                    VK_MBUTTON,
                    VK_RBUTTON,
                    VK_XBUTTON1,
                    VK_XBUTTON2,
                };
                
                for(u32 ButtonIndex = 0;
                    ButtonIndex < MouseButton_Count;
                    ++ButtonIndex)
                {
                    NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
                    NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
                    ProcessInputMessage(&NewInput->MouseButtons[ButtonIndex],
                                        Win32GetKeyState(ButtonVKs[ButtonIndex]) & (1 << 15));
                }
                END_BLOCK();
                
                HDC DeviceContext = Win32GetDC(Win32State->Window);
                
                win32_offscreen_buffer *Backbuffer = &Win32State->Backbuffer;
                
                loaded_bitmap OutputTarget;
                ZeroStruct(OutputTarget);
                OutputTarget.Memory = Backbuffer->Memory;
                OutputTarget.Width = Backbuffer->Width;
                OutputTarget.Height = Backbuffer->Height;
                OutputTarget.Pitch = Backbuffer->Pitch;
                
                render_commands Commands_ = BeginRenderCommands(PushBufferSize, PushBuffer, Backbuffer->Width, Backbuffer->Height);
                render_commands *Commands = &Commands_;
                
#if KENGINE_INTERNAL
                BEGIN_BLOCK("AppUpdateFrame");
                if(Win32State->AppUpdateFrame)
                {
                    Win32State->AppUpdateFrame(Platform, Commands, &Win32State->Arena, NewInput);
                }
                END_BLOCK();
                if(DllNeedsToBeReloaded)
                {
                    Win32CompleteAllWork(Platform->PerFrameWorkQueue);
                    Win32CompleteAllWork(Platform->BackgroundWorkQueue);
                    SetDebugEventRecording(false);
                }
                
                if(Win32State->DebugUpdateFrame)
                {
                    Win32State->DebugUpdateFrame(Platform, Commands, &Win32State->Arena, NewInput);
                }
                
                
                if(DllNeedsToBeReloaded)
                {
                    Win32UnloadAppCode(Win32State);
                    Win32LoadAppCode(Platform, Win32State, NewDLLWriteTime);
                    SetDebugEventRecording(true);
                }
                
#else
                AppUpdateFrame(Platform, Commands, &Win32State->Arena, NewInput);
#endif
                
                BEGIN_BLOCK("ExpandRenderStorage");
                u32 NeededSortMemorySize = Commands->PushBufferElementCount * sizeof(sort_entry);
                if(CurrentSortMemorySize < NeededSortMemorySize)
                {
                    Win32VirtualFree(SortMemory, 0, MEM_RELEASE);
                    while(CurrentSortMemorySize < NeededSortMemorySize)
                    {
                        CurrentSortMemorySize *= 2;
                    }
                    SortMemory = Win32VirtualAlloc(0, CurrentSortMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                }
                
                // TODO(kstandbridge): Can we merge sort/clip and push buffer memory together?
                u32 NeededClipMemorySize = Commands->PushBufferElementCount * sizeof(sort_entry);
                if(CurrentClipMemorySize < NeededClipMemorySize)
                {
                    Win32VirtualFree(ClipMemory, 0, MEM_RELEASE);
                    while(CurrentClipMemorySize < NeededClipMemorySize)
                    {
                        CurrentClipMemorySize *= 2;
                    }
                    ClipMemory = Win32VirtualAlloc(0, CurrentClipMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                }
                END_BLOCK();
                
                BEGIN_BLOCK("Render");
                SortRenderCommands(Commands, SortMemory);
                LinearizeClipRects(Commands, ClipMemory);
                
                b32 ForceSoftwareRendering = false;
                
                DEBUG_IF(ForceSoftwareRendering)
                {
                    ForceSoftwareRendering = true;
                }
                
                if(ForceSoftwareRendering || !HardwareRendering)
                {
                    SoftwareRenderCommands(Platform, Platform->PerFrameWorkQueue, Commands, &OutputTarget);
                    if(!Win32StretchDIBits(DeviceContext, 0, 0, Backbuffer->Width, Backbuffer->Height, 0, 0, Backbuffer->Width, Backbuffer->Height,
                                           Backbuffer->Memory, &Backbuffer->Info, DIB_RGB_COLORS, SRCCOPY))
                    {
                        InvalidCodePath;
                    }
                }
                else
                {                
                    Win32OpenGLRenderCommands(Commands);
                    Win32SwapBuffers(DeviceContext);
                }
                
                EndRenderCommands(Commands);
                
                if(!Win32ReleaseDC(Win32State->Window, DeviceContext))
                {
                    InvalidCodePath;
                }
                END_BLOCK();
                Win32CompleteAllWork(Platform->PerFrameWorkQueue);
                
                app_input *Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
                
                
#if 0
                //if(!HardwareRendering)
                {                
                    BEGIN_BLOCK("FrameWait");
                    f32 FrameSeconds = Win32GetSecondsElapsed(Win32State, LastCounter, Win32GetWallClock());
                    
                    if(FrameSeconds < TargetSeconds)
                    {
                        DWORD Miliseconds = (DWORD)(1000.0f * (TargetSeconds - FrameSeconds));
                        if(Miliseconds > 0)
                        {
                            Win32Sleep(Miliseconds);
                        }
                        
                        FrameSeconds = Win32GetSecondsElapsed(Win32State, LastCounter, Win32GetWallClock());
                        
                        // NOTE(kstandbridge): FINE I'll make my own sleep function, with blackjack and hookers!
                        while(FrameSeconds < TargetSeconds)
                        {
                            FrameSeconds = Win32GetSecondsElapsed(Win32State, LastCounter, Win32GetWallClock());
                            _mm_pause();
                        }
                    }
                    END_BLOCK();
                }
#endif
                
                LARGE_INTEGER ThisCounter = Win32GetWallClock();                
                f32 MeasuredSecondsPerFrame = Win32GetSecondsElapsed(LastCounter, ThisCounter, Win32State->PerfCountFrequency);
                f32 ExactTargetFramesPerUpdate = MeasuredSecondsPerFrame*(f32)MonitorRefreshHz;
                u32 NewExpectedFramesPerUpdate = RoundF32ToU32(ExactTargetFramesPerUpdate);
                ExpectedFramesPerUpdate = NewExpectedFramesPerUpdate;
                
                TargetSecondsPerFrame = MeasuredSecondsPerFrame;
                
                DEBUG_FRAME_END(MeasuredSecondsPerFrame);
                LastCounter = ThisCounter;
                
            }
        }
        else
        {
            // TODO(kstandbridge): Error creating window
        }
    }
    else
    {
        // TODO(kstandbridge): Error registering window class
    }
    
    Win32ExitProcess(0);
    
    InvalidCodePath;
}