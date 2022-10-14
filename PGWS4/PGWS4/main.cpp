#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>
#include<d3dcompiler.h>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
using namespace std;
using namespace DirectX;

void DebugOutPutFormatStaring(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // 
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//既定の処理を行う
}

#ifdef _DEBUG
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	if (!SUCCEEDED(result))return;

	debugLayer->EnableDebugLayer(); //デバッグレイヤーを有効にする
	debugLayer->Release();          //有効化したらインターフェイスを開放する
}
#endif _DEBUG

#ifdef _DEBUG
int main()
{
#else
int WINAOI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	const unsigned int window_width = 1280;
	const unsigned int window_height = 720;

	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQeue = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;

	//ウィンドウクラスの生成＆登録
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");//アプリケーションのクラス名
	w.hInstance = GetModuleHandle(nullptr);//ハンドルの取得

	RegisterClassEx(&w);//アプリケーションクラス（ウィンドウの指定をOSに伝える）

	RECT wrc = { 0,0,window_width,window_height };//ウィンドウサイズを決める

	//関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName, //クラス名指定
		_T("DX12 テスト"),                    //タイトルバーの文字
		WS_OVERLAPPEDWINDOW,                  //タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,                        //表示 x 座標はOSにお任せ
		CW_USEDEFAULT,                        //表示 y 座標はOSにお任せ
		wrc.right - wrc.left,                 //ウィンドウ幅
		wrc.bottom - wrc.top,                 //ウィンドウ高
		nullptr,                              //親ウィンドウ
		nullptr,                              //メニューハンドル
		w.hInstance,                          //呼び出しアプリケーション
		nullptr);                             //追加パラメーター

