#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include <vector>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace std;

//@brief コンソール画面にフォーマット付き文字列を表示
//@param format フォーマット(%dとか%fとかの)
//@param 可変長引数
//@remarks この関数はデバッグ用です。デバッグ時にしか動作しません。
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

//面倒だけど書かなければいけない関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
//ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//既定の処理を行う
}

#ifdef _DEBUG
int main()
{
#else アクティブでないプリプロセッサ　ブロック
#endif
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

//ウィンドウクラスの作成＆登録
WNDCLASSEX w = {};

w.cbSize = sizeof(WNDCLASSEX);
w.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
w.lpszClassName = _T("DX12Sample");//アプリケーションクラス名
w.hInstance = GetModuleHandle(nullptr);//ハンドルの取得

RegisterClassEx(&w);//アプリケーションクラス（ウィンドウクラスをOSに伝える）

RECT wrc = { 0,0,window_width,window_height };//ウィンドウサイズを決める

//関数を使ってウィンドウのサイズを補正する
AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
//ウィンドウオブジェクトの生成
HWND hwnd = CreateWindow(w.lpszClassName,
	_T("DX12テスト"),
	WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	wrc.right - wrc.left,
	wrc.bottom - wrc.top,
	nullptr,
	nullptr,
	w.hInstance,
	nullptr);//追加パラメーター

HRESULT D3D12CreateDevice(
	IUnknown * pAdapter,//ひとまずはnullptrでOK
	D3D_FEATURE_LEVEL MnimumFeatureLevel,//最低限必要なフィーチャーレベル
	REFIID riid,
	void** ppDevice
);

D3D_FEATURE_LEVEL levels[] =
{
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
};

auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
//アダプターの列挙用
std::vector<IDXGIAdapter*>adapters;
//ここに特定の名前を持つアダプターオブジェクトが入る
IDXGIAdapter* tmpAdapter = nullptr;
for (auto adpt : adapters)
{
	DXGI_ADAPTER_DESC adesc = {};
	adpt->GetDesc(&adesc);//アダプター説明オブジェクトの取得
	std::wstring strDesc = adesc.Description;
	//探したいアダプターの名前を確認
	if (strDesc.find(L"NVIDIA") != std::string::npos)
	{
		tmpAdapter = adpt;
		break;
	}
}

//DirectXデバイスの初期化
D3D_FEATURE_LEVEL featureLevel;
for (auto lv : levels)
{
	if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
	{
		featureLevel = lv;
		break;//生成可能なバージョンが見つかったらループを打ち切り
	}
}

result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
	IID_PPV_ARGS(&_cmdAllocator));
result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
	_cmdAllocator, nullptr,
	IID_PPV_ARGS(&_cmdList));

D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
//タイムアウトなし
cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//アダプターを１つしか使わないときは０でよい
cmdQueueDesc.NodeMask = 0;
//プライオリティは特に指定なし
cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
//コマンドリストと合わせる
cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
//キュー作成
result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));

DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
swapchainDesc.Width = window_width;
swapchainDesc.Height = window_height;
swapchainDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
swapchainDesc.Stereo = false;
swapchainDesc.SampleDesc.Count = 1;
swapchainDesc.SampleDesc.Quality = 0;
swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
swapchainDesc.BufferCount = 2;
//バックバッファーは伸び縮み可能
swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
//フリップ後は速やかには破棄
swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//特に指定なし
swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
//ウィンドウ、フルスクリーンの切り替え可能
swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
result = _dxgiFactory->CreateSwapChainForHwnd(
	_cmdQueue, hwnd,
	&swapchainDesc, nullptr, nullptr,
	(IDXGISwapChain1**)&_swapchain);//本来はQueryInterface等を用いて
//IDXGISwapChain4*への変換チェックをするが、
//ここではわかりやすさ重視ためにキャストで対応

D3D12_DESCRIPTOR_HEAP_DESC hespDesc = {};
hespDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
hespDesc.NodeMask = 0;
hespDesc.NumDescriptors = 2;//表裏の２つ
hespDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし

ID3D12DescriptorHeap* rtvHeaps = nullptr;
result = _dev->CreateDescriptorHeap(&hespDesc, IID_PPV_ARGS(&rtvHeaps));

DXGI_SWAP_CHAIN_DESC swcDesc = {};
result = _swapchain->GetDesc(&swcDesc);

std::vector<ID3D12Resource*>_backBuffers(swcDesc.BufferCount);
for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
{
	result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
	D3D12_CPU_DESCRIPTOR_HANDLE handle
		= rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
}

//ウィンドウ表示
ShowWindow(hwnd, SW_SHOW);

while (true)
{
	MSG msg;
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {

		//アプリケーションが終わるときにmesseageがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	//DirectX処理
		//バックバッファのインデックスを取得
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	//レンダーターゲットを指定
	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

	//画面クリア
	float clearcolor[] = { 1.0f,1.0f,0.0f,1.0f };//黄色
	_cmdList->ClearRenderTargetView(rtvH, clearcolor, 0, nullptr);

	//命令のクローズ
	_cmdList->Close();

	//コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	_cmdAllocator->Reset();//キューをクリア
	_cmdList->Reset(_cmdAllocator, nullptr);//再びコマンドリストをためる準備

	//フリップ
	_swapchain->Present(1, 0);
}

//もうクラスは使わないので登録解除する
UnregisterClass(w.lpszClassName, w.hInstance);

return 0;
}
