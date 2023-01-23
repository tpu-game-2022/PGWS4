#include "Application.h"
#include <tchar.h>
#include "Dx12Wrapper.h"
#include "PMDRenderer.h"
#include "PMDActor.h"

using namespace DirectX;
using namespace Microsoft::WRL;

// @brief コンソール画面にフォーマット付き文字列を表示
// @param format フォーマット（%d とか %f とかの）
// @param 可変長引数
// @remarks この関数はデバッグ用です。デバッグ時にしか動作しません
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

// 面倒だけど書かなければいけない関数
static LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OS に対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam); // 既定の処理を行う
}


static HWND CreateGameWindow(WNDCLASSEX& windowClass, UINT width, UINT height)
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure; // コールバック関数の指定
	windowClass.lpszClassName = _T("DX12Sample"); // アプリケーションクラス名（適当でよい）
	windowClass.hInstance = GetModuleHandle(nullptr); // ハンドルの取得

	RegisterClassEx(&windowClass); // アプリケーションクラス（ウィンドウクラスの指定を OS に伝える）

	RECT wrc = { 0, 0, (LONG)width, (LONG)height }; // ウィンドウサイズを決める

	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	// ウィンドウオブジェクトの生成
	return CreateWindow(windowClass.lpszClassName, // クラス名指定
		_T("DX12 テスト "), // タイトルバーの文字
		WS_OVERLAPPEDWINDOW, // タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT, // 表示 x 座標は OS にお任せ
		CW_USEDEFAULT, // 表示 y 座標は OS にお任せ
		wrc.right - wrc.left, // ウィンドウ幅
		wrc.bottom - wrc.top, // ウィンドウ高
		nullptr, // 親ウィンドウハンドル
		nullptr, // メニューハンドル
		windowClass.hInstance, // 呼び出しアプリケーションハンドル
		nullptr); // 追加パラメーター
}


bool Application::Init()
{
	ThrowIfFailed(CoInitializeEx(0, COINIT_MULTITHREADED));// 呼ばないと動作しないメソッドがある

	_hwnd = CreateGameWindow(_windowClass, _window_width, _window_height);// ウィンドウ生成

	_dx12.reset(new Dx12Wrapper(_hwnd, _window_width, _window_height));
	_pmdRenderer.reset(new PMDRenderer(*_dx12));

		_pmdActor.reset(new PMDActor("Model/巡音ルカ.pmd", *_pmdRenderer));
	//	_pmdActor.reset(new PMDActor("Model/初音ミクmetal.pmd", *_pmdRenderer));
	//  _pmdActor.reset(new PMDActor("Model/初音ミク.pmd", *_pmdRenderer));
	//  _pmdActor->LoadVMDFile("motion/pose.vmd", "pose");
	//  _pmdActor->LoadVMDFile("motion/swing.vmd", "pose");
	//  _pmdActor->LoadVMDFile("motion/swing2.vmd", "pose");
	  _pmdActor->LoadVMDFile("motion/motion.vmd", "pose");
	_pmdActor->PlayAnimation();

	return true;
}

void Application::Run()
{
	//ウィンドウ表示
	ShowWindow(_hwnd, SW_SHOW);

	while (true)
	{
		MSG msg;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// アプリケーションが終わるときに message が WM_QUIT になる
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
	//もうクラス使わんから登録解除
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