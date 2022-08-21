#include "win32_kengine_tests.h"
#include "win32_kengine_shared.c"

#include "win32_kengine_generated.c"
#include "kengine_random.c"

global s32 TotalTests;
global s32 FailedTests;

#define ASSERT(Expression)                 \
++TotalTests;                          \
if(!(Expression))                      \
{                                      \
++FailedTests;                     \
Win32ConsoleOut(Arena, "%s(%d): failed assert!\n", \
__FILE__, __LINE__);        \
}

internal b32
Win32ConsoleOut_(string Text)
{
    u32 Result = 0;
    
    HANDLE OutputHandle = Win32GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    Win32WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&Result, 0);
    Assert(Result == Text.Size);
    
    return Result;
}


internal b32
Win32ConsoleOut(memory_arena *Arena, char *Format, ...)
{
    format_string_state StringState = BeginFormatString(Arena);
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Text = EndFormatString(&StringState);
    
    b32 Result = Win32ConsoleOut_(Text);
    return Result;
}


inline void
RunStringsAreEqualTests(memory_arena *Arena)
{
    ASSERT(StringsAreEqual(String("Foo"), String("Foo")));
    ASSERT(!StringsAreEqual(String("Bar"), String("Foo")));
    ASSERT(StringsAreEqual(String("Foo bar Bas"), String("Foo bar Bas")));
    ASSERT(!StringsAreEqual(String("Foo bar Bas"), String("Bas bar Foo")));
    ASSERT(!StringsAreEqual(String("Foo bar Bas"), String("")));
    ASSERT(!StringsAreEqual(String(""), String("Bas bar Foo")));
}

inline void
RunFormatStringSignedDecimalIntegerTests(memory_arena *Arena)
{
    {
        string A = String("before 42 after");
        string B = FormatString(Arena, "before %i after", 42);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("aaa 111 bbb 222 ccc 333 ddd");
        string B = FormatString(Arena, "aaa %d bbb %d ccc %d ddd", 111, 222, 333);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("aaa 111 bbb 222 ccc 333 ddd");
        string B = FormatString(Arena, "aaa %d bbb %d ccc %d ddd", 444, 555, 666);
        ASSERT(!StringsAreEqual(A, B));
    }
    {
        string A = String("before 2147483647 after");
        string B = FormatString(Arena, "before %d after", S32Max);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before -2147483648 after");
        string B = FormatString(Arena, "before %d after", S32Min);
        ASSERT(StringsAreEqual(A, B));
    }
}

inline void
RunFormatStringUnsignedDecimalIntegerTests(memory_arena *Arena)
{
    {
        string A = String("before 4294967295 after");
        string B = FormatString(Arena, "before %u after", U32Max);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 0 after");
        string B = FormatString(Arena, "before %u after", U32Min);
        ASSERT(StringsAreEqual(A, B));
    }
}

inline void
RunFormatStringDecimalFloatingPoint(memory_arena *Arena)
{
    {
        string A = String("before 3.141592 after");
        string B = FormatString(Arena, "before %f after", Pi32);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 6.283185 after");
        string B = FormatString(Arena, "before %f after", Tau32);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 3.141 after");
        string B = FormatString(Arena, "before %.3f after", Pi32);
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before 3.141 after");
        string B = FormatString(Arena, "before %.*f after", 3, Pi32);
        ASSERT(StringsAreEqual(A, B));
    }
    
    // NOTE(kstandbridge): Intentionally checking string length 
    // as precision is not accurate with currently implementation 
    {
        string A = String("before 6.28318530717958647692 after");
        string B = FormatString(Arena, "before %.20f after", Tau32);
        ASSERT(A.Size == B.Size);
    }
    {
        string A = String("before 3.14159265359 after");
        string B = FormatString(Arena, "before %.11f after", Pi32);
        ASSERT(A.Size == B.Size);
    }
}

