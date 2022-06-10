#include "kengine.h"

global platform_api Platform;
global colors Colors;

#if KENGINE_INTERNAL
global app_memory *GlobalDebugMemory;
#endif

#include "kengine_render_group.c"
#include "kengine_ui.c"
#include "kengine_assets.c"
#include "kengine_debug.c"

extern void
AppUpdateAndRender(app_memory *Memory, app_input *Input, app_offscreen_buffer *Buffer)
{
    Platform = Memory->PlatformAPI;
    
#if KENGINE_INTERNAL
    GlobalDebugMemory = Memory;
#endif
    
#if 1
    
    Colors.Clear = RGBColor(255, 255, 255, 255);
    
    Colors.Text = RGBColor(0, 0, 0, 255);
    Colors.TextBorder = RGBColor(122, 122, 122, 255);
    Colors.SelectedTextBorder = RGBColor(0, 120, 215, 255);
    Colors.SelectedTextBackground = RGBColor(0, 120, 215, 255);
    Colors.SelectedText = RGBColor(255, 255, 255, 255);
    
    Colors.SelectedOutline = RGBColor(15, 15, 15, 255);
    Colors.SelectedOutlineAlt = RGBColor(255, 255, 255, 255);
    
    Colors.CheckBoxBorder = RGBColor(51, 51, 51, 255);
    Colors.CheckBoxBackground = RGBColor(255, 255, 255, 255);
    Colors.CheckBoxBorderClicked = RGBColor(0, 84, 153, 255);
    Colors.CheckBoxBackgroundClicked = RGBColor(204, 228, 247, 255);
    
    Colors.TextBackground = RGBColor(255, 255, 255, 255);
    Colors.Caret = RGBColor(0, 0, 0, 255);
    
    Colors.HotButton = RGBColor(229, 241, 251, 255);
    Colors.Button = RGBColor(225, 225, 225, 255);
    Colors.ClickedButton = RGBColor(204, 228, 247, 255);
    Colors.ButtonBorder = RGBColor(173, 173, 173, 173);
    
#else
    
    // NOTE(kstandbridge): Dark mode
    Colors.Text = RBGColor(255, 255, 255, 255);
    Colors.Clear = RGBColor(56, 56, 56, 255);
    Colors.HotButton = RGBColor(69, 69, 69, 255);
    Colors.Button = RGBColor(51, 51, 51, 255);
    Colors.ClickedButton = RGBColor(102, 102, 102, 255);
    Colors.ButtonBorder = RGBColor(155, 155, 155, 255);
    Colors.Caret = RGBColor(230, 230, 230, 255);
    
#endif
    
    app_state *AppState = (app_state *)Memory->Storage;
    
    if(!AppState->IsInitialized)
    {
        InitializeArena(&AppState->PermanentArena, Memory->StorageSize - sizeof(app_state), (u8 *)Memory->Storage + sizeof(app_state));
        
        AppState->TestBMP = LoadBMP(&AppState->PermanentArena, "test_tree.bmp");
        AppState->TestFont = Platform.DEBUGGetGlyphForCodePoint(&AppState->PermanentArena, 'K');
        AppState->TestP = V2(500.0f, 500.0f);
        
        SubArena(&AppState->Assets.Arena, &AppState->PermanentArena, Megabytes(512));
        
        SubArena(&AppState->TransientArena, &AppState->PermanentArena, Megabytes(256));
        
        //AppState->UiState.Assets = &AppState->Assets;
        
        AppState->UiScale = V2(0.2f, 0.0f);
        
        AppState->TestString.Length = 1;
        AppState->TestString.SelectionStart = 1;
        AppState->TestString.SelectionEnd = 1;
        AppState->TestString.Size = 32;
        AppState->TestString.Data = PushSize(&AppState->PermanentArena, AppState->TestString.Size);
        AppState->TestString.Data[0] = ':';
        
        string TheString = String("Lorem ipsum dolor sit amet, consectetur adipiscing elit. \nDuis mattis iaculis nunc, vitae laoreet dolor. Sed condimentum,\n nulla venenatis interdum gravida, metus magna vestibulum urna,\n nec euismod lectus dui at mauris. Aenean venenatis ut ligula\n sit amet ullamcorper. Vivamus in magna tristique, sodales\n magna ac, sodales purus. Proin ut est ante. Quisque et \n sollicitudin velit. Fusce id elementum augue, non maximus\n magna. Aliquam finibus erat sit amet nibh pharetra, eget pharetra\n est convallis. Nam sodales tellus imperdiet ante hendrerit, ut\ntristique ex euismod. Morbi gravida elit orci, at ultrices\n turpis efficitur ac. Fusce dapibus auctor lorem quis tempor.\nSuspendisse at egestas justo. Nam bibendum ultricies molestie.\n Aenean lobortis vehicula ante, elementum eleifend eros congue\n eget. Phasellus placerat varius nunc non faucibus.");
        AppState->LongString.Length = (u32)TheString.Size;
        AppState->LongString.Size = TheString.Size;
        AppState->LongString.Data = TheString.Data;
        AppState->LongString.SelectionStart = 10;
        AppState->LongString.SelectionEnd = 5;
        
        AppState->IsInitialized = true;
    }
    
#if KENGINE_INTERNAL
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    
    if(!DebugState->IsInitialized)
    {
        InitializeArena(&DebugState->Arena, Memory->DebugStorageSize - sizeof(debug_state), (u8 *)Memory->DebugStorage + sizeof(debug_state));
        
        DebugState->Assets = &AppState->Assets;
        
        DebugState->LeftEdge = 20.0f;
        DebugState->AtY = 0.0f;
        DebugState->FontScale = 0.2f;
        
        DebugState->IsInitialized = true;
    }
    
#endif
    
    AppState->Time += Input->dtForFrame;
    
#if 1
    loaded_bitmap DrawBufferInteral;
    loaded_bitmap *DrawBuffer = &DrawBufferInteral;
    DrawBuffer->Memory = Buffer->Memory;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
#else
    loaded_bitmap DrawBufferInteral;
    loaded_bitmap *DrawBuffer = &DrawBufferInteral;
    s32 Y = 320;
    s32 X = 240;
    DrawBuffer->Memory = (u8 *)Buffer->Memory + X*BITMAP_BYTES_PER_PIXEL + Y*Buffer->Pitch;
    DrawBuffer->Width = 1024;
    DrawBuffer->Height = 768;
    DrawBuffer->Pitch = Buffer->Pitch;
#endif
    
#if KENGINE_INTERNAL
    DEBUGStart(DrawBuffer);
#endif
    
    {
        temporary_memory TempMem = BeginTemporaryMemory(&AppState->TransientArena);
        
        render_group *RenderGroup = AllocateRenderGroup(TempMem.Arena, Megabytes(4), DrawBuffer);
        
        if(Memory->ExecutableReloaded)
        {
            PushClear(RenderGroup, V4(1, 1, 1, 1));
        }
        else
        {
            PushClear(RenderGroup, V4(0, 0, 0, 1));
        }
        
        
#if 0
        v2 P = V2(Buffer->Width*0.5f, Buffer->Height*0.5f);
        f32 Angle = 0.1f*AppState->Time;
        PushBitmap(RenderGroup, &AppState->TestBMP, (f32)AppState->TestBMP.Height, P, V4(1, 1, 1, 1), Angle);
#endif
        
#if 0
        v2 RectP = V2((f32)Buffer->Width / 2, (f32)Buffer->Height / 2);
        // TODO(kstandbridge): push circle?
        DrawCircle(DrawBuffer, RectP, V2Add(RectP, V2(50, 50)), V4(1, 1, 0, 1.0f), Rectangle2i(0, Buffer->Width, 0, Buffer->Height));
#endif
        
        
        tile_render_work Work;
        ZeroStruct(Work);
        Work.Group = RenderGroup;
        Work.ClipRect.MinX = 0;
        Work.ClipRect.MaxX = DrawBuffer->Width;
        Work.ClipRect.MinY = 0;
        Work.ClipRect.MaxY = DrawBuffer->Height;
        TileRenderWorkThread(&Work);
        EndTemporaryMemory(TempMem);
    }
    
    
    temporary_memory TempMem = BeginTemporaryMemory(&AppState->TransientArena);
    
    ui_layout *Layout = BeginUIFrame(TempMem.Arena, DrawBuffer);
    
#if 0
    
    BeginRow(Layout);
    {    
        Spacer(Layout);
        
        BeginRow(Layout);
        {    
            Spacer(Layout);
        }
        EndRow(Layout);
        
        BeginRow(Layout);
        {    
            Spacer(Layout);
            
            BeginRow(Layout);
            {
                Spacer(Layout);
            }
            EndRow(Layout);
            
            BeginRow(Layout);
            {
                Spacer(Layout);
            }
            EndRow(Layout);
            
            
            BeginRow(Layout);
            {
                Spacer(Layout);
            }
            EndRow(Layout);
        }
        EndRow(Layout);
        
    }
    EndRow(Layout);
#else
    
    
    BeginRow(Layout);
    {
        Label(Layout, "Show: ");
        Checkbox(Layout, "Empty Worlds", &AppState->ShowEmptyWorlds); // Editable bool
        Checkbox(Layout, "Local", &AppState->ShowLocal); // Editable bool
        Checkbox(Layout, "Available", &AppState->ShowAvailable); // Editable bool
        Spacer(Layout);
        Label(Layout, "Filter: ");
        Textbox(Layout, &AppState->FilterText);
    }
    EndRow(Layout);
    
    BeginRow(Layout);
    {
        // NOTE(kstandbridge): Listview Worlds
        Spacer(Layout);
    }
    EndRow(Layout);
    
    BeginRow(Layout);
    {
        BeginRow(Layout);
        {
            // NOTE(kstandbridge): Listview Worlds
            Spacer(Layout);
        }
        EndRow(Layout);
        
        Splitter(Layout, &UserSettings->BuildRunSplitSize);
        
        BeginRow(Layout);
        {   
            BeginRow(Layout);
            Checkbox(Layout, "Edit run params", &UserSettings->EditRunParams);
            EndRow(Layout);
            
            BeginRow(Layout);
            DropDown(Layout, "default");
            EndRow(Layout);
            
            BeginRow(Layout);
            {
                Textbox(Layout, "thing.exe");
                Button(Layout, "Copy");
            }
            EndRow(Layout);
            
            BeginRow(Layout);
            {
                Label(Layout, "Sync command");
                Button(Layout, "Copy");
            }
            EndRow(Layout);
            
            BeginRow(Layout);
            {
                Button(Layout, "Run");
                Button(Layout, "Sync");
                Button(Layout, "Cancel");
                Button(Layout, "Clean");
                Button(Layout, "Open Folder");
            }
            EndRow(Layout);
            
            BeginRow(Layout);
            {
                Label(Layout, "DownloadPath");
                Button(Layout, "Copy");
            }
            EndRow(Layout);
        }
        EndRow(Layout);
        
    }
    EndRow(Layout);
    
    
    
    
    BeginRow(Layout);
    Textbox(Layout, &AppState->LogMessages, &AppSate->SelectedMessage);
    EndRow(Layout);
    
    BeginRow(Layout);
    {
        Button(Layout, "Settings");
        Button(Layout, "Settings");
        Button(Layout, "Settings");
        Spacer(Layout);
        Button(Layout, "Open Remote Config");
        Button(Layout, "Open Log");
    }
    EndRow(Layout);
    
    
    
#endif
    
    EndUIFrame(Layout);
    
    EndTemporaryMemory(TempMem);
    
#if KENGINE_INTERNAL
    DEBUGEnd();
#endif
    
    CheckArena(&AppState->PermanentArena);
    CheckArena(&AppState->TransientArena);
    CheckArena(&AppState->Assets.Arena);
}
