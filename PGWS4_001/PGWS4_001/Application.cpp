#include "Application.h"
#include <tchar.h>
#include "Dx12Wrapper.h"
#include "PMDRenderer.h"
#include "PMDActor.h"

using namespace DirectX;
using namespace Microsoft::WRL;

static void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf_s(format, valist);
	va_end(valist);
#endif
}

static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

static LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

static HWND CreateGameWindow(WNDCLASSEX& windowClass, UINT width, UINT height)
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;
	windowClass.lpszClassName = _T("DX12Sample");
	windowClass.hInstance = GetModuleHandle(nullptr);

	RegisterClassEx(&windowClass);

	RECT wrc = { 0, 0, (LONG)width, (LONG)height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	return CreateWindow(windowClass.lpszClassName, _T("DX12 テスト "), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, wrc.right - wrc.left, wrc.bottom - wrc.top, nullptr, nullptr, windowClass.hInstance, nullptr);
}

bool Application::Init()
{
	ThrowIfFailed(CoInitializeEx(0, COINIT_MULTITHREADED));

	_hwnd = CreateGameWindow(_windowClass, _window_width, _window_height);
	_dx12.reset(new Dx12Wrapper(_hwnd, _window_width, _window_height));
	_pmdRenderer.reset(new PMDRenderer(*_dx12));
	_pmdActor.reset(new PMDActor("Model/初音ミク.pmd", *_pmdRenderer)); //初音ミクmetal | 巡音ルカ

	return true;
}

void Application::Run()
{
	ShowWindow(_hwnd, SW_SHOW);

	while (true)
	{
		MSG msg;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		_pmdActor->Update();
		_dx12->BeginDraw();

		auto _cmdList = _dx12->CommandList();
		_cmdList->SetPipelineState(_pmdRenderer->GetPipelineState());
		_cmdList->SetGraphicsRootSignature(_pmdRenderer->GetRootSignature());

		_dx12->ApplySceneDescHeap();

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_pmdActor->Draw();
		_dx12->EndDraw();
	}
}

void Application::Terminate()
{
	UnregisterClass(_windowClass.lpszClassName, _windowClass.hInstance);
}

Application::Application()
{

}

Application::~Application()
{

}

Application& Application::Instance()
{
	static Application instance;

	return instance;
}