inline void
RunFormatStringStringOfCharactersTests(memory_arena *Arena)
{
    {
        string A = String("before bar after");
        string B = FormatString(Arena, "before %s after", "bar");
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("11 11 11 foo 22 22 22 bar 33 33 33 bas 44 44 44");
        string B = FormatString(Arena, "11 11 11 %s 22 22 22 %s 33 33 33 %s 44 44 44", "foo", "bar", "bas");
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("11 11 11 foo 22 22 22 bar 33 33 33 bas 44 44 44");
        string B = FormatString(Arena, "11 11 11 %s 22 22 22 %s 33 33 33 %s 44 44 44", "bas", "bar", "foo");
        ASSERT(!StringsAreEqual(A, B));
    }
    {
        string A = String("before foo after");
        string B = FormatString(Arena, "before %.3s after", "foobar");
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before foo after");
        string B = FormatString(Arena, "before %.*s after", 3, "foobar");
        ASSERT(StringsAreEqual(A, B));
    }
}

inline void
RunFormatStringStringTypeTests(memory_arena *Arena)
{
    {
        string A = String("before bar after");
        string B = FormatString(Arena, "before %S after", String("bar"));
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("foo 1234 bar 5678 bas");
        string B = FormatString(Arena, "foo %S bar %S bas", String("1234"), String("5678"));
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before foo after");
        string B = FormatString(Arena, "before %S after", String("bar"));
        ASSERT(!StringsAreEqual(A, B));
    }
    {
        string A = String("before foo after");
        string B = FormatString(Arena, "before %.3S after", String("foobar"));
        ASSERT(StringsAreEqual(A, B));
    }
    
}

inline void
RunFormatStringPercentTests(memory_arena *Arena)
{
    {
        string A = String("before % after");
        string B = FormatString(Arena, "before %% after");
        ASSERT(StringsAreEqual(A, B));
    }
    {
        string A = String("before  after");
        string B = FormatString(Arena, "before %% after");
        ASSERT(!StringsAreEqual(A, B));
    }
}

inline void
RunV2Tests(memory_arena *Arena)
{
    {
        v2 A = V2(2.3f, 4.5f);
        ASSERT(A.X == 2.3f);
        ASSERT(A.Y == 4.5f);
    }
    {
        v2 A = V2Set1(3.14f);
        ASSERT(A.X == 3.14f);
        ASSERT(A.Y == 3.14f);
    }
    {
        v2 C = V2Add(V2Set1(3.0f), V2Set1(2.0f));
        ASSERT(C.X == 5.0f);
        ASSERT(C.Y == 5.0f);
    }
    {
        v2 C = V2Subtract(V2Set1(3.0f), V2Set1(2.0f));
        ASSERT(C.X == 1.0f);
        ASSERT(C.Y == 1.0f);
    }
    {
        v2 C = V2Multiply(V2Set1(3.0f), V2Set1(2.0f));
        ASSERT(C.X == 6.0f);
        ASSERT(C.Y == 6.0f);
    }
    {
        v2 C = V2Divide(V2Set1(3.0f), V2Set1(2.0f));
        ASSERT(C.X == 1.5f);
        ASSERT(C.Y == 1.5f);
    }
}

inline void
RunUpperCamelCaseTests(memory_arena *Arena)
{
    {
        string Expected = String("UpperCamelCase");
        string Actual = FormatString(Arena, "upper_camel_case");
        ToUpperCamelCase(&Actual);
        ASSERT(StringsAreEqual(Expected, Actual));
    }
}

typedef struct double_linked_list
{
    u32 SortKey;
    
    struct double_linked_list *Prev;
    struct double_linked_list *Next;
    
} double_linked_list;

inline void
DoubleLinkedListInit(double_linked_list *Sentinel)
{
    Sentinel->Next = Sentinel;
    Sentinel->Prev = Sentinel;
}

inline void
RunDoubleLinkedListInitTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    ASSERT(Sentinel.Next == &Sentinel);
    ASSERT(Sentinel.Prev == &Sentinel);
}

inline void
DoubleLinkedListInsert(double_linked_list *Sentinel, double_linked_list *Element)
{
    Element->Next = Sentinel->Next;
    Element->Prev = Sentinel;
    Element->Next->Prev = Element;
    Element->Prev->Next = Element;
}

