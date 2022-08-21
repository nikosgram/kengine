// NOTE(kstandbridge): This is the consumer code, this would not be part of kengine at all and something implemented by the user of the framework

// TODO(kstandbridge): Perhaps this should be a dll export?

#if 0
internal void
DrawAppGrid(app_state *AppState, ui_state *UIState, render_group *RenderGroup, memory_arena *PermArena, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    ui_grid Grid = BeginGrid(UIState, TempArena, Bounds, 3, 3);
    {
        SetColumnWidth(&Grid, 0, 0, 128.0f);
        SetColumnWidth(&Grid, 2, 2, 256.0f);
        
        SetRowHeight(&Grid, 1, SIZE_AUTO);
        
        if(Button(&Grid, RenderGroup, 0, 0, GenerateInteractionId(AppState), AppState, String("Top Left"))) { __debugbreak(); }
        if(Button(&Grid, RenderGroup, 1, 0, GenerateInteractionId(AppState), AppState, String("Top Middle"))) { __debugbreak(); }
        if(Button(&Grid, RenderGroup, 2, 0, GenerateInteractionId(AppState), AppState, String("Top Right"))) { __debugbreak(); }
        
        if(Button(&Grid, RenderGroup, 0, 1, GenerateInteractionId(AppState), AppState, String("Middle Left"))) { __debugbreak(); }
        
        ui_grid InnerGrid = BeginGrid(UIState, TempArena, GetCellBounds(&Grid, 1, 1), 3, 2);
        {
            SetRowHeight(&InnerGrid, 1, SIZE_AUTO);
            if(Button(&InnerGrid, RenderGroup, 0, 0, GenerateInteractionId(AppState), AppState, String("Inner NW"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 1, 0, GenerateInteractionId(AppState), AppState, String("Inner NE"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 0, 1, GenerateInteractionId(AppState), AppState, String("Inner W"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 1, 1, GenerateInteractionId(AppState), AppState, String("Inner E"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 0, 2, GenerateInteractionId(AppState), AppState, String("Inner SW"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 1, 2, GenerateInteractionId(AppState), AppState, String("Inner SE"))) { __debugbreak(); }
        }
        EndGrid(&InnerGrid);
        
        if(Button(&Grid, RenderGroup, 2, 1, GenerateInteractionId(AppState), AppState, String("Middle Right"))) { __debugbreak(); }
        
        if(Button(&Grid, RenderGroup, 0, 2, GenerateInteractionId(AppState), AppState, String("Bottom Left"))) { __debugbreak(); }
        if(Button(&Grid, RenderGroup, 1, 2, GenerateInteractionId(AppState), AppState, String("Bottom Middle"))) { __debugbreak(); }
        if(Button(&Grid, RenderGroup, 2, 2, GenerateInteractionId(AppState), AppState, String("Bottom Right"))) { __debugbreak(); }
    }
    EndGrid(&Grid);
}
#endif

typedef struct node_link
{
    struct node *Node;
    
    struct node_link *Prev;
    struct node_link *Next;
} node_link;

inline void
NodeLinkInit(node_link *Sentinel)
{
    Sentinel->Node = 0;
    Sentinel->Prev = Sentinel;
    Sentinel->Next = Sentinel;
}

inline void
NodeLinkInsert(node_link *Sentinel, node_link *Link)
{
    Assert(Link->Node);
    Link->Next = Sentinel->Next;
    Link->Prev = Sentinel;
    Link->Next->Prev = Link;
    Link->Prev->Next = Link;
}

inline void
NodeLinkInsertAtLast(node_link *Sentinel, node_link *Link)
{
    Assert(Link->Node);
    Link->Next = Sentinel;
    Link->Prev = Sentinel->Prev;
    Link->Next->Prev = Link;
    Link->Prev->Next = Link;
}

inline void
NodeLinkRemove(node_link *Link)
{
    Link->Prev->Next = Link->Next;
    Link->Next->Prev = Link->Prev;
}

inline node_link *
NodeLinkSortedGetByIndex(node_link *Sentinel, sort_entry *SortEntries, u32 SortEntryCount, u32 Index)
{
    sort_entry *FirstNodeEntry = SortEntries + Index;
    u32 EntryIndex = FirstNodeEntry->Index;
    
    node_link *Result = Sentinel->Next;
    
    while(EntryIndex)
    {
        Result = Result->Next;
        --EntryIndex;
    }
    
    return Result;
}

inline b32
NodeLinkIsEmpty(node_link *Sentinel)
{
    b32 Result = Sentinel->Next == Sentinel;
    return Result;
}

typedef struct node
{
    b32 Obstacle;
    b32 Visited;
    
    f32 GlobalGoal;
    f32 LocalGoal;
    
    v2i P;
    
    struct node_link NeighbourSentinal;
    struct node *Parent;
    
    struct node *Next;
} node;

global node *EndNode;
global node *StartNode;
global node *Nodes;
global s32 Columns;
global s32 Rows;

#define PADDING 10
#define CEL_DIM 40

inline v2
GetCellP(v2 StartingPoint, s32 Column, s32 Row)
{
    v2 Result = StartingPoint;
    Result.X += (PADDING + CEL_DIM) * Column;
    Result.Y += (PADDING + CEL_DIM) * Row;
    
    return Result;
}

internal void
DrawAppGrid(app_state *AppState, ui_state *UIState, render_group *RenderGroup, memory_arena *PermArena, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    v2 MouseP = V2(Input->MouseX, Input->MouseY);
    
    // NOTE(kstandbridge): Calculate number of nodes we have
    v2 TotalDim = V2Subtract(Bounds.Max, Bounds.Min);
    
    // NOTE(kstandbridge): Generate nodes
    if(Nodes == 0)
    {
        Columns = CeilF32ToS32(TotalDim.X / (PADDING+CEL_DIM)) - 1;
        Rows = CeilF32ToS32(TotalDim.Y / (PADDING+CEL_DIM)) - 1;
        Nodes = PushArray(PermArena, Rows * Columns, node);
        for(s32 Row = 0;
            Row < Rows;
            ++Row)
        {
            for(s32 Column = 0;
                Column < Columns;
                ++Column)
            {
                node *Node = Nodes + (Row*Columns + Column);
                Node->Obstacle = false;
                Node->Visited = false;
                Node->P = V2i(Column, Row);
                Node->Parent = 0;
                NodeLinkInit(&Node->NeighbourSentinal);
                Node->Next = 0;
            }
        }
        
        for(s32 Row = 0;
            Row < Rows;
            ++Row)
        {
            for(s32 Column = 0;
                Column < Columns;
                ++Column)
            {
                node *Node = Nodes + (Row*Columns + Column);
                if(Row > 0)
                {
                    node_link *Link = PushStruct(PermArena, node_link);
                    Link->Node = Nodes + ((Row - 1)*Columns + (Column + 0));;
                    NodeLinkInsertAtLast(&Node->NeighbourSentinal, Link);
                }
                if(Row < Rows - 1)
                {
                    node_link *Link = PushStruct(PermArena, node_link);
                    Link->Node = Nodes + ((Row + 1)*Columns + (Column + 0));
                    NodeLinkInsertAtLast(&Node->NeighbourSentinal, Link);
                    
                }
                
                if(Column > 0)
                {
                    node_link *Link = PushStruct(PermArena, node_link);
                    Link->Node = Nodes + ((Row + 0)*Columns + (Column - 1));
                    NodeLinkInsertAtLast(&Node->NeighbourSentinal, Link);
                }
                if(Column < Columns -1)
                {
                    node_link *Link = PushStruct(PermArena, node_link);
                    Link->Node = Nodes + ((Row + 0)*Columns + (Column + 1));
                    NodeLinkInsertAtLast(&Node->NeighbourSentinal, Link);
                }
            }
        }
        
        StartNode = Nodes;
        EndNode = Nodes + (Rows * Columns) - 1;
    }
    
    v2 TotalUsedDim = V2((f32)Columns * (PADDING+CEL_DIM), (f32)Rows * (PADDING+CEL_DIM));
    v2 RemainingDim = V2Subtract(TotalDim, TotalUsedDim);
    v2 StartingAt = V2Add(Bounds.Min, V2Multiply(V2Set1(0.5f), RemainingDim));
    
    b32 NeedsPathUpdate = false;
    
    u32 Index = 0;
    u32 TotalNodes = Rows * Columns;
    for(node *Node = Nodes;
        Index < TotalNodes;
        ++Index, ++Node)
    {
        v2 CellP = GetCellP(StartingAt, Node->P.X, Node->P.Y);
        v2 CellDim = V2Add(CellP, V2Set1(CEL_DIM));
        rectangle2 CellBounds = Rectangle2(CellP, CellDim);
        v4 Color = V4(1.0f, 0.5f, 0.0f, 1.0f);
        
        
        // NOTE(kstandbridge): Draw paths between nodes
        {        
            for(node_link *NeighbourLink = Node->NeighbourSentinal.Next;
                NeighbourLink != &Node->NeighbourSentinal;
                NeighbourLink = NeighbourLink->Next)
            {
                node *NeighbourNode = NeighbourLink->Node;
                v2 PathP = GetCellP(StartingAt, NeighbourNode->P.X, NeighbourNode->P.Y);
                rectangle2 PathBounds = Rectangle2(V2Add(CellP, V2Set1(0.5f*CEL_DIM)),
                                                   V2Add(PathP, V2Set1(0.5f*CEL_DIM)));
                PathBounds = Rectangle2AddRadiusTo(PathBounds, 1.0f);
                PushRenderCommandRectangle(RenderGroup, Color, PathBounds, 1.0f);
            }
        }
        
        // NOTE(kstandbridge): Draw Node
        {   
            if(Rectangle2IsIn(CellBounds, MouseP))
            {
                if(WasPressed(Input->MouseButtons[MouseButton_Left]))
                {
                    Node->Obstacle = !Node->Obstacle;
                    NeedsPathUpdate = true;
                }
                if(WasPressed(Input->MouseButtons[MouseButton_Middle]))
                {
                    EndNode = Node;
                }
                if(WasPressed(Input->MouseButtons[MouseButton_Right]))
                {
                    StartNode = Node;
                }
                Color = V4(0.0f, 1.0f, 0.0f, 1.0f);
            }
            
            if(Node->Visited)
            {
                Color = V4(0.0f, 0.5f, 0.0f, 1.0f);
            }
            
            if(Node->Obstacle)
            {
                Color = V4(0.3f, 0.3f, 0.3f, 1.0f);
            }
            
            if(Node == StartNode)
            {
                Color = V4(0.0f, 1.0f, 0.0f, 1.0f);
            }
            if(Node == EndNode)
            {
                Color = V4(1.0f, 0.0f, 0.0f, 1.0f);
            }
            
            PushRenderCommandRectangle(RenderGroup, Color, CellBounds, 1.5f);
        }
    }
    
    // NOTE(kstandbridge): Solve A*
    if(NeedsPathUpdate)
    {
        
        // NOTE(kstandbridge): Reset nodes
        {        
            Index = 0;
            for(node *Node = Nodes;
                Index < TotalNodes;
                ++Index, ++Node)
            {
                Node->Visited = false;
                Node->GlobalGoal = F32Max;
                Node->LocalGoal = F32Max;
                Node->Parent = 0;
            }
        }
        
        // NOTE(kstandbridge): Starting node
        StartNode->LocalGoal = 0.0f;
        StartNode->GlobalGoal = V2iDistanceBetween(StartNode->P, EndNode->P);
        
        node_link NotTestedSentinel;
        NodeLinkInit(&NotTestedSentinel);
        
        node_link *CurrentLink = PushStruct(TempArena, node_link);
        CurrentLink->Node = StartNode;
        NodeLinkInsertAtLast(&NotTestedSentinel, CurrentLink); 
        
        u32 NotTestedCount = 1;
        while(!NodeLinkIsEmpty(&NotTestedSentinel) &&
              (CurrentLink->Node != EndNode))
        {
            // NOTE(kstandbridge): Sort NotTestedLinks
            sort_entry *SortEntries = PushArray(TempArena, NotTestedCount, sort_entry);
            {
                node_link *NodeLink = NotTestedSentinel.Next;
                sort_entry *SortEntry = SortEntries;
                for(u32 SortIndex = 0;
                    SortIndex < NotTestedCount;
                    ++SortIndex, ++SortEntry, NodeLink = NodeLink->Next)
                {
                    SortEntry->SortKey = NodeLink->Node->GlobalGoal;
                    SortEntry->Index = SortIndex;
                }
                BubbleSort(NotTestedCount, SortEntries);
            }
            
            
            node_link *FirstLink = NodeLinkSortedGetByIndex(&NotTestedSentinel, SortEntries, NotTestedCount, 0);
            if(FirstLink->Node->Visited)
            {
                NodeLinkRemove(FirstLink);
                --NotTestedCount;
            }
            else
                
            {            
                CurrentLink = FirstLink;
                node *CurrentNode = CurrentLink->Node;
                CurrentNode->Visited = true;
                
                // NOTE(kstandbridge): Check each Neighbour
                for(node_link *NeighbourLink = CurrentNode->NeighbourSentinal.Next;
                    NeighbourLink != &CurrentNode->NeighbourSentinal;
                    NeighbourLink = NeighbourLink->Next)
                {
                    node *NeighbourNode = NeighbourLink->Node;
                    
                    if(!NeighbourNode->Visited && !NeighbourNode->Obstacle)
                    {
                        node_link *LinkCopy = PushStruct(PermArena, node_link);
                        LinkCopy->Node = NeighbourNode;
                        NodeLinkInsertAtLast(&NotTestedSentinel, LinkCopy);
                        ++NotTestedCount;
                    }
                    
                    f32 NewDistance = V2iDistanceBetween(CurrentNode->P, NeighbourNode->P);
                    f32 PossibleLowerGoal = CurrentNode->LocalGoal + NewDistance;
                    
                    if(PossibleLowerGoal < NeighbourNode->LocalGoal)
                    {
                        NeighbourNode->Parent = CurrentNode;
                        NeighbourNode->LocalGoal = PossibleLowerGoal;
                        
                        NeighbourNode->GlobalGoal = NeighbourNode->LocalGoal + V2iDistanceBetween(NeighbourNode->P, EndNode->P);
                    }
                }
                
            }
        }
        
    }
    
    // NOTE(kstandbridge): Draw path
    {
        node *Node = EndNode;
        while(Node->Parent)
        {
            node *Parent = Node->Parent;
            
            v2 NodeP = GetCellP(StartingAt, Node->P.X, Node->P.Y);
            v2 ParentP = GetCellP(StartingAt, Parent->P.X, Parent->P.Y);
            rectangle2 PathBounds = Rectangle2(V2Add(ParentP, V2Set1(0.5f*CEL_DIM)),
                                               V2Add(NodeP, V2Set1(0.5f*CEL_DIM)));
            PathBounds = Rectangle2AddRadiusTo(PathBounds, 3.0f);
            v4 Color = V4(1.0f, 1.0f, 0.0f, 1.0f);
            PushRenderCommandRectangle(RenderGroup, Color, PathBounds, 3.0f);
            
            Node = Parent;
        }
    }
}