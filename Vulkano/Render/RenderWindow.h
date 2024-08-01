#pragma once
#include <string>
#include <Windows.h>

class FRenderWindow
{
public:
    FRenderWindow(const std::string& Title, int InWidth, int InHeight);
    void Init(HINSTANCE hinstance);
    void HandleMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Shutdown();

public:
    static FRenderWindow* ThisWindow;
    std::string GetWindowName() const;
    HINSTANCE GetHInstance() const;
    HWND GetWindow() const;
    int GetWidth() const;
    int GetHeight() const;

private:
    std::string WindowsName;
    int Width;
    int Height;
    bool bInit;

    HINSTANCE WindowInstance;
    HWND Window;
};