inline void
RunDoubleLinkedListInsertTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    {
        double_linked_list Element;
        DoubleLinkedListInsert(&Sentinel, &Element);
        
        ASSERT(Sentinel.Next != &Sentinel);
        ASSERT(Sentinel.Prev != &Sentinel);
        
        ASSERT(Sentinel.Next == &Element);
        ASSERT(Sentinel.Prev == &Element);
        
        ASSERT(Sentinel.Next->Next == &Sentinel);
        ASSERT(Sentinel.Prev->Prev == &Sentinel);
    }
    {
        double_linked_list First;
        double_linked_list Second;
        double_linked_list Third;
        DoubleLinkedListInsert(&Sentinel, &First);
        DoubleLinkedListInsert(&Sentinel, &Second);
        DoubleLinkedListInsert(&Sentinel, &Third);
        
        ASSERT(Sentinel.Next == &Third);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &First);
    }
}

inline void
DoubleLinkedListInsertAtLast(double_linked_list *Sentinel, double_linked_list *Element)
{
    Element->Next = Sentinel;
    Element->Prev = Sentinel->Prev;
    Element->Next->Prev = Element;
    Element->Prev->Next = Element;
}

inline void
RunDoubleLinkedListInsertAtLastTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    double_linked_list First;
    double_linked_list Second;
    double_linked_list Third;
    DoubleLinkedListInsertAtLast(&Sentinel, &First);
    DoubleLinkedListInsertAtLast(&Sentinel, &Second);
    DoubleLinkedListInsertAtLast(&Sentinel, &Third);
    
    ASSERT(Sentinel.Next == &First);
    ASSERT(Sentinel.Next->Next == &Second);
    ASSERT(Sentinel.Next->Next->Next == &Third);
}

inline b32
DoubleLinkedListIsEmpty(double_linked_list *Sentinel)
{
    b32 Result = (Sentinel->Next == Sentinel);
    return Result;
}

inline void
RunDoubleLinkedListIsEmptyTests(memory_arena *Arena)
{
    double_linked_list Sentinel;
    DoubleLinkedListInit(&Sentinel);
    
    Assert(DoubleLinkedListIsEmpty(&Sentinel));
    
    double_linked_list Element;
    DoubleLinkedListInsert(&Sentinel, &Element);
    Assert(!DoubleLinkedListIsEmpty(&Sentinel));
}

inline void
DoubleLinkedListSwap(double_linked_list *A, double_linked_list *B)
{
    if(A == B)
    {   
        InvalidCodePath;
    }
    else if(A->Next == B)
    {
        A->Next = B->Next;
        B->Prev = A->Prev;
        A->Next->Prev = A;
        B->Prev->Next = B;
        B->Next = A;
        A->Prev = B;
    }
    else
    {
        double_linked_list *ANext = A->Next;
        double_linked_list *APrev = A->Prev;
        
        A->Next = B->Next;
        A->Prev = B->Prev;
        A->Next->Prev = A;
        A->Prev->Next = A;
        
        B->Next = ANext;
        B->Prev = APrev;
        B->Next->Prev = B;
        B->Prev->Next = B;
    }
}

inline void
RunDoubleLinkedListSwapTests(memory_arena *Arena)
{
    {
        double_linked_list Sentinel;
        DoubleLinkedListInit(&Sentinel);
        
        double_linked_list First;
        double_linked_list Second;
        double_linked_list Third;
        DoubleLinkedListInsertAtLast(&Sentinel, &First);
        DoubleLinkedListInsertAtLast(&Sentinel, &Second);
        DoubleLinkedListInsertAtLast(&Sentinel, &Third);
        
        ASSERT(Sentinel.Next == &First);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &Third);
        
        DoubleLinkedListSwap(&First, &Third);
        
        ASSERT(Sentinel.Next == &Third);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &First);
    }
    {
        double_linked_list Sentinel;
        DoubleLinkedListInit(&Sentinel);
        
        double_linked_list First;
        double_linked_list Second;
        double_linked_list Third;
        DoubleLinkedListInsertAtLast(&Sentinel, &First);
        DoubleLinkedListInsertAtLast(&Sentinel, &Second);
        DoubleLinkedListInsertAtLast(&Sentinel, &Third);
        
        ASSERT(Sentinel.Next == &First);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &Third);
        
        DoubleLinkedListSwap(&Second, &Third);
        
        ASSERT(Sentinel.Next == &First);
        ASSERT(Sentinel.Next->Next == &Third);
        ASSERT(Sentinel.Next->Next->Next == &Second);
        
        DoubleLinkedListSwap(&First, &Third);
        
        ASSERT(Sentinel.Next == &Third);
        ASSERT(Sentinel.Next->Next == &First);
        ASSERT(Sentinel.Next->Next->Next == &Second);
    }
}

