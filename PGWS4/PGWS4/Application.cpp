#include "Application.h"
#include <tchar.h>
#include "Dx12Wrapper.h"
#include "PMDRenderer.h"
#include "PMDActor.h"

using namespace DirectX;
using namespace Microsoft::WRL;

using namespace DirectX;
using namespace Microsoft::WRL;

///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
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

//面倒ですが、ウィンドウプロシージャは必須なので書いておきます
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//ウィンドウが破棄されたら呼ばれます
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
}

static HWND CreateGameWindow(WNDCLASSEX& windowClass, UINT width, UINT height)
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	windowClass.lpszClassName = _T("DirectXTest");//アプリケーションクラス名(適当でいいです)
	windowClass.hInstance = GetModuleHandle(0);//ハンドルの取得
	RegisterClassEx(&windowClass);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	RECT wrc = { 0, 0, (LONG)width, (LONG)height };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
	//ウィンドウオブジェクトの生成
	return CreateWindow(windowClass.lpszClassName,//クラス名指定
		_T("DX12テスト"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT,//表示X座標はOSにお任せします
		CW_USEDEFAULT,//表示Y座標はOSにお任せします
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		windowClass.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ
}

bool Application::Init()
{
	ThrowIfFailed(CoInitializeEx(0, COINIT_MULTITHREADED)); // 呼ばないと動作しないメソッドがある

	_hwnd = CreateGameWindow(_windowClass, _window_width, _window_height); // ウィンドウ生成

	_dx12.reset(new Dx12Wrapper(_hwnd, _window_width, _window_height));
	_pmdRenderer.reset(new PMDRenderer(*_dx12));

	//_pmdActor.reset(new PMDActor("Model/巡音ルカ.pmd", *_pmdRenderer));
	//_pmdActor.reset(new PMDActor("Model/初音ミクmetal.pmd", *_pmdRenderer));
	_pmdActor.reset(new PMDActor("Model/初音ミク.pmd", *_pmdRenderer));
	//_pmdActor->LoadVMDFile("motion/swing.vmd", "pose");
	//_pmdActor->LoadVMDFile("motion/swing2.vmd", "pose");
	//_pmdActor->LoadVMDFile("motion/motion.vmd", "pose");
	_pmdActor->LoadVMDFile("motion/ビバハピ.vmd", "pose");
	_pmdActor->PlayAnimation();

	return true;
}

void Application::Run()
{
	ShowWindow(_hwnd, SW_SHOW); // ウィンドウ表示

	while (true)
	{
		MSG msg;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// アプリケーションが終わるときに message が WM_QUITになる
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		_dx12->BeginDraw();

		auto _cmdList = _dx12->CommandList();
		
		_cmdList->SetPipelineState(_pmdRenderer->GetPipelineState());

		_cmdList->SetGraphicsRootSignature(_pmdRenderer->GetRootSignature());

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		_dx12->ApplySceneDescHeap();

		_pmdActor->Update();
		_pmdActor->Draw();

		_dx12->EndDraw();		
	}
}

void Application::Terminate()
{
	// もうクラス使わんから登録解除
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