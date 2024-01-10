#include <Windows.h>

// UINT = 32 bit unsigned integer

LRESULT CALLBACK
MainWindowCallback(HWND Window,
					UINT Message,
					WPARAM wParam,
					LPARAM lParam)
{
	LRESULT Result = 0;


	switch (Message)
	{
		case WM_SIZE:	 // User change window size
		{
			OutputDebugStringA("WN_SIZE\n");
			break;
		}
		case WM_DESTROY: // Window deletes window
		{
			OutputDebugStringA("WM_DESTROY\n");
			break;
		}
		case WM_CLOSE:   // User closes the window
		{
			OutputDebugStringA("WM_CLOSE\n");
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

			// Find the WIDTH & HEIGHT
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int WIDTH = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int HEIGHT = Paint.rcPaint.right - Paint.rcPaint.left;
			PatBlt(DeviceContext,X, Y, WIDTH, HEIGHT, WHITENESS); 

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
	WindowClass.lpfnWndProc = MainWindowCallback; // Pointer to a function 
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
			MSG Message;
			for (;;)
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