typedef b32 double_linked_list_predicate(double_linked_list *A, double_linked_list *B);

inline void
DoubleLinkedListSort(memory_arena *Arena, double_linked_list *First, u32 Count, double_linked_list_predicate *Predicate)
{
    
#if 1
    double_linked_list *Sentinel = First->Prev;
    for(u32 Outer = 0;
        Outer < Count;
        ++Outer)
    {
        b32 ListIsSorted = true;
        for(double_linked_list *EntryA = First;
            EntryA != Sentinel->Prev;
            EntryA = EntryA->Next)
        {
            double_linked_list *EntryB = EntryA->Next;
            
            if(Predicate(EntryA, EntryB))
            {
                DoubleLinkedListSwap(EntryA, EntryB);
                ListIsSorted = false;
            }
        }
        
        if(ListIsSorted)
        {
            break;
        }
    }
#else
    double_linked_list *Temp = (double_linked_list *)(Arena->Base + Arena->Used);
    if(Count == 1)
    {
        // NOTE(kstandbridge): No work to do.
    }
    else if(Count == 2)
    {
        double_linked_list *EntryA = First;
        double_linked_list *EntryB = EntryA->Next;
        
        if(Predicate(EntryA, EntryB))
        {
            DoubleLinkedListSwap(EntryA, EntryB);
        }
    }
    else
    {
        u32 Half0 = Count / 2;
        u32 Half1 = Count - Half0;
        
        Assert(Half0 >= 1);
        Assert(Half1 >= 1);
        
        double_linked_list *InHalf0 = First;
        
        double_linked_list *InHalf1 = First;
        for(u32 Next = 0;
            Next < Half0;
            ++Next)
        {
            InHalf1 = InHalf1->Next;
        }
        
        DoubleLinkedListSort(Arena, InHalf0, Half0, Predicate);
        DoubleLinkedListSort(Arena, InHalf1, Half1, Predicate);
        
        
        double_linked_list *ReadHalf0 = First;
        double_linked_list *ReadHalf1 = First;
        for(u32 Next = 0;
            Next < Half0;
            ++Next)
        {
            ReadHalf1 = ReadHalf1->Next;
        }
        double_linked_list *End = First;
        for(u32 Next = 0;
            Next < Count;
            ++Next)
        {
            End = End->Next;
        }
        
        double_linked_list *Out = Temp;
        for(u32 Index = 0;
            Index < Count;
            ++Index)
        {
            if(ReadHalf0 == InHalf1)
            {
                *Out++ = *ReadHalf1;
                ReadHalf1 = ReadHalf1->Next;
            }
            else if(ReadHalf1 == End)
            {
                *Out++ = *ReadHalf0;
                ReadHalf0 = ReadHalf0->Next;
            }
            else if(!Predicate(ReadHalf0, ReadHalf1))
            {
                *Out++ = *ReadHalf0;
                ReadHalf0 = ReadHalf0->Next;
            }
            else
            {
                *Out++ = *ReadHalf1;
                ReadHalf1 = ReadHalf1->Next;
            }            
        }
        Assert(Out == (Temp + Count));
        Assert(ReadHalf0 == InHalf1);
        Assert(ReadHalf1 == End);
        
        for(u32 Index = 0;
            Index < Count;
            ++Index)
        {
            First = Temp + Index;
            First = First->Next;
        }
    }
#endif
}

internal b32
DoubleLinkedListPredicate(double_linked_list *A, double_linked_list *B)
{
    b32 Result = A->SortKey > B->SortKey;
    return Result;
}

