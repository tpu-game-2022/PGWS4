#include<Windows.h>
//ウィンドウの表記に必要
#include<tchar.h>
//DirectX12のオブジェクトとして各命令を出すのに必要
#include<d3d12.h>
//表記に必要
#include<dxgi1_6.h>
//アダプター検索に必要
#include<vector>
//-------------------------------------------------------
//ポリゴン表示に使う
#include<DirectXMath.h>
//HLSL用
#include<d3dcompiler.h>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace std;
using namespace DirectX;

//@brief コンソール画面にフォーマット付き文字列を表示
//@param format フォーマット(%d、%f等)
//@param 可変長引数
//@remarks デバッグ用関数（デバッグ時にしか表示されない）

void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

//OSのイベントに応答する処理(書かないといけない)
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		//OSに対して終了させる事を伝える
		PostQuitMessage(0);
		return 0;
	}
	//既定の処理を使う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	if (!SUCCEEDED(result)) return;

	//デバッグレイヤーを有効にする
	debugLayer->EnableDebugLayer();
	//有効かしたらインターフェイスを解放する
	debugLayer->Release();
}
#endif// _DEBUG


//グラデーション用
void ColorChange(float* r, float* g, float* b)
{
	//赤色
	if (*r < 1.0f && *g < 1.0f && *b < 1.0f)
	{
		*r += 0.01f;
	}
	//黄色
	if(*r >= 1.0f  && *g < 1.0f)
	{
		*g += 0.01f;
	}
	//緑
	if (*r >= 0.0f && *g >= 1.0f)
	{
		*r -= 0.01f;
	}
	//水色
	if (*g >= 1.0f && *b < 1.0f && *r < 0.0f)
	{
		*b += 0.01f;
	}
	//青色
	if (*b >= 1.0f && *r < 0.0f && *g >= 0.0f)
	{
		*g -= 0.01f;
	}
	//紫
	if (*b >= 1.0f && *r < 1.0f && *g < 0.0f)
	{
		*r += 0.01f;
	}
	//黒

}

