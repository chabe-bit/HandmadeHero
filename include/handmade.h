#pragma once

#include "common.h"

#ifndef HANDMADE_H
#define HANDMADE_H	

struct game_offscreen_buffer
{
	//BITMAPINFO BitmapInfo;
	void* BitmapMemory; // Memory that we receive from windows to draw into our renderer
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	i16* Samples; 
};

internal
void GameUpdateAndRender(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset,
					game_sound_output_buffer* SoundBuffer);
internal
void GameOutputSound(game_sound_output_buffer* SoundBuffer);


#endif