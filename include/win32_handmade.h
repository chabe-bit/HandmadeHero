#pragma once
#include "common.h"
#include <wingdi.h>

struct win32_offscreen_buffer
{
	BITMAPINFO BitmapInfo;
	void* BitmapMemory; // Memory that we receive from windows to draw into our renderer
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_sound_output
{
	// Sound test
	i32 SamplesPerSecond;
	u32 RunningSampleIndex;
	int BytesPerSample;
	int SecondaryBufferSize;
	f32 tSine;
	int LatencySampleCount;
};