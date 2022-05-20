
internal void
TextElement(ui_layout *Layout, v2 P, string Str, ui_interaction Interaction, v2 TextOffset, v2 Padding, f32 Scale)
{
    rectangle2 Bounds = TextOpInternal(TextOp_SizeText, Layout->RenderGroup, Layout->State->Assets, P, Scale, Str);
    
    v2 Dim = V2(Bounds.Max.X - Bounds.Min.X, 
                Bounds.Max.Y - Bounds.Min.Y);
    Dim = V2Add(Dim, Padding);
    
    b32 IsHot = InteractionIsHot(Layout->State, Interaction);
    
    v4 ButtonColor = IsHot ? V4(0, 0, 1, 1) : V4(1, 0, 0, 1);
    
    b32 IsSelected = InteractionIsSelected(Layout->State, Interaction);
    if(IsSelected)
    {
        ButtonColor = V4(0, 1, 0, 1);
    }
    
    PushRect(Layout->RenderGroup, P, Dim, ButtonColor);
    WriteLine(Layout->RenderGroup, Layout->State->Assets, V2Subtract(P, TextOffset), Scale, Str);
    
    if(IsInRectangle(Rectangle2(P, V2Add(P, Dim)), Layout->MouseP))
    {
        Layout->State->NextHotInteraction = Interaction;
    }
}

internal ui_layout
BeginUIFrame(ui_state *UiState, memory_arena *Arena, render_group *RenderGroup, app_input *Input, f32 Scale)
{
    ui_layout Result;
    ZeroStruct(Result);
    
    Result.State = UiState;
    Result.Arena = Arena;
    Result.RenderGroup = RenderGroup;
    
    Result.Scale = Scale;
    Result.MouseP = V2(Input->MouseX, Input->MouseY);
    Result.dMouseP = V2Subtract(Result.MouseP, UiState->LastMouseP);
    
    UiState->ToExecute = UiState->NextToExecute;
    ClearInteraction(&UiState->NextToExecute);
    
    return Result;
}

