#pragma once

#include "common.h"

#ifndef HANDMADE_H
#define HANDMADE_H	

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array[0]))

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

struct game_button_state
{
	int HalfTransitionCount;
	bool EndedDown;
};

struct game_controller_input
{
	bool IsAnalog;

	f32 StartX;
	f32 StartY;

	f32 MinX;
	f32 MinY;

	f32 MaxX;
	f32 MaxY;

	f32 EndX;
	f32 EndY;

	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};
};

struct game_input
{
	game_controller_input Controllers[4];
};


internal
void GameUpdateAndRender(game_input* Input,
						game_offscreen_buffer* Buffer,
						game_sound_output_buffer* SoundBuffer);
internal
void GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz);


#endif