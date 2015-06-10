#ifndef GAME_MATH_H
#define GAME_MATH_H

struct v2
{
	float32 X, Y;
	float32 &operator[](int Index) { return( (&X)[Index] ); }

	inline v2 &operator*=(float32 A);
	inline v2 &operator+=(v2 A);
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

inline v2 &v2::
operator+=(v2 A)
{
	*this = *this + A;

	return *this;
}


inline v2
operator-(v2 A, v2 B)
{
	v2 Result;

	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;

	return Result;
}

inline v2
operator*(float32 A, v2 B)
{
	v2 Result;

	Result.X = A*B.X;
	Result.Y = A*B.Y;

	return Result;
}

inline v2 &v2::
operator*=(float32 A)
{
	*this = A * *this;

	return *this;
}

#endif