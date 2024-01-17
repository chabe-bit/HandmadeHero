#pragma once

#include "common.h"

#ifndef HANDMADE_H
#define HANDMADE_H	

// RenderSomething(GlobalBackBuffer, xOffset, yOffset);
struct game_offscreen_buffer
{
	//BITMAPINFO BitmapInfo;
	void* BitmapMemory; // Memory that we receive from windows to draw into our renderer
	int Width;
	int Height;
	int Pitch;
};

internal 
void GameUpdateAndRender(game_offscreen_buffer Buffer);



#endif