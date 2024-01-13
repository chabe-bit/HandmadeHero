#include <Windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static

struct win32_offscreen_buffer
{
	BITMAPINFO BitmapInfo;
	void* BitmapMemory; // Memory that we receive from windows to draw into our renderer
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};


// Contain a variable to exit on its own
global_variable bool Running; // Global for now
global_variable win32_offscreen_buffer GlobalBackBuffer;


internal void
RenderSomething(win32_offscreen_buffer Buffer, int xOffset, int yOffset)
{
	int Width = Buffer.Width;
	int Height = Buffer.Height;

	// Change the void* BitmapMemory into something it does understand, treated as bytes to memory
	uint8_t* Row = (uint8_t*)Buffer.BitmapMemory;
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		// Point each row in the bitmap as a pointer
		uint8_t* Pixel = (uint8_t*)Row;
		for (int X = 0; X < Buffer.Width; ++X)
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
		Row += Buffer.Pitch;
	}
}

// DIBSection, device independent bitmap, talk about the things you wrtie into its buffer to display using GDI
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	// TODO(Ben): Bulletproof this
	// Maybe don't free first, free after, then free first

	if (Buffer->BitmapMemory)
	{
		// We don't have to pass in the size of memory that we allocated into this function because it
		// remembers the amount we allocated, so we can pass a 0.
		VirtualFree(Buffer->BitmapMemory, 0, MEM_RELEASE);

		// MEM_DECOMMIT : 
		// Decommit and leaves the pages we've freed reserved because we might want to use them again.
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->BitmapInfo.bmiHeader.biSize = sizeof(Buffer->BitmapInfo.bmiHeader);
	Buffer->BitmapInfo.bmiHeader.biWidth = Buffer->Width;
	Buffer->BitmapInfo.bmiHeader.biHeight = -Buffer->Height;
	Buffer->BitmapInfo.bmiHeader.biPlanes = 1;
	Buffer->BitmapInfo.bmiHeader.biBitCount = 32;
	Buffer->BitmapInfo.bmiHeader.biCompression = BI_RGB;

	// How much memory do we want?
	// We requested for 32 bits, 8 bits R, 8 bits G, 8 bits B and 8 bits for Pad (unused and purpose is for alignment)
	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * Buffer->BytesPerPixel; // The overall size of the pitch is the width, but subset sizes are the pixel : | | | | | | | | |

	
}
 
internal void
Win32DisplayBufferToWindow(HDC DeviceContext, RECT ClientRect, 
							win32_offscreen_buffer Buffer,
							int X, int Y, int Width, int Height)
{ 
	int WindowWidth = ClientRect.right - ClientRect.left;
	int WindowHeight = ClientRect.bottom - ClientRect.top ;
	StretchDIBits(DeviceContext, 
		0, 0, Buffer.Width, Buffer.Height,
		0, 0, WindowWidth, WindowHeight,
		Buffer.BitmapMemory,
		&Buffer.BitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
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
			Win32ResizeDIBSection(&GlobalBackBuffer, Width, Height);
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

			Win32DisplayBufferToWindow(DeviceContext, ClientRect, GlobalBackBuffer, X, Y, Width, Height);

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

	WindowClass.style = CS_HREDRAW | CS_VREDRAW; // Set of binary flags
	WindowClass.lpfnWndProc = Win32MainWindowCallback; // Pointer to a function 
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
					if (Message.message == WM_QUIT)
					{
						Running = false;
					}
					
					TranslateMessage(&Message); // Processes messages that comes in
					DispatchMessage(&Message);
				
				}
				RenderSomething(GlobalBackBuffer, xOffset, yOffset);
				HDC DeviceContext = GetDC(Window);

				RECT ClientRect;
				GetClientRect(Window, &ClientRect);

				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				Win32DisplayBufferToWindow(DeviceContext, ClientRect, GlobalBackBuffer, 0, 0, WindowWidth, WindowHeight);
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



