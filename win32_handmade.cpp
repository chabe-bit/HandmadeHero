#include <Windows.h>
#include <intrin.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#include "include/common.h"
#include "handmade.cpp"



global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable bool SoundIsPlaying;



////////////////////////////////////////////////
// ---------- STRUCTS --------------------------
////////////////////////////////////////////////
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
	int ToneHz; // Actual value is ~261
	i16 ToneVolume;
	u32 RunningSampleIndex;
	int WavePeriod; // Samples per chunk!
	int HalfWavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	int LatencySampleCount;
};

////////////////////////////////////////////////
// ------------ XInputGetState -----------------
////////////////////////////////////////////////
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define XInputGetState XInputGetState_;
typedef X_INPUT_GET_STATE(_x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable _x_input_get_state* XInputGetState_ = XInputGetStateStub;

////////////////////////////////////////////////
// ------------ XInputSetState -----------------
////////////////////////////////////////////////
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
#define XInputSetState XInputSetState_;
typedef X_INPUT_SET_STATE(_x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable _x_input_set_state* XInputSetState_ = XInputSetStateStub;

////////////////////////////////////////////////
// ------------ Create Sound -----------------
////////////////////////////////////////////////
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter );
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void
Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
	if (XInputLibrary)
	{
		XInputGetState_ = (_x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState_ = (_x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");

	}
}

internal void
Win32InitDSound(HWND Window, i32 SamplePerSecond, i32 BufferSize)
{
	// 1. Load library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	// 2. Give the option to function with or without sound
	if (DSoundLibrary)
	{
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			// Set format of primary buffer
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplePerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER; // Set to primary buffer
				// LPCDSBUFFERDESC lpcDSBufferDesc;
				// IDirectSound8_CreateSoundBuffer();
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				OutputDebugStringA("We made it!\n");
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					// Failing to set format...
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						OutputDebugStringA("Primary buffer created successfully!\n");

					}
					else
					{
						OutputDebugStringA("Primary buffer failed!\n");

					}
				}
				else
				{
					OutputDebugStringA("Sound isn't being created! 123\n");
				}
			}
			else
			{
				OutputDebugStringA("Sound isn't being created! 456\n");
			}
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0; // Set to primary buffer
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{
				OutputDebugStringA("Secondary buffer created successfully!");
			}
			else
			{
				OutputDebugStringA("Secondary buffer failed!");
			}
		}
		else
		{

		}
	}
	else
	{

	}

}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

// Contain a variable to exit on its own
global_variable bool Running; // Global for now
global_variable win32_offscreen_buffer GlobalBackBuffer;


