#ifndef GAME_MATH_H
#define GAME_MATH_H

union v2
{
	struct
	{
		float32 X;
		float32 Y;
	};
	float32 E[2];
};

inline v2
V2(float32 X, float32 Y)
{
	v2 Result;

	Result.X = X;
	Result.Y = Y;

	return Result;
}

inline v2
operator*(float32 B, v2 A)
{
	v2 Result;

	Result.X = B * A.X;
	Result.Y = B * A.Y;

	return Result;
}

inline v2
operator*(v2 A, float32 B)
{
	v2 Result;

	Result.X = B * A.X;
	Result.Y = B * A.Y;

	return Result;
}

inline v2 &
operator*=(v2 &A, float32 B)
{
	A = B * A;

	return A;
}

inline v2
operator-(v2 A)
{
	v2 Result;

	Result.X = -A.X;
	Result.Y = -A.Y;

	return Result;
}

inline v2
operator+(v2 A, v2 B)
{
	v2 Result;

	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;

	return Result;
}

inline v2 &
operator+=(v2 &A, v2 B)
{
	A = A + B;

	return A;
}

inline v2
operator-(v2 A, v2 B)
{
	v2 Result;

	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;

	return Result;
}

inline float32
Square(float32 A)
{
	float32 Result = A * A;

	return Result;
}

inline float32
Inner(v2 A, v2 B)
{
	float32 Result = A.X * B.X + A.Y * B.Y;

	return Result;
}

inline float32
LengthSq(v2 A)
{
	float32 Result = Inner(A,A);

	return Result;
}

struct rectangle2
{
	v2 Min;
	v2 Max;
};

inline rectangle2
RectMinMax(v2 Min, v2 Max)
{
	rectangle2 Result;

	Result.Min = Min;
	Result.Max = Max;

	return Result;
}

inline rectangle2
RectMinDim(v2 Min, v2 Dim)
{
	rectangle2 Result;

	Result.Min = Min;
	Result.Max = Min + Dim;

	return Result;
}

inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim)
{
	rectangle2 Result;

	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;

	return Result;
}

inline bool32
IsInRectangle(rectangle2 Rectangle, v2 Test)
{
	bool32 Result = ( (Test.X >= Rectangle.Min.X) &&
					  (Test.Y >= Rectangle.Min.Y) &&
					  (Test.X < Rectangle.Max.X) &&
					  (Test.Y < Rectangle.Max.Y) );

	return Result;
}

#endif