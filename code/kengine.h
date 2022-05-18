#ifndef KENGINE_H

/* TODO(kstandbridge): 

- UI push system
- UI render system
- meta linked list
- meta double linked list
- meta free list
- Format string c	Character
- Format string o	Unsigned octal
- Format string x	Unsigned hexadecimal integer
- Format string X	Unsigned hexadecimal integer (uppercase)
- Format string F	Decimal floating point, uppercase
- Format string e	Scientific notation (mantissa/exponent), lowercase
- Format string E	Scientific notation (mantissa/exponent), uppercase
- Format string g	Use the shortest representation: %e or %f
- Format string G	Use the shortest representation: %E or %F
- Format string a	Hexadecimal floating point, lowercase
- Format string A	Hexadecimal floating point, uppercase
- Format string p	Pointer address
- Format string n	Nothing printed.
- Format string flags
- Format string width
- introspect method names remove underscores

*/

#include "kengine_platform.h"
#include "kengine_shared.h"
#include "kengine_intrinsics.h"
#include "kengine_generated.h"
#include "kengine_math.h"
#include "kengine_render_group.h"

#pragma pack(push, 1)
typedef struct bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    s32 Width;
    s32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 SizeOfBitmap;
    s32 HorzResolution;
    s32 VertResolution;
    u32 ColorsUsed;
    u32 ColorsImportant;
    
    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
} bitmap_header;
#pragma pack(pop)

typedef enum ui_interaction_type
{
    UiInteraction_None,
    
    UiInteraction_NOP,
    
    UiInteraction_ImmediateButton,
    UiInteraction_Draggable,
    
} ui_interaction_type;

typedef struct app_state app_state;

typedef struct ui_interaction
{
    u32 ID;
    ui_interaction_type Type;
    
    union
    {
        void *Generic;
        v2 *P;
    };
} ui_interaction;

typedef struct app_state
{
    b32 IsInitialized;
    
    f32 Time;
    
    memory_arena PermanentArena;
    memory_arena TransientArena;
    
    loaded_bitmap TestBMP;
    loaded_bitmap TestFont;
    
    loaded_bitmap Glyphs[256];
    
    v2 LastMouseP;
    ui_interaction Interaction;
    
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    ui_interaction ToExecute;
    ui_interaction NextToExecute;
    
    v2 TestP;
    s32 TestCounter;
    
} app_state;


inline b32
InteractionsAreEqual(ui_interaction A, ui_interaction B)
{
    b32 Result = ((A.ID == B.ID) &&
                  (A.Type == B.Type) &&
                  (A.Generic == B.Generic));
    
    return Result;
}

inline b32
InteractionIsHot(app_state *AppState, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(AppState->HotInteraction, A);
    
    if(A.Type == UiInteraction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32 
InteractionIsValid(ui_interaction *Interaction)
{
    b32 Result = (Interaction->Type != UiInteraction_None);
    
    return Result;
}

inline void
ClearInteraction(ui_interaction *Interaction)
{
    Interaction->Type = UiInteraction_None;
    Interaction->Generic = 0;
}

#define KENGINE_H
#endif //KENGINE_H
