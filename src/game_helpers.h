#ifndef GAME_HELPERS_H
#define GAME_HELPERS_H

#include "math.h"

inline int32
RoundFloat32ToInt32(float32 FloatValue)
{
	int32 Result = (int32)(FloatValue + 0.5f);
	return Result;
}

inline uint32
RoundFloat32ToUInt32(float32 FloatValue)
{
	uint32 Result = (uint32)(FloatValue + 0.5f);
	return Result;
}

inline int32
TruncateFloat32ToInt32(float32 FloatValue)
{
	int32 Result = (uint32)(FloatValue);
	return Result;
}

inline int32
FloorFloat32ToInt32(float32 FloatValue)
{
	int32 Result = (uint32)floorf(FloatValue);
	return Result;
}

inline float32 
Sin(float32 Angle)
{
	float32 Result = sinf(Angle);
	return Result;
}

inline float32 
Cos(float32 Angle)
{
	float32 Result = cosf(Angle);
	return Result;
}

inline float32 
ATan2(float32 Y, float32 X)
{
	float32 Result = atan2f(Y, X);
	return Result;
}

#endif