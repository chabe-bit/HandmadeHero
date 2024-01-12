#include <Windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static

// Contain a variable to exit on its own
global_variable bool Running; // Global for now
global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory; // Memory that we receive from windows to draw into our renderer
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void
RenderSomething(int xOffset, int yOffset)
{
	int Width = BitmapWidth;
	int Height = BitmapHeight;
	// Change the void* BitmapMemory into something it does understand, treated as bytes to memory
	int Pitch = Width * BytesPerPixel; // The overall size of the pitch is the width, but subset sizes are the pixel : | | | | | | | | |
	uint8_t* Row = (uint8_t*)BitmapMemory;
	for (int Y = 0; Y < BitmapHeight; ++Y)
	{
		// Point each row in the bitmap as a pointer
		uint8_t* Pixel = (uint8_t*)BitmapMemory;
		for (int X = 0; X < BitmapWidth; ++X)
		{
			// Write into the Pixel by derefencing
			/*
				Because of the window architecture using little endian and using data coming out of 32 bits,
				RR GG BB xx being their original, would return as  xx BB GG RR, reversed! They didn't like that,
				so defined the memory read as backwards.

				Pixel in memory: 00 00 00 00

			*/

			*Pixel = (uint8_t)(X + xOffset);
			++Pixel;

			*Pixel = (uint8_t)(Y + yOffset);
			++Pixel;

			*Pixel = 0;
			++Pixel;

			*Pixel = 0;
			++Pixel;

		}
		Row += Pitch;
	}
}

// DIBSection, device independent bitmap, talk about the things you wrtie into its buffer to display using GDI
internal void
x64ResizeDBISection(int Width, int Height)
{
	// TODO(Ben): Bulletproof this
	// Maybe don't free first, free after, then free first

	if (BitmapMemory)
	{
		// We don't have to pass in the size of memory that we allocated into this function because it
		// remembers the amount we allocated, so we can pass a 0.
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);

		// MEM_DECOMMIT : 
		// Decommit and leaves the pages we've freed reserved because we might want to use them again.
	}

	BitmapWidth = Width;
	BitmapHeight = Height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	// How much memory do we want?
	// We requested for 32 bits, 8 bits R, 8 bits G, 8 bits B and 8 bits for Pad (unused and purpose is for alignment)
	int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;

	// Allocation
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	RenderSomething(0, 0);
}

internal void
x64UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height)
{
	int WindowWidth = ClientRect->right - ClientRect->left;
	int WindowHeight = ClientRect->bottom - ClientRect->top ;
	StretchDIBits(DeviceContext, 
		0, 0, BitmapWidth, BitmapHeight,
		0, 0, WindowWidth, WindowHeight,
		BitmapMemory,
		&BitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK
x64MainWindowCallback(HWND Window,
					UINT Message,
					WPARAM wParam,
					LPARAM lParam)
{
	LRESULT Result = 0;


	switch (Message)
	{
		case WM_SIZE:	 // User change window size
		{
			// Every time we resize our window, this function is called to correct our display to fit the window
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			// Find the W and H for the buffer in our window 
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			x64ResizeDBISection(Width, Height);
			OutputDebugStringA("WN_SIZE\n");
			break;
		}
		case WM_DESTROY: // Window deletes window
		{
			// Handle as an error, recreate window
			Running = false;
			break;
		}
		case WM_CLOSE:   // User closes the window
		{
			// Handle with a message to the user
			Running = false;
			break;
		}
		case WM_ACTIVATEAPP: // User clicks app to be used
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
			break;
		}
		case WM_PAINT: // User clicks app to be used
		{
			// Prepares the window for painting
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			x64UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);

			EndPaint(Window, &Paint);
			break;
		}
		default: // User closes the window
		{
			Result = DefWindowProcA(Window, Message, wParam, lParam); // Catch all messages we missed
			break;
		}
	}

	return Result;
}

int CALLBACK
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR     CmdLine,
		int       ShowCmd)
{
	WNDCLASSA WindowClass = {};

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // Set of binary flags
	WindowClass.lpfnWndProc = x64MainWindowCallback; // Pointer to a function 
	WindowClass.hInstance = Instance;   // Determines what setting we set our window to
	// WindowClass     hIcon;       
	WindowClass.lpszClassName = "HandmadeHeroWindowClass"; // Name for our window 

	// Registering the window class
	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(
						0,
						WindowClass.lpszClassName,
						"Handmade Hero",
						WS_OVERLAPPEDWINDOW | WS_VISIBLE,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						0,
						0,
						Instance,
						0);
		if (Window)
		{
			// Start a message loop
			// A queue of message are created for you by the window
			// Use GetMessage() to dispatch incoming messages
			int xOffset = 0;
			int yOffset = 0;
			Running = true;
			while (Running)
			{
				MSG Message;

				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					BOOL MessageResult = (GetMessage(&Message, 0, 0, 0));
					if (MessageResult > 0) // If it doesn't exit 
					{
						// TranslateMessage(&Message); // Processes messages that comes in
						DispatchMessage(&Message);
					}

				}
				RenderSomething(xOffset, yOffset);
				HDC DeviceContext = GetDC(Window);

				RECT ClientRect;
				GetClientRect(Window, &ClientRect);

				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				x64UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
				ReleaseDC(Window, DeviceContext);

				xOffset++;
			}
		}
		else
		{

		}
	} 
	else
	{

	}

	return 0;
}



