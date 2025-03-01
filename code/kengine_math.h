#ifndef KENGINE_MATH_H

//
// NOTE(kstandbridge): Scalar operations
//

inline u32
F32ToRadixValue(f32 Value)
{
    u32 Result = *(u32 *)&Value;
    if(Result & 0x80000000)
    {
        Result = ~Result;
    }
    else
    {
        Result |= 0x80000000;
    }
    return(Result);
}

inline f32
SafeRatioN(f32 Numerator, f32 Divisor, f32 N)
{
    f32 Result = N;
    
    if(Divisor != 0.0f)
    {
        Result = Numerator / Divisor;
    }
    
    return Result;
}

inline f32
SafeRatio0(f32 Numerator, f32 Divisor)
{
    f32 Result = SafeRatioN(Numerator, Divisor, 0.0f);
    return Result;
}

inline f32
SafeRatio1(f32 Numerator, f32 Divisor)
{
    f32 Result = SafeRatioN(Numerator, Divisor, 1.0f);
    return Result;
}
inline u32
SafeTruncateU64ToU32(u64 Value)
{
    Assert(Value <= U32Max);
    u32 Result = (u32)Value;
    return(Result);
}

inline u16
SafeTruncateU32ToU16(u32 Value)
{
    Assert(Value <= U16Max);
    u16 Result = (u16)Value;
    return(Result);
}

inline f32
Square(f32 A)
{
    f32 Result = A*A;
    return Result;
}

//
// NOTE(kstandbridge): v2 operations
//

inline f32
Inner(v2 A, v2 B)
{
    f32 Result = A.X*B.X + A.Y*B.Y;
    return Result;
}

inline f32
LengthSq(v2 A)
{
    f32 Result = Inner(A, A);
    return Result;
}

inline f32
LengthV2(v2 A)
{
    f32 Result = SquareRoot(LengthSq(A));
    return Result;
}

inline v2
Hadamard(v2 A, v2 B)
{
    v2 Result = V2(A.X*B.X, A.Y*B.Y);
    return Result;
}

inline f32
V2DistanceBetween(v2 A, v2 B)
{
    f32 Result = SquareRoot((A.X - B.X)*(A.X - B.X) + (A.Y - B.Y)*(A.Y - B.Y));
    return Result;
}

//
// NOTE(kstandbridge): v2i operations
//

inline f32
V2iDistanceBetween(v2i A, v2i B)
{
    f32 Result = V2DistanceBetween(V2((f32)A.X, (f32)A.Y), V2((f32)B.X, (f32)B.Y));
    return Result;
}

//
// NOTE(kstandbridge): v4 operations
//

inline v4
RGBColor(f32 R, f32 G, f32 B, f32 A)
{
    v4 Result;
    
    Result.R = R/255.0f;
    Result.G = G/255.0f;
    Result.B = B/255.0f;
    Result.A = A/255.0f;
    
    return Result;
}

inline v4
SRGB255ToLinear1(v4 C)
{
    v4 Result;
    
    f32 Inv255 = 1.0f / 255.0f;
    
    Result.R = Square(Inv255*C.R);
    Result.G = Square(Inv255*C.G);
    Result.B = Square(Inv255*C.B);
    Result.A = Inv255*C.A;
    
    return Result;
}

inline v4
Linear1ToSRGB255(v4 C)
{
    v4 Result;
    
    f32 One255 = 255.0f;
    
    Result.R = One255*SquareRoot(C.R);
    Result.G = One255*SquareRoot(C.G);
    Result.B = One255*SquareRoot(C.B);
    Result.A = One255*C.A;
    
    return Result;
}

//
// NOTE(kstandbridge): rectangle2 operations
//

inline rectangle2
Rectangle2AddRadiusTo(rectangle2 Rectangle, f32 Radius)
{
    rectangle2 Result;
    
    Result.Min = V2Subtract(Rectangle.Min, V2Set1(Radius));
    Result.Max = V2Add(Rectangle.Max, V2Set1(Radius));
    
    return Result;
}

inline rectangle2
Rectangle2InvertedInfinity()
{
    rectangle2 Result;
    
    Result.Min.X = Result.Min.Y = F32Max;
    Result.Max.X = Result.Max.Y = -F32Max;
    
    return Result;
}

inline rectangle2
Rectangle2MinDim(v2 Min, v2 Dim)
{
    rectangle2 Result;
    
    Result.Min = Min;
    Result.Max = V2Add(Min, Dim);
    
    return(Result);
}


inline rectangle2
Rectangle2Union(rectangle2 A, rectangle2 B)
{
    rectangle2 Result;
    
    Result.Min.X = (A.Min.X < B.Min.X) ? A.Min.X : B.Min.X;
    Result.Min.Y = (A.Min.Y < B.Min.Y) ? A.Min.Y : B.Min.Y;
    Result.Max.X = (A.Max.X > B.Max.X) ? A.Max.X : B.Max.X;
    Result.Max.Y = (A.Max.Y > B.Max.Y) ? A.Max.Y : B.Max.Y;
    
    return Result;
}

inline b32
Rectangle2IsIn(rectangle2 Rectangle, v2 Test)
{
    b32 Result = ((Test.X >= Rectangle.Min.X) &&
                  (Test.Y >= Rectangle.Min.Y) &&
                  (Test.X < Rectangle.Max.X) &&
                  (Test.Y < Rectangle.Max.Y));
    
    return Result;
}

inline v2
Rectangle2GetDim(rectangle2 Rectangle)
{
    v2 Result = V2Subtract(Rectangle.Max, Rectangle.Min);
    return Result;
}

//
// NOTE(kstandbridge): rectangle2i operations
//

inline rectangle2i
InvertedInfinityRectangle2i()
{
    rectangle2i Result;
    
    Result.MinX = Result.MinY = S32Max;
    Result.MaxX = Result.MaxY = S32Min;
    
    return Result;
}

inline rectangle2i
Intersect(rectangle2i A, rectangle2i B)
{
    rectangle2i Result;
    
    Result.MinX = (A.MinX < B.MinX) ? B.MinX : A.MinX;
    Result.MinY = (A.MinY < B.MinY) ? B.MinY : A.MinY;
    Result.MaxX = (A.MaxX > B.MaxX) ? B.MaxX : A.MaxX;
    Result.MaxY = (A.MaxY > B.MaxY) ? B.MaxY : A.MaxY;    
    
    return(Result);
}


inline b32
HasArea(rectangle2i A)
{
    b32 Result = ((A.MinX < A.MaxX) && (A.MinY < A.MaxY));
    
    return Result;
}

#define KENGINE_MATH_H
#endif //KENGINE_MATH_H