#ifdef  _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif //  _DEBUG

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

	//アダプターの列挙用
	std::vector<IDXGIAdapter*>adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdampter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdampter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		adapters.push_back(tmpAdampter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);//アダプターの説明オブジェクト取得
	    std:wstring strDesc = adesc.Description;
		//探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdampter = adpt;
			break;
		}
	}
	//Direct3D　デバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdampter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break; //生成可能なバージョンが見つかったらループを打ち切り
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocator, nullptr,
		IID_PPV_ARGS(&_cmdList));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDese = {};
	cmdQueueDese.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDese.NodeMask = 0;
	cmdQueueDese.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDese.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = _dev->CreateCommandQueue(&cmdQueueDese, IID_PPV_ARGS(&_cmdQeue));

	HRESULT CreateSwapChainForHwnd(
		IUnknown*pDevice,
		HWND hWnd,
		const DXGI_SWAP_CHAIN_DESC1*pDesc,
		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*pFullscreenDesc,
		IDXGIOutput*pRestrictToOutput,
		IDXGISwapChain1**ppSwapChain
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

	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQeue, hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)&_swapchain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //レンダーターゲットびゅーなので　RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;                      //裏表の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //特に指定なし

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

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

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	XMFLOAT3 vertices[] = {
		{-0.4f,-0.7f,0.0f},  //左下
		{-0.4f, +0.7f,0.0f },  //左上
		{+0.4f,-0.7f,0.0f },  //右下
		{+0.4f,+0.7f,0.0f},  //右上
	};

	//頂点インデックス
	unsigned short indices[] = {
		0,1,2,
		2,1,3,
	};

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);  //頂点情報が入るだけのサイズ
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

	XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	ID3D12Resource* idxbuff = nullptr;
	//設定は、バッファーのサイズ以外、頂点バッファーの設定を使いまわしても良い
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxbuff));

	//作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxbuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxbuff->Unmap(0, nullptr);

	//インデックスバッファービューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxbuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();  //バッファーの仮想アドレス
	vbView.SizeInBytes = sizeof(vertices);  //全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]); //1頂点あたりのバイト数

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* errorBlob = nullptr;
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",  //シェーダー名
		nullptr,  //defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,  //インクルードはデフォルト
		"BasicVS","vs_5_0",  //関数はBasicVS,対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG|D3DCOMPILE_SKIP_OPTIMIZATION,  //デバッグ用および最適化なし
		0,
		&_vsBlob,&errorBlob);  //エラー時はerrorBlobにメッセージが入る
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見当たりません");
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

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",  //シェーダー名
		nullptr,  //defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,  //インクルードはデフォルト
		"BasicPS", "ps_5_0",  //関数はBasicVS,対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,  //デバッグ用および最適化なし
		0,
		&_psBlob, &errorBlob);  //エラー時はerrorBlobにメッセージが入る
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見当たりません");
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

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipline = {};

	gpipline.pRootSignature = nullptr; //あとで設定する

	gpipline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipline.PS.BytecodeLength = _psBlob->GetBufferSize();

	//デフォルトのサンプルマスクを表す定数（0xffffffff）
	gpipline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//まだアンチエイリアスは使わないためfalse
	gpipline.RasterizerState.MultisampleEnable = false;

	gpipline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  //カリングしない
	gpipline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;  //中身を塗りつぶす
	gpipline.RasterizerState.DepthClipEnable = true;  //深度方向のクリッピングは有効に

	gpipline.BlendState.AlphaToCoverageEnable = false;
	gpipline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	//ひとまず加算や乗算やαブレンディングは使用しない
	renderTargetBlendDesc.BlendEnable = false;
	//ひとまず論理演算子は使用しない
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipline.InputLayout.pInputElementDescs = inputLayout;  //レイヤー先頭アドレス
	gpipline.InputLayout.NumElements = _countof(inputLayout);  //レイアウト配列の要素数

	gpipline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; //ストリップ時のカットなし
	gpipline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;  //三角形で構成

	gpipline.NumRenderTargets = 1;  //今は１つのみ
	gpipline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  //0~1に正規化されtPGBA

	gpipline.SampleDesc.Count = 1;  //サンプリングは1ピクセルにつき1
	gpipline.SampleDesc.Quality = 0;  //クオリティは最低

	D3D12_ROOT_SIGNATURE_DESC rootSignaterDesc = {};
	rootSignaterDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* rotSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignaterDesc,  //ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0,  //ルートシグネチャバージョン
		&rotSigBlob,  //シェーダを作ったときと同じ
		&errorBlob);  //エラー処理も同じ

	ID3D12RootSignature* rootSigunature = nullptr;
	result = _dev->CreateRootSignature(
		0,  //nodemask。0でよい
		rotSigBlob->GetBufferPointer(), //シェーダの時と同様
		rotSigBlob->GetBufferSize(), //シェーダの時と同様
		IID_PPV_ARGS(&rootSigunature));
	rotSigBlob->Release(); //不要になったので解放

	gpipline.pRootSignature = rootSigunature;

	ID3D12PipelineState* _pipelinesture = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipline, IID_PPV_ARGS(&_pipelinesture));

	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;  //出力先の幅(ピクセル数)
	viewport.Height = window_height;  //出力先の高さ(ピクセル数)
	viewport.TopLeftX = 0; //出力先の左上座標X
	viewport.TopLeftY = 0; //出力先の左上座標Y
	viewport.MaxDepth = 1.0f; //深度最大値
	viewport.MinDepth = 0.0f; //深度最小値

	D3D12_RECT scissorrect = {};  //ビューポートの全てを表示する設定
	scissorrect.top = 0;  //切り抜き上座標
	scissorrect.left = 0;  //切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;  //切り抜き上座標
	scissorrect.bottom = scissorrect.top + window_height;  //切り抜き下座標


	float alpha = 0;
	while (true)
	{
		MSG msg ;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//アプリケーションが終わるときにmessageがWM_QUITになる
			if (msg.message == WM_QUIT)
			{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//DirectXの処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; //遷移
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;  //特になし
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];  //バックバッファ―リソース
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;  //直前はPRESENT状態
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;  //今からRT状態
		_cmdList->ResourceBarrier(1, &BarrierDesc);  //バリア指定実行

		_cmdList->SetPipelineState(_pipelinesture);


		//レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, 0,nullptr);

		//画面クリア
		//alpha = alpha + 0.01f;
		//if (alpha > 1.0f)
		//{
		//	alpha = 0.0f;
		//}
		float clearColor[] = { 0.0f,0.0f,0.3f,1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		_cmdList->SetGraphicsRootSignature(rootSigunature);

		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0); //6=インデックス数

		//前後だけ入れ替える
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//命令のクローズ
		_cmdList->Close();

		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQeue->ExecuteCommandLists(1, cmdlists);
		//_swapchain->Present(1, 0);

		//待ち
		_cmdQeue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal)
		{
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);  //イベントハンドルの取得
			WaitForSingleObject(event, INFINITE);  //イベントが発生するまで無限に待つ
			CloseHandle(event);  //イベントハンドルを閉じる
		}

		_cmdList->Reset(_cmdAllocator, nullptr); //キューをクリア
		_swapchain->Present(1, 0); //再びコマンドリストを溜める準備
	}
	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
	DebugOutPutFormatStaring("Show window test.");
	getchar();
	return 0;
}

