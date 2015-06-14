#ifndef GAME_INTRINSICS_H
#define GAME_INTRINSICS_H

#include "math.h"

inline int32
SignOf(int32 Value)
{
	int32 Result = (Value >= 0) ? 1 : -1;
	return Result;
}

inline float32
SquareRoot(float32 FloatValue)
{
	float32 Result = sqrtf(FloatValue);
	return Result;
}

inline float32
AbsoluteValue(float32 FloatValue)
{
	float32 Result = fabs(FloatValue);
	return Result;
}

inline uint32
RotateLeft(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
	uint32 Result = _rotl(Value, Amount);
#else
	Amout &= 31;
	uint32 Result = ((Value << Amount) | (Value >> ( 32 - Amount)));
#endif
	return Result;
}

inline uint32
RotateRight(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
	uint32 Result = _rotr(Value, Amount);
#else
	Amout &= 31;
	uint32 Result = ((Value >> Amount) | (Value << ( 32 - Amount)));
#endif
	return Result;
}

inline int32
RoundFloat32ToInt32(float32 FloatValue)
{
	int32 Result = (int32)roundf(FloatValue);
	return Result;
}

inline uint32
RoundFloat32ToUInt32(float32 FloatValue)
{
	uint32 Result = (uint32)roundf(FloatValue);
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

inline int32
CeilFloat32ToInt32(float32 FloatValue)
{
    int32 Result = (int32)ceilf(FloatValue);
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

struct bit_scan_result
{
	bool32 Found;
	uint32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(uint32 Value)
{
	bit_scan_result Result = {};

#if COMPILER_MSVC
	Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
	for(uint32 Test = 0; Test < 32; ++Test)
	{
		if(Value & (1 << Test))
		{
			Result.Index = Test;
			Result.Found = true;
			break;
		}
	}
#endif

	return Result;
}


#endif