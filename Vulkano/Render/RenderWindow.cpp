#include "RenderWindow.h"

FRenderWindow* FRenderWindow::ThisWindow = nullptr;

FRenderWindow::FRenderWindow(const std::string& Title, int InWidth, int InHeight)
{
    Width = InWidth;
    Height = InHeight;
    WindowsName = Title;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						
{																									
	if (FRenderWindow::ThisWindow)																		
	{																								
		FRenderWindow::ThisWindow->HandleMessages(hWnd, uMsg, wParam, lParam);									
	}																						
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));												
}	

void FRenderWindow::Init(HINSTANCE hinstance)
{
    WindowInstance = hinstance;
	WNDCLASSEX wndClass;

	ThisWindow = this;

	HICON hIcon = (HICON)LoadImage(
		NULL, 
		L"Resources/Vulkano.ico", 
		IMAGE_ICON, 
		GetSystemMetrics(SM_CXSMICON), 
		GetSystemMetrics(SM_CYSMICON), 
		LR_LOADFROMFILE | LR_SHARED
	);

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hinstance;
	wndClass.hIcon = hIcon;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = L"Vulkano";
	wndClass.hIconSm = hIcon;

	if (!RegisterClassEx(&wndClass))
	{
		printf("Could not register window class!");
		fflush(stdout);
		exit(1);
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	

	DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	
	RECT windowRect = {
		0L,
		0L,
		(long)Width,
		(long)Height
	};

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);
	
	Window = CreateWindowExA(0,
		WindowsName.c_str(),
		WindowsName.c_str(),
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hinstance,
		NULL);

	if (!Window)
	{
		printf("Could not create window!");
		fflush(stdout);
		exit(1);
	}

	// Center on screen
	uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
	uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
	SetWindowPos(Window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	ShowWindow(Window, SW_SHOW);
	SetForegroundWindow(Window);
	SetFocus(Window);
}

void FRenderWindow::HandleMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		PostQuitMessage(0);
		break; 
	}
}

void FRenderWindow::Shutdown()
{
}

std::string FRenderWindow::GetWindowName() const
{
	return WindowsName;
}

HINSTANCE FRenderWindow::GetHInstance() const
{
	return WindowInstance;
}

HWND FRenderWindow::GetWindow() const
{
	return Window;
}

int FRenderWindow::GetWidth() const
{
	return Width;
}

int FRenderWindow::GetHeight() const
{
	return Height;
}
