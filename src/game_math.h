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

#endif