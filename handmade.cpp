#include "include/handmade.h"

internal
void GameOutputSound(game_sound_output_buffer* SoundBuffer)
{
	local_persist f32 tSine;
	i16 ToneVolume = 3000;
	int ToneHz = 256;
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
GameUpdateAndRender(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset,
					game_sound_output_buffer* SoundBuffer)
{
	// What do we need? What do we start with?
	// Where in time do we want these samples to be, so that if we're in trouble we can skip forward
	
	// TODO(Ben): Allow sample offsets here for more robust platform options
	GameOutputSound(SoundBuffer);
	Renderer(Buffer, BlueOffset, GreenOffset);
}