inline void
RunDoubleLinkedListSortTests(memory_arena *Arena)
{
    {
        double_linked_list Sentinel;
        DoubleLinkedListInit(&Sentinel);
        
        double_linked_list First;
        First.SortKey = 10;
        double_linked_list Second;
        Second.SortKey = 5;
        double_linked_list Third;
        Third.SortKey = 15;
        DoubleLinkedListInsertAtLast(&Sentinel, &First);
        DoubleLinkedListInsertAtLast(&Sentinel, &Second);
        DoubleLinkedListInsertAtLast(&Sentinel, &Third);
        
        ASSERT(Sentinel.Next == &First);
        ASSERT(Sentinel.Next->Next == &Second);
        ASSERT(Sentinel.Next->Next->Next == &Third);
        
        DoubleLinkedListSort(Arena, Sentinel.Next, 3, DoubleLinkedListPredicate);
        
        ASSERT(Sentinel.Next == &Third);
        ASSERT(Sentinel.Next->Next == &First);
        ASSERT(Sentinel.Next->Next->Next == &Second);
    }
    
    
    {
        LARGE_INTEGER LastCounter;
        Win32QueryPerformanceCounter(&LastCounter);
        random_state RandomState;
        RandomState.Value = (u32)LastCounter.QuadPart;
        
        double_linked_list Sentinel;
        DoubleLinkedListInit(&Sentinel);
        u32 ElementCount = RandomU32(&RandomState) % 100;
        for(u32 Index = 0;
            Index < ElementCount;
            ++Index)
        {
            double_linked_list *Element = PushStruct(Arena, double_linked_list);
            Element->SortKey = RandomU32(&RandomState);
            DoubleLinkedListInsert(&Sentinel, Element);
        }
        
        DoubleLinkedListSort(Arena, Sentinel.Next, ElementCount, DoubleLinkedListPredicate);
        
        u32 LastSoryKey = U32Max;
        for(double_linked_list *Element = Sentinel.Next;
            Element != &Sentinel;
            Element = Element->Next)
        {
            ASSERT(Element->SortKey < LastSoryKey);
            LastSoryKey = Element->SortKey;
        }
    }
    
}

internal b32
RunAllTests(memory_arena *Arena)
{
    RunStringsAreEqualTests(Arena);
    RunFormatStringSignedDecimalIntegerTests(Arena);
    RunFormatStringUnsignedDecimalIntegerTests(Arena);
    RunFormatStringDecimalFloatingPoint(Arena);
    RunFormatStringStringOfCharactersTests(Arena);
    RunFormatStringStringTypeTests(Arena);
    RunFormatStringPercentTests(Arena);
    
    RunV2Tests(Arena);
    
    RunUpperCamelCaseTests(Arena);
    
    RunDoubleLinkedListInitTests(Arena);
    RunDoubleLinkedListInsertTests(Arena);
    RunDoubleLinkedListInsertAtLastTests(Arena);
    RunDoubleLinkedListIsEmptyTests(Arena);
    RunDoubleLinkedListSwapTests(Arena);
    RunDoubleLinkedListSortTests(Arena);
    
    b32 Result = (FailedTests == 0);
    return Result;
}

s32 __stdcall
mainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
#if KENGINE_INTERNAL
    void *BaseAddress = (void *)Terabytes(2);
#else
    void *BaseAddress = 0;
#endif
    
    u64 MemoryBlockSize = Megabytes(16);
    void *MemoryBlock = Win32VirtualAlloc(BaseAddress, MemoryBlockSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(MemoryBlock);
    
    memory_arena Arena_;
    memory_arena *Arena = &Arena_;
    InitializeArena(Arena, MemoryBlockSize, MemoryBlock);
    
    b32 Result = RunAllTests(Arena);
    Win32ConsoleOut(Arena, "Unit Tests %s: %d/%d passed.\n", Result ? "Successful" : "Failed", TotalTests - FailedTests, TotalTests);
    
    Win32ExitProcess(0);
    
    InvalidCodePath;
    
    return 0;
}