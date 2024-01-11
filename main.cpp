#include <Windows.h>

#define internal static
#define local_persist static
#define global_variable static

// Contain a variable to exit on its own
global_variable bool Running; // Global for now
global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory; // Memory that we receive from windows to draw into our renderer
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;
// DIBSection, device independent bitmap, talk about the things you wrtie into its buffer to display using GDI
internal void
x64ResizeDBISection(int Width, int Height)
{
	// TODO(Ben): Bulletproof this
	// Maybe don't free first, free after, then free first

	// Check to see if we have a bitmaphandle, if we don't then we go ahead and create one
	if (BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}

	if (!BitmapDeviceContext)
	{
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	BitmapInfo.bmiHeader.biSizeImage = BI_RGB;

	BitmapHandle = CreateDIBSection(
		 BitmapDeviceContext,
		 &BitmapInfo,
		 DIB_RGB_COLORS,
		 &BitmapMemory,
		 0,
		 0);
}

internal void
x64UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
	// Copy one rect to another
	StretchDIBits(DeviceContext, 
		X, Y, Width, Height,
		X, Y, Width, Height,
		BitmapMemory,
		&BitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY
	);
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
			int Width = ClientRect.bottom - ClientRect.top;
			int Height = ClientRect.right - ClientRect.left;
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
			
			

			// Find the Width & Height
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int Height = Paint.rcPaint.right - Paint.rcPaint.left;
			x64UpdateWindow(DeviceContext, X, Y, Width, Height);

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
		HWND WindowHandle = CreateWindowExA(
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
		if (WindowHandle)
		{
			// Start a message loop
			// A queue of message are created for you by the window
			// Use GetMessage() to dispatch incoming messages
			Running = true;
			MSG Message;
			while (Running)
			{
				BOOL MessageResult = (GetMessage(&Message, 0, 0, 0));
				if (MessageResult > 0) // If it doesn't exit 
				{
					// TranslateMessage(&Message); // Processes messages that comes in
					DispatchMessage(&Message);
				}
				else
				{
					break;
				}
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



