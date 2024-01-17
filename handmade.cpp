#include "include/handmade.h"

internal
void RenderSomething(game_offscreen_buffer* Buffer, int xOffset, int yOffset)
{
	int Width = Buffer->Width;
	int Height = Buffer->Height;

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

			*Pixel = (U8)(X + xOffset);
			++Pixel;

			*Pixel = (U8)(Y + yOffset);
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
GameUpdateAndRender(game_offscreen_buffer Buffer)
{
	int BlueOffset = 0;
	int GreenOffset = 0;
	RenderSomething(&Buffer, BlueOffset, GreenOffset);
}
