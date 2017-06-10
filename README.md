## Visual Studio
* Version             : Visual Studio Community 2017
* Platform Toolset    : Visual Studio 2017(v141)
* Windows SDK Version : 10.0.14393.0

## LibGen.bat
Automatic create Library folder and copy files from project folder

    .
    └── Library
        │
        ├── bin
        │   ├── x64
        │   │   └── WinFW.lib
        │   └── x86
        │       └── WinFW.dll
        │
        ├── include
        │   └── WinFW.hpp
        │
        └── lib
            ├── x64
            │   └── WinFW.lib
            └── x86
                └── WinFW.lib

## Example
```cpp
#define USE_MAIN
#include <WinFW.hpp>

#pragma comment(lib, "WinFW.lib")

using namespace WinFW;

IPtr<Window> window;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

constexpr int width = 800;
constexpr int height = 600;
constexpr wchar_t *title = L"Test";

int main(HINSTANCE hInstance, char *lpCmdLine, int nCmdShow) {
	window = Window::New(WindowConfig::New(WinClass::New(WinClassConfig::New(L"Class1", WndProc)), width, height));
	window->setTitle(title);
	window->show();

	EventLoop::init();
	while (EventLoop::isActive()) {
		if (EventLoop::fps(30)) {

		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
		EventLoop::destroy();
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
```