internal void
HandleUIInteractionsInternal(ui_layout *Layout, app_input *Input)
{
    // NOTE(kstandbridge): Input text
    ui_interaction SelectedInteraction = Layout->State->SelectedInteraction;
    if(Input->Text[0] != '\0')
    {
        if(SelectedInteraction.Type == UiInteraction_TextInput)
        {
            editable_string *Str = SelectedInteraction.Str;
            
            char *At = Input->Text;
            while(*At != '\0')
            {
                if(Str->Length < Str->Size)
                {
                    ++Str->Length;
                    umm Index = Str->Length;
                    while(Index > Str->SelectionStart)
                    {
                        Str->Data[Index] = Str->Data[Index - 1];
                        --Index;
                    }
                    Str->Data[Str->SelectionStart++] = *At;
                }
                ++At;
            }
            
        }
    }
    // NOTE(kstandbridge): Keyboard buttons
    for(keyboard_button_type Type = 0;
        Type != KeyboardButton_Count;
        ++Type)
    {
        
        if(WasPressed(Input->KeyboardButtons[Type]))
        {
            if(SelectedInteraction.Type == UiInteraction_TextInput)
            {
                editable_string *Str = SelectedInteraction.Str;
                if(Type == KeyboardButton_Backspace)
                {
                    if(Str->Length > 0)
                    {
                        umm StartMoveIndex = Str->SelectionStart--;
                        while(StartMoveIndex < Str->Length)
                        {
                            Str->Data[StartMoveIndex - 1] = Str->Data[StartMoveIndex++];
                        }
                        Str->Data[--Str->Length] = '\0';
                    }
                    
                }
                if(Type == KeyboardButton_Right)
                {
                    if(Str->SelectionStart < Str->Length)
                    {
                        ++Str->SelectionStart;
                    }
                }
                if(Type == KeyboardButton_Left)
                {
                    if(Str->SelectionStart > 0)
                    {
                        --Str->SelectionStart;
                    }
                }
            }
        }
    }
    
    // NOTE(kstandbridge): Mouse buttons
    u32 TransitionCount = Input->MouseButtons[MouseButton_Left].HalfTransitionCount;
    b32 MouseButton = Input->MouseButtons[MouseButton_Left].EndedDown;
    if(TransitionCount % 2)
    {
        MouseButton = !MouseButton;
    }
    
    for(u32 TransitionIndex = 0;
        TransitionIndex <= TransitionCount;
        ++TransitionIndex)
    {
        b32 MouseDown = false;
        b32 MouseUp = false;
        if(TransitionIndex != 0)
        {
            MouseDown = MouseButton;
            MouseUp = !MouseButton;
        }
        
        b32 EndInteraction = false;
        
        if(MouseDown)
        {
            Layout->State->SelectedInteraction = Layout->State->HotInteraction;
        }
        
        switch(Layout->State->Interaction.Type)
        {
            case UiInteraction_ImmediateButton:
            {
                if(MouseUp)
                {
                    Layout->State->NextToExecute = Layout->State->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            // TODO(kstandbridge): InteractionSelect that could choose a textbox or row in list
            
            case UiInteraction_None:
            {
                Layout->State->HotInteraction = Layout->State->NextHotInteraction;
                if(MouseDown)
                {
                    Layout->State->Interaction = Layout->State->NextHotInteraction;
                }
            } break;
            
            default:
            {
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            }
        }
        
        if(EndInteraction)
        {
            ClearInteraction(&Layout->State->Interaction);
        }
        
        MouseButton = !MouseButton;
    }
    
    ClearInteraction(&Layout->State->NextHotInteraction);
}

internal void
DrawUIInternal(ui_layout *Layout)
{
    f32 CurrentY = 0.0f;
    f32 TotalHeight = (f32)Layout->RenderGroup->OutputTarget->Height;
    f32 RemainingHeight = TotalHeight - Layout->UsedHeight;
    f32 HeightPerFill = RemainingHeight / Layout->FillRows;
    for(ui_layout_row *Row = Layout->LastRow;
        Row;
        Row = Row->Next)
    {
        f32 TotalWidth = (f32)Layout->RenderGroup->OutputTarget->Width;
        f32 RemainingWidth = TotalWidth - Row->UsedWidth;
        f32 WidthPerSpacer = RemainingWidth / Row->SpacerCount;
        v2 P = V2(TotalWidth, CurrentY);
        
        for(ui_element *Element = Row->LastElement;
            Element;
            Element = Element->Next)
        {
            if(Element->Type == ElementType_Spacer)
            {
                P.X -= WidthPerSpacer;
            }
            else
            {
                P.X -= Element->Dim.X;
                v2 HeightDifference = V2(0.0f, Row->MaxHeight - Element->Dim.Y);
                v2 TextOffset = V2(0.0f, Element->TextOffset - HeightDifference.Y);
                if(Row->Type == LayoutType_Fill)
                {
                    TextOffset.Y -= (0.5f*HeightPerFill) - 0.5f*(Element->Dim.Y);
                    HeightDifference.Y = HeightPerFill - Element->Dim.Y;
                }
                else
                {
                    Assert(Row->Type == LayoutType_Auto);
                }
                
                if(Row->SpacerCount == 0)
                {
                    HeightDifference.X = (RemainingWidth/Row->ElementCount);
                    TextOffset.X -= 0.5f*HeightDifference.X;
                    P.X -= HeightDifference.X;
                }
                
                switch(Element->Type)
                {
                    case ElementType_TextBox:
                    {
                        TextElement(Layout, P, Element->Label, Element->Interaction, TextOffset, HeightDifference, Layout->Scale);
                        
                        if(Element->Interaction.Type == UiInteraction_TextInput)
                        {
                            editable_string *Str = Element->Interaction.Str;
                            rectangle2 TextBounds = GetTextSize(Layout->RenderGroup, Layout->State->Assets, V2(0, 0), Layout->Scale, StringInternal(Str->SelectionStart, Str->Data));
                            v2 SelectionStartDim = V2(TextBounds.Max.X - TextBounds.Min.X, 0);
                            
                            PushRect(Layout->RenderGroup, V2Add(P, SelectionStartDim), V2(5.0f, Element->Dim.Y), V4(0.0f, 0.0f, 0.0f, 1.0f));
                        }
                        
                    } break;
                    
                    case ElementType_Static:
                    case ElementType_Checkbox:
                    case ElementType_Slider:
                    case ElementType_Button:
                    {
                        TextElement(Layout, P, Element->Label, Element->Interaction, TextOffset, HeightDifference, Layout->Scale);
                        
                    } break;
                    
                    InvalidDefaultCase;
                }
                
            }
        }
        if(Row->Type == LayoutType_Auto)
        {
            CurrentY += Row->MaxHeight;
        }
        else
        {
            Assert(Row->Type == LayoutType_Fill);
            CurrentY += HeightPerFill;
        }
    }
}

internal void
EndUIFrame(ui_layout *Layout, app_input *Input)
{
    Assert(!Layout->IsCreatingRow);
    
    HandleUIInteractionsInternal(Layout, Input);
    
    Layout->State->LastMouseP = Layout->MouseP;
    
    DrawUIInternal(Layout);
}

inline void
BeginRow(ui_layout *Layout, ui_layout_type Type)
{
    Assert(!Layout->IsCreatingRow);
    Layout->IsCreatingRow = true;
    
    ui_layout_row *Row = PushStruct(Layout->Arena, ui_layout_row);
    ZeroStruct(*Row);
    Row->Next = Layout->LastRow;
    Layout->LastRow = Row;
    
    Row->Type = Type;
}

inline void
EndRow(ui_layout *Layout)
{
    Assert(Layout->IsCreatingRow);
    Layout->IsCreatingRow = false;
    
    ui_layout_row *Row = Layout->LastRow;
    if(Row->Type == LayoutType_Fill)
    {
        ++Layout->FillRows;
    }
    else
    {
        Assert(Row->Type == LayoutType_Auto);
    }
    
    for(ui_element *Element = Row->LastElement;
        Element;
        Element = Element->Next)
    {
        if(Element->Type == ElementType_Spacer)
        {
            ++Row->SpacerCount;
        }
        else
        {
            ++Row->ElementCount;
            rectangle2 TextBounds = GetTextSize(Layout->RenderGroup, Layout->State->Assets, V2(0, 0), Layout->Scale, Element->Label);
            Element->TextOffset = TextBounds.Min.Y;
            Element->Dim = V2(TextBounds.Max.X - TextBounds.Min.X, 
                              TextBounds.Max.Y - TextBounds.Min.Y);
            
            Row->UsedWidth += Element->Dim.X;
            if(Element->Dim.Y > Row->MaxHeight)
            {
                Row->MaxHeight = Element->Dim.Y;
            }
        }
    }
    
    if(Row->Type == LayoutType_Auto)
    {
        Layout->UsedHeight += Row->MaxHeight;
    }
}

inline ui_element *
PushElementInternal(ui_layout *Layout, ui_element_type ElementType, ui_element_type InteractionType, u32 ID, string Str, void *Target)
{
    Assert(Layout->IsCreatingRow);
    
    ui_element *Element = PushStruct(Layout->Arena, ui_element);
    ZeroStruct(*Element);
    Element->Next = Layout->LastRow->LastElement;
    Layout->LastRow->LastElement = Element;
    
    Element->Type = ElementType;
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = ID;
    Element->Interaction.Type = InteractionType;
    Element->Interaction.Generic = Target;
    Element->Label = Str;
    
    return Element;
}

inline void
PushSpacerElement(ui_layout *Layout)
{
    Assert(Layout->IsCreatingRow);
    
    PushElementInternal(Layout, ElementType_Spacer, UiInteraction_NOP, 0, String(""), 0);
}
inline b32
PushButtonElement(ui_layout *Layout, u32 ID, string Str)
{
    Assert(Layout->IsCreatingRow);
    
    ui_element *Element = PushElementInternal(Layout, ElementType_Button, UiInteraction_ImmediateButton, ID, Str, 0);
    
    b32 Result = InteractionsAreEqual(Element->Interaction, Layout->State->ToExecute);
    return Result;
}

inline void
PushScrollElement(ui_layout *Layout, u32 ID, string Str, v2 *TargetP)
{
    Assert(Layout->IsCreatingRow);
    
    ui_element *Element = PushElementInternal(Layout, ElementType_Slider, UiInteraction_Draggable, ID, Str, TargetP);
    
    if(InteractionsAreEqual(Element->Interaction, Layout->State->Interaction))
    {
        v2 *P = Layout->State->Interaction.P;
        *P = V2Add(*P, V2Multiply(V2Set1(0.01f), Layout->dMouseP));
    }
}

inline void
PushTextInputElement(ui_layout *Layout, u32 ID, editable_string *Target)
{
    Assert(Layout->IsCreatingRow);
    
    PushElementInternal(Layout, ElementType_TextBox, UiInteraction_TextInput, ID, StringInternal(Target->Length, Target->Data), Target);
}