internal void
RenderSomething(win32_offscreen_buffer Buffer, int xOffset, int yOffset)
{
	int Width = Buffer.Width;
	int Height = Buffer.Height;

	// Change the void* BitmapMemory into something it does understand, treated as bytes to memory
	u8* Row = (u8*)Buffer.BitmapMemory;
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		// Point each row in the bitmap as a pointer
		u8* Pixel = (u8*)Row;
		for (int X = 0; X < Buffer.Width; ++X)
		{
			// Write into the Pixel by derefencing
			/*
				Because of the window architecture using little endian and using data coming out of 32 bits,
				RR GG BB xx being their original, would return as  xx BB GG RR, reversed! They didn't like that,
				so defined the memory read as backwards.

				Pixel in memory: 00 00 00 00

			*/

			*Pixel = (u8)(X + xOffset);
			++Pixel;

			*Pixel = (u8)(Y + yOffset);
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
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
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
Win32DisplayBufferToWindow(HDC DeviceContext, int WindowWidth, int WindowHeight,
	win32_offscreen_buffer Buffer,
	int X, int Y, int Width, int Height)
{
	// (TODO): Correct the aspect ratio when we do this split
	StretchDIBits(DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer.Width, Buffer.Height,
		Buffer.BitmapMemory,
		&Buffer.BitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY);
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam)
{
	LRESULT Result = 0;


	switch (Message)
	{
	case WM_SIZE:	 // User change window size
	{

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
	case WM_SYSKEYUP:
	{
		break;
	}
	case WM_SYSKEYDOWN:
	{
		break;

	}
	case WM_KEYUP:
	{
		break;

	}
	case WM_KEYDOWN:
	{
		i32 VKCode = WParam;
		// Check to see if a key WAS down
		bool WasDown = ((LParam & (1 << 30)) != 0); // Shift 30 bits up (or left)
		bool IsDown = ((LParam & (1 << 31)) == 0); // We want it down!

		if (WasDown != IsDown)
		{
			if (VKCode == 'W')
			{
				OutputDebugStringA("W key pressed!\n");
			}
			else if (VKCode == 'A')
			{

			}
			else if (VKCode == 'S')
			{

			}
			else if (VKCode == 'D')
			{

			}
			else if (VKCode == 'Q')
			{

			}
			else if (VKCode == 'E')
			{

			}
			else if (VKCode == VK_UP)
			{

			}
			else if (VKCode == VK_LEFT)
			{

			}
			else if (VKCode == VK_RIGHT)
			{

			}
			else if (VKCode == VK_DOWN)
			{

			}
			else if (VKCode == VK_ESCAPE)
			{


			}
			else if (VKCode == VK_SPACE)
			{

			}
		}


		i32 AltKeyWasDown = (LParam & (1 << 29));
		if ((VKCode == VK_F4) && AltKeyWasDown)
		{
			Running = false;
		}


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

		win32_window_dimension Dimension = Win32GetWindowDimension(Window);

		Win32DisplayBufferToWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, X, Y, Width, Height);

		EndPaint(Window, &Paint);
		break;
	}
	default: // User closes the window
	{
		Result = DefWindowProcA(Window, Message, WParam, LParam); // Catch all messages we missed
		break;
	}
	}

	return Result;
}

internal void
Win32ClearSoundBuffer(win32_sound_output* SoundOutput)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		u8* DestSample = (u8*)Region1;
		for (DWORD ByteIndex = 0;
			ByteIndex < Region1Size;
			++ByteIndex)
		{
			*DestSample++ = 0;
		}

		DestSample = (u8*)Region2;
		for (DWORD ByteIndex = 0;
			ByteIndex < Region2Size;
			++ByteIndex)
		{
			*DestSample++ = 0;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void
Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
	game_sound_output_buffer* SoundBuffer)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample; // Get the region1 sample counts
		i16* DestSample = (i16*)Region1;
		i16* SourceSample = SoundBuffer->Samples;
		for (DWORD SampleIndex = 0;
			SampleIndex < Region1SampleCount;
			++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample; // Get the region2 sample counts
		DestSample = (i16*)Region2;
		for (DWORD SampleIndex = 0;
			SampleIndex < Region2SampleCount;
			++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, 
								game_button_state* OldState, DWORD ButtonBit,
								game_button_state* NewState)
{
	// OldState keeps track of previous input
	// NewState sets the current input 
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount =
		(OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

int CALLBACK
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR     CmdLine,
	int       ShowCmd)
{
	LARGE_INTEGER QueryPerfCountFrequencyResult; // Only called once in the program, at the start of initialization
	QueryPerformanceFrequency(&QueryPerfCountFrequencyResult);
	i64 PerfCountFrequency = QueryPerfCountFrequencyResult.QuadPart;

	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	// Every time we resize our window, this function is called to correct our display to fit the window
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // Set of binary flags
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
			HDC DeviceContext = GetDC(Window);
			// Start a message loop
			// A queue of message are created for you by the window
			// Use GetMessage() to dispatch incoming messages

			win32_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			i16* Samples = (i16*)VirtualAlloc(0, 48000 * 2 * sizeof(16), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			
			game_input Input[2] = {};
			game_input* NewInput = &Input[0];
			game_input* OldInput = &Input[1];

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			i64 LastCycleCount = __rdtsc();
			Running = true;
			while (Running)
			{
				LARGE_INTEGER BeginCounter;
				BeginCounter.QuadPart; // 64 bit value, clock value that corresponds to our clock time, timestamp that the time we start
				QueryPerformanceCounter(&BeginCounter);
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

				// This is where we taking user input
				int MaxControllerCount = XUSER_MAX_COUNT;
				if (MaxControllerCount > ArrayCount(NewInput->Controllers))
				{
					MaxControllerCount = ArrayCount(NewInput->Controllers);
				}
				for (DWORD ControllerIndex = 0; 
					ControllerIndex < MaxControllerCount; 
					++ControllerIndex)
				{

					__debugbreak;
					game_controller_input* OldController = &OldInput->Controllers[ControllerIndex];
					game_controller_input* NewController = &NewInput->Controllers[ControllerIndex];

					XINPUT_STATE ControllerState;
					if (XInputGetState_(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// This means the controller is plugged in.
						XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

						bool Up = (Pad->wButtons) & XINPUT_GAMEPAD_DPAD_UP;
						bool Down = (Pad->wButtons) & XINPUT_GAMEPAD_DPAD_DOWN;
						bool Left = (Pad->wButtons) & XINPUT_GAMEPAD_DPAD_LEFT;
						bool Right = (Pad->wButtons) & XINPUT_GAMEPAD_DPAD_RIGHT;
						i16 StickX = Pad->sThumbLX;
						i16 StickY = Pad->sThumbLY;
						 

						Win32ProcessXInputDigitalButton(Pad->wButtons,
							&OldController->Down, XINPUT_GAMEPAD_A,
							&NewController->Down);
						Win32ProcessXInputDigitalButton(Pad->wButtons,
							&OldController->Right, XINPUT_GAMEPAD_B,
							&NewController->Right);
						Win32ProcessXInputDigitalButton(Pad->wButtons,
							&OldController->Left, XINPUT_GAMEPAD_X,
							&NewController->Left);
						Win32ProcessXInputDigitalButton(Pad->wButtons,
							&OldController->Up, XINPUT_GAMEPAD_Y,
							&NewController->Up);
						Win32ProcessXInputDigitalButton(Pad->wButtons,
							&OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
							&NewController->LeftShoulder);
						Win32ProcessXInputDigitalButton(Pad->wButtons,
							&OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
							&NewController->RightShoulder);


					/*	bool Start = (Pad->wButtons) & XINPUT_GAMEPAD_START;
						bool Back = (Pad->wButtons) & XINPUT_GAMEPAD_BACK;*/
					}
					else
					{
						// This means a the controller is not plugged in.

					}

				}

				// DirectSound output
				DWORD ByteToLock;
				DWORD BytesToWrite;
				DWORD TargetCursor;
				DWORD PlayCursor;
				DWORD WriteCursor; // Leads the PlayCursor to ensure it doesn't overwritten by the hardware
				bool SoundIsValid = false;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(
					&PlayCursor,
					&WriteCursor)))
				{

					ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize; // Runs forever, but where in the buffer where would it be?

					TargetCursor =
						((PlayCursor +
							SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample) %
							SoundOutput.SecondaryBufferSize);


					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}

					SoundIsValid = true;
				}

				game_sound_output_buffer SoundBuffer = {};
				SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
				SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
				SoundBuffer.Samples = Samples;

				// Isolate Game and Platform layer
				game_offscreen_buffer Buffer = {};
				Buffer.BitmapMemory = GlobalBackBuffer.BitmapMemory;
				Buffer.Width = GlobalBackBuffer.Width;
				Buffer.Height = GlobalBackBuffer.Height;
				Buffer.Pitch = GlobalBackBuffer.Pitch;
				GameUpdateAndRender(NewInput, &Buffer, &SoundBuffer);

				if (SoundIsValid)
				{
					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
				}

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferToWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer, 0, 0, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				i64 EndCycleCount = __rdtsc();

				// ------ Dimensional Analysis ------- Clock Time Info! ----------
				i64 CyclesElapsed(EndCycleCount - LastCycleCount);
				i64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart; // Lets us compute the difference count every time.
				i32 MSPerFrame = (i32)((1000.0f * CounterElapsed) / PerfCountFrequency); // This returns the second  
				i32 FPS = (i32)(PerfCountFrequency / CounterElapsed);
				i32 MCPF = (i32)CyclesElapsed / (1000 * 1000);

				char buffer[256];
				wsprintfA(buffer, "%dms/f,	%df/s	%dmc/f\n", MSPerFrame, FPS, MCPF);
				// OutputDebugStringA(buffer);

				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;

				game_input* Temp = NewInput;
				NewInput = OldInput;
				OldInput = Temp;

				// Swap macro
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



/** ------- NOTES ------------------------------
 *  Hertz (Hz) is basically cycles per second.
 *  A cycle is the distance from peak to peak.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */