#ifndef KENGINE_RENDER_GROUP_H

typedef struct
{
    render_commands *Commands;
    u16 CurrentClipRectIndex;
    
    loaded_glyph *Glyphs;
    
    // TODO(kstandbridge): Arena will be removed when we no longer load glyphs in draw routine
    memory_arena *Arena;
} render_group;

typedef struct
{
    v3 OffsetP;
    f32 Scale;
    f32 SortBias;
} object_transform;

typedef struct
{
    v2 P;
    f32 Scale;
    b32 Valid;
    f32 SortKey;
} entity_basis_p_result;

#define KENGINE_RENDER_GROUP_H
#endif //KENGINE_RENDER_GROUP_H