#ifdef _DEBUG
//コマンドライン出力で情報を表示できるようにデバッグ時はmainを使用
int main()
{
#ifdef _DEBUG
	//デッバグレイヤーをオン
	EnableDebugLayer();
#endif // _DEBUG

#else
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int)
{
#endif // DEBUG
	//ウィンドウの縦横
	const unsigned int window_width = 1280;
	const unsigned int window_height = 720;

	//オブジェクト命令、生成、画面更新用変数
	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	//コマンド作成用(CPUからGPUに命令する用)
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	//コマンドキュー作成用
	ID3D12CommandQueue* _cmdQueue = nullptr;

	IDXGISwapChain4* _swapchain = nullptr;

	//ウィンドウクラスの生成登録
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	//コールバック関数の指定
	w.lpfnWndProc = (WNDPROC)WindowProcedure;
	//アプリケーションクラス名
	w.lpszClassName = _T("DX12Sample");
	//ハンドルの取得
	w.hInstance = GetModuleHandle(nullptr);

	//アプリケーションクラス(ウィンドウクラスの指定をOSに伝える)
	RegisterClassEx(&w);

	//ウィンドウサイズを決める
	RECT wrc = { 0,0,window_width,window_height };

	//関数を使ってウィンドウのサイズを設定する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(
		//クラス名指定
		w.lpszClassName,
		//タイトルバーの文字
		_T("DX12 テスト"),
		//タイトルバーと境界線のあるウィンドウ
		WS_OVERLAPPEDWINDOW,
		//表示x(OS頼り)
		CW_USEDEFAULT,
		//表示y(同様)
		CW_USEDEFAULT,
		//ウィンドウ幅
		wrc.right - wrc.left,
		//ウィンドウ高
		wrc.bottom - wrc.top,
		//親ウィンドウハンドル
		nullptr,
		//メニューハンドル
		nullptr,
		//呼び出しアプリケーションハンドル
		w.hInstance,
		//追加パラメータ-
		nullptr);

	//D3D12Deviceの生成------------------------------------------------
	HRESULT D3D12CreateDevice(
		IUnknown * pAdapter,
		D3D_FEATURE_LEVEL MinimumFeatureLevel,
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

#ifdef _DEBUG
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif // _DEBUG

	//アダプターの列挙と検索-------------------------------------------------
	//アダプター列挙用
	vector<IDXGIAdapter*> adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	//列挙ここまで
	//検索
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		//探したいアダプターの名前を確認(nullでないか確認する)
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;
		}
	}
	
	//コマンドリスト作成-----------------------------------------------
	result = _dev->
		CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->
		CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocator, nullptr,
		IID_PPV_ARGS(&_cmdList));
	//コマンドキュー作成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	//タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//アダプターを1つしか使わない場合は0で可
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	//コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//キュー作成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(& _cmdQueue));

	//スワップチェーンの作成-------------------------------------------
	HRESULT CreateSwapChainForHwnd(
		IUnknown * pDevice,
		HWND hWnd,
		const DXGI_SWAP_CHAIN_DESC1 * pDesc,
		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pFullscreenDesc,
		IDXGIOutput * pRestrictToOutput,
		IDXGISwapChain1 * *ppSwapChain
	);

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	//バッファーは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	//フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//ウィンドウフルスクリーン切替可能
	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue, hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)&_swapchain);

	//スワップチェーンのバッファを取得---------------------------------
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	//レンダーターゲットビューなのでRTV
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	//裏表の2つ
	heapDesc.NumDescriptors = 2;
	//指定なし
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	//スワップチェーンのリソースとRTVを関連付ける
	//スワップチェーンの情報の取得
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	//ディスクリプタをバッファの数だけ生成
	vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	for (UINT idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		//バッファのポインタを取得
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		D3D12_CPU_DESCRIPTOR_HANDLE handle
			= rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
	}

	//フェンスオブジェクトの宣言と生成（待ちで必要）
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	//三角形を生成する用--------------------------------------------------
	//3つの頂点を定義
	XMFLOAT3 vertices[]
	{
		//左下
		{-1.0f,-1.0f,0.0f},
		//左上
		{-1.0f,+1.0f,0.0f},
		//右下
		{+1.0f,-1.0f,0.0f},
	};
	//頂点バッファの生成
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	//頂点情報が入るだけのサイズ
	resdesc.Width = sizeof(vertices);
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	//頂点データをコピ-
	//メインメモリに頂点バッファと同じメモリを確保
	XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	//確保したメインメモリに頂点データをコピー
	std::copy(std::begin(vertices), std:: end(vertices), vertMap);
	//CPUで設定は完了したのでビデオメモリに転送
	vertBuff->Unmap(0, nullptr);

	//頂点バッファビュー(頂点バッファの説明)
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	//バッファーの仮想アドレス
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	//全バイト数
	vbView.SizeInBytes = sizeof(vertices);
	//1頂点当たりのバイト数
	vbView.StrideInBytes = sizeof(vertices[0]);

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* errorBlob = nullptr;
	result = D3DCompileFromFile(
		//シェーダー名
		L"BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&_vsBlob, &errorBlob);

	//ピクセルシェーダーのコンパイルロード
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見つかりません");
		}
		else
		{
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}
	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	//チャレンジ課題1---------------------------------------------
	float r = 0.0f, g = 0.0f, b = 0.0f;

	while (true)
	{
		MSG msg;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//アプリが終わる時にmessageがWM_QUITになる
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//スワップチェーンの動作
		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		//遷移
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; 
		//指定なし
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		//バックバッファーリソース
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
		BarrierDesc.Transition.Subresource = 0;
		//直前はPRESENT状態
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		//今からRT状態
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//バリア指定実行
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		//画面クリア(α値無し)
		ColorChange(&r,&g,&b);
		float clearColor[] = { r,g,b, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		//前後だけ入れ替える
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//命令のクローズ
		_cmdList->Close();

		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);

		//待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);
		if (_fence->GetCompletedValue() != _fenceVal)
		{
			//（ビジーループを使うとCPUを消費させるので良くない)
			//スレッドを寝かせてイベントが起きたらOSに起こしてもらう
			auto event = CreateEvent(nullptr, false, false, nullptr);
			//イベントハンドルの取得
			_fence->SetEventOnCompletion(_fenceVal, event);
			//イベントが発生するまで無限に待つ
			WaitForSingleObject(event, INFINITE); 
			//イベントハンドルを閉じる
			CloseHandle(event); 
		}

		//キューをクリア
		_cmdAllocator->Reset();
		//再びコマンドリストをためる準備
		_cmdList->Reset(_cmdAllocator, nullptr);

		_swapchain->Present(1, 0);
	}

	//クラスはもう使用しないので登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
