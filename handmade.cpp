#include "include/handmade.h"

internal
void GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{
	local_persist f32 tSine;
	i16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	i16* SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; 
		SampleIndex < SoundBuffer->SampleCount;
		++SampleIndex)
	{
		f32 SineValue = sinf(tSine);
		i16 SampleValue = (i16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f * PI * 1.0f / (f32)WavePeriod; // The divide that tells us where we are, then multiplky by the cycle of our wave

	}
}

internal
void Renderer(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{

	// Change the void* BitmapMemory into something it does understand, treated as bytes to memory
	u8* Row = (u8*)Buffer->BitmapMemory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		// Point each row in the bitmap as a pointer
		u8* Pixel = (u8*)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			// Write into the Pixel by derefencing
			/*
				Because of the window architecture using little endian and using data coming out of 32 bits,
				RR GG BB xx being their original, would return as  xx BB GG RR, reversed! They didn't like that,
				so defined the memory read as backwards.

				Pixel in memory: 00 00 00 00

			*/

			*Pixel = (u8)(X + BlueOffset);
			++Pixel;

			*Pixel = (u8)(Y + GreenOffset);
			++Pixel;

			*Pixel = 0;
			++Pixel;

			*Pixel = 0;
			++Pixel;


		}
		Row += Buffer->Pitch;
	}
}

internal void
GameUpdateAndRender(game_input* Input, game_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer)
{
	local_persist int BlueOffset = 0;
	local_persist int GreenOffset = 0;
	local_persist int ToneHz = 256;

	// Possible to make a fighting game? 
	game_controller_input* Input0 = &Input->Controllers[0];
	if (Input0->IsAnalog)
	{ 
		ToneHz = 256 + (int)(128.0f * (Input0->EndX));
		BlueOffset += (int)4.0f * (Input0->EndY);
	}
	else
	{

	}

	//Input.SKeyEndedDown;
	//Input.SKeyHalfTransitionCount;
	if (Input0->Down.EndedDown)
	{
		GreenOffset += 1;
	}
	
	GameOutputSound(SoundBuffer, ToneHz);
	Renderer(Buffer, BlueOffset, GreenOffset);
}
