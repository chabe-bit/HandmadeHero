#include "include/handmade.h"

internal
void GameOutputSound(game_sound_output_buffer* SoundBuffer)
{
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		local_persist F32 tSine;
		I16 ToneVolume = 3000;
		int ToneHz = 256;
		int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

		I16* SampleOut = SoundBuffer->Samples; // Store the pointer for the sound output
		
		// Computer a value that'll be a sine wave
		// If you're working with math heavy code, you'll always be working with floats
		// we need to think where we are in the sine wave
		
		F32 SineValue = sinf(tSine);
		I16 SampleValue = (I16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		++SoundBuffer->RunningSampleIndex;

		tSine = 2.0f * PI * 1.0f / (F32)WavePeriod; // The divide that tells us where we are, then multiplky by the cycle of our wave

	}
}

internal
void Renderer(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{

	// Change the void* BitmapMemory into something it does understand, treated as bytes to memory
	U8* Row = (U8*)Buffer->BitmapMemory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		// Point each row in the bitmap as a pointer
		U8* Pixel = (U8*)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			// Write into the Pixel by derefencing
			/*
				Because of the window architecture using little endian and using data coming out of 32 bits,
				RR GG BB xx being their original, would return as  xx BB GG RR, reversed! They didn't like that,
				so defined the memory read as backwards.

				Pixel in memory: 00 00 00 00

			*/

			*Pixel = (U8)(X + BlueOffset);
			++Pixel;

			*Pixel = (U8)(Y + GreenOffset);
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
