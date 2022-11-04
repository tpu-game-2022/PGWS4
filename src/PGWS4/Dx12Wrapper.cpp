#include "Dx12Wrapper.h"
#include <map>
#include <exception>

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;


static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

#ifdef _DEBUG
static void EnableDebugLayer()
{
	ComPtr<ID3D12Debug> debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	if (!SUCCEEDED(result)) return;

	debugLayer->EnableDebugLayer(); // デバッグレイヤーを有効化する
	debugLayer->Release(); // 有効化したらインターフェイスを解放する
}
#endif// _DEBUG

// ファイル名から拡張子を取得する
// @param path 対象のパス文字列
// @return 拡張子
static std::string GetExtension(const std::string& path)
{
	int idx = static_cast<int>(path.rfind('.'));
	return path.substr(
		idx + 1, path.length() - idx - 1);
}

// std::string（マルチバイト文字列）からstd::wstring（ワイド文字列）を得る
// @param str マルチバイト文字列
// @return 変換されたワイド文字列
static std::wstring GetWideStringFromString(const std::string& str)
{
	// 呼び出し1 回目( 文字列数を得る)
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr; // string のwchar_t 版
	wstr.resize(num1); // 得られた文字列数でリサイズ

	// 呼び出し2 回目（確保済みのwstr に変換文字列をコピー）
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1);

	assert(num1 == num2); // 一応チェック
	return wstr;
}



ComPtr<IDXGIFactory6> Dx12Wrapper::InitializeGraphicsInterface()
{
	ComPtr<IDXGIFactory6> dxgiFactory = nullptr;

#ifdef _DEBUG
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
#else
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
#endif
	return dxgiFactory;
}

ComPtr<ID3D12Device> Dx12Wrapper::InitializeDevice(IDXGIFactory6* dxgiFactory)
{
	static D3D_FEATURE_LEVEL levels[] =
	{
		 D3D_FEATURE_LEVEL_12_1,
		 D3D_FEATURE_LEVEL_12_0,
		 D3D_FEATURE_LEVEL_11_1,
		 D3D_FEATURE_LEVEL_11_0,
	};

	// アダプターの列挙用
	std::vector <IDXGIAdapter*> adapters;
	// ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0;
		dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); // アダプターの説明オブジェクト取得
		std::wstring strDesc = adesc.Description;
		// 探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
		else if (strDesc.find(L"Radeon") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	// Direct3D デバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	ComPtr<ID3D12Device> dev;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			return dev; // 生成可能なバージョンが見つかったらループを打ち切り
		}
	}

	return nullptr;
}

void Dx12Wrapper::InitializeCommand(
	ComPtr<ID3D12CommandAllocator>& cmdAllocator, ComPtr<ID3D12GraphicsCommandList>& cmdList,
	ComPtr<ID3D12CommandQueue>& cmdQueue, ID3D12Device* dev)
{
	// コマンドアロケーター
	ThrowIfFailed(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(cmdAllocator.ReleaseAndGetAddressOf())));

	// コマンドリスト
	ThrowIfFailed(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAllocator.Get(), nullptr,
		IID_PPV_ARGS(cmdList.ReleaseAndGetAddressOf())));

	// コマンドキュー
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;// タイムアウトなし
	cmdQueueDesc.NodeMask = 0;// アダプターを 1 つしか使わないときは 0 でよい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;// プライオリティは特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;// コマンドリストと合わせる
	// キュー生成
	ThrowIfFailed(dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(cmdQueue.ReleaseAndGetAddressOf())));
}

ComPtr<IDXGISwapChain4> Dx12Wrapper::CreateSwapChain(unsigned int width, unsigned int height,
	IDXGIFactory6* dxgiFactory, ID3D12CommandQueue* cmdQueue, HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = width;
	swapchainDesc.Height = height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = 2;
	// バックバッファーは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	// フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// 特に指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// ウィンドウ⇔フルスクリーン切り替え可能
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ComPtr<IDXGISwapChain4> swapchain = nullptr;
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
		cmdQueue, hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)swapchain.ReleaseAndGetAddressOf())); // 本来はQueryInterface などを用いて
	// IDXGISwapChain4* への変換チェックをするが、
	// ここではわかりやすさ重視のためキャストで対応

	return swapchain;
}

void Dx12Wrapper::InitializeBackBuffers(std::vector<ID3D12Resource*>& backBuffers, ComPtr<ID3D12DescriptorHeap>& rtvHeaps,
	ID3D12Device* dev, IDXGISwapChain4* swapchain)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビューなので RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; // 表裏の 2 つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 特に指定なし

	ThrowIfFailed(dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf())));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	ThrowIfFailed(swapchain->GetDesc(&swcDesc));

	// レンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ガンマ補正なし
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	backBuffers = std::vector<ID3D12Resource*>(swcDesc.BufferCount);
	for (UINT idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		ThrowIfFailed(swapchain->GetBuffer(idx, IID_PPV_ARGS(&backBuffers[idx])));
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeaps->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(idx, dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		dev->CreateRenderTargetView(backBuffers[idx], &rtvDesc, handle);
	}

}
void Dx12Wrapper::InitializeDepthBuffer(
	ComPtr<ID3D12Resource>& depthBuffer, ComPtr<ID3D12DescriptorHeap>& dsvHeap,
	unsigned int width, unsigned int height, ID3D12Device* dev)
{
	// 深度バッファーの作成
	D3D12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		width, height,
		1U, 1U, 1U, 0U, // arraySize, mipLevels, sampleCount, SampleQuality
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	// 深度値用ヒーププロパティ
	const D3D12_HEAP_PROPERTIES depthHeapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// このクリアする際の値が重要な意味を持つ
	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	ThrowIfFailed(dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf())));

	// 深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // 深度に使うことがわかればよい
	dsvHeapDesc.NumDescriptors = 1; // 深度ビューは1 つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // デプスステンシルビューとして使う
	ThrowIfFailed(dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf())));

	// 深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値に32 ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D テクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // フラグは特になし

	dev->CreateDepthStencilView(
		depthBuffer.Get(),
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

UINT64 Dx12Wrapper::InitializeFence(ComPtr<ID3D12Fence>& fence, ID3D12Device* dev)
{
	UINT64 fenceVal = 0;
	ThrowIfFailed(dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
	return fenceVal;
}

void Dx12Wrapper::InitializeMatrixes()
{
	// 定数バッファー作成
	_worldMat = XMMatrixRotationY(XM_PIDIV4);

	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	_viewMat = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	_projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2, // 画角は90°
		static_cast<float>(_screen_size[0])
		/ static_cast<float>(_screen_size[1]), // アスペクト比
		1.0f, // 近いほう
		100.0f // 遠いほう
	);
}

void Dx12Wrapper::CreateSceneView()
{
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_sceneConstBuff.ReleaseAndGetAddressOf())
	));

	ThrowIfFailed(_sceneConstBuff->Map(0, nullptr, (void**)&_mappedSceneData)); // マップ

	_mappedSceneData->world = _worldMat;
	_mappedSceneData->view = _viewMat;
	_mappedSceneData->proj = _projMat;
	_mappedSceneData->eye = XMFLOAT3(-_viewMat.r[3].m128_f32[0], -_viewMat.r[3].m128_f32[1], -_viewMat.r[3].m128_f32[2]);

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	// シェーダーから見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// マスクは0
	descHeapDesc.NodeMask = 0;
	// SRV1つとCBV1つ
	descHeapDesc.NumDescriptors = 1;
	// シェーダーリソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	// 生成
	ThrowIfFailed(_dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(_sceneDescHeap.ReleaseAndGetAddressOf())));

	//デスクリプタの先頭ハンドルを取得しておく
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHeapHandle(_sceneDescHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _sceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(_sceneConstBuff->GetDesc().Width);

	// 定数バッファービューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
}

ComPtr<ID3D12Resource> Dx12Wrapper::GetTextureByPath(const char* texpath) 
{
	auto it = _textureTable.find(texpath);
	if (it != _textureTable.end()) {
		//テーブルに内にあったらロードするのではなくマップ内の
		//リソースを返す
		return _textureTable[texpath];
	}
	else {
		return ComPtr<ID3D12Resource>(LoadTextureFromFile(texpath));
	}

}
void Dx12Wrapper::InitializeTextureLoaderTable(std::map<std::string, LoadLambda_t>& _textureTable)
{
	_textureTable["sph"]
		= _textureTable["spa"]
		= _textureTable["bmp"]
		= _textureTable["png"]
		= _textureTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)-> HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};
	_textureTable["tga"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)-> HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};
	_textureTable["dds"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};
}

ComPtr<ID3D12Resource> Dx12Wrapper::LoadTextureFromFile(const char* texPath)
{
	// ファイル名パスとリソースのマップテーブル
	auto it = _textureTable.find(texPath);
	if (it != _textureTable.end())
	{
		// テーブル内にあったらロードするのではなく
		// マップ内のリソースを返す
		return it->second;
	}

	// テクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	std::wstring wtexpath = GetWideStringFromString(texPath); // テクスチャのファイルパス
	std::string ext = GetExtension(texPath); // 拡張子を取得
	if (_loadLambdaTable.find(ext) == _loadLambdaTable.end()) { return nullptr; }// おかしな拡張子
	auto result = _loadLambdaTable[ext](
		wtexpath,
		&metadata,
		scratchImg);
	if (FAILED(result)) { return nullptr; }

	// WriteToSubresource で転送する用のヒープ設定
	const D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	const D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		static_cast<UINT>(metadata.width), // 幅
		static_cast<UINT>(metadata.height), // 高さ
		static_cast<UINT16>(metadata.arraySize),
		static_cast<UINT16>(metadata.mipLevels));

	// バッファー作成
	ComPtr<ID3D12Resource> texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) { return nullptr; }

	const Image* img = scratchImg.GetImage(0, 0, 0); // 生データ抽出

	result = texbuff->WriteToSubresource(
		0,
		nullptr, // 全領域へコピー
		img->pixels, // 元データアドレス
		static_cast<UINT>(img->rowPitch), // 1 ラインサイズ
		static_cast<UINT>(img->slicePitch) // 全サイズ
	);
	if (FAILED(result)) { return nullptr; }

	_textureTable[texPath] = texbuff.Get();
	return texbuff.Get();
}


Dx12Wrapper::Dx12Wrapper(HWND hwnd, unsigned int width, unsigned int height)
{
	_screen_size[0] = width;
	_screen_size[1] = height;

#ifdef _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif

	//DirectX12関連初期化
	_dxgiFactory = InitializeGraphicsInterface();// アダプター
	_dev = InitializeDevice(_dxgiFactory.Get());// デバイス
	InitializeCommand(_cmdAllocator, _cmdList, _cmdQueue, _dev.Get());// コマンドリスト関係
	_fenceVal = InitializeFence(_fence, _dev.Get());// フェンス

	_swapchain = CreateSwapChain(width, height, _dxgiFactory.Get(), _cmdQueue.Get(), hwnd);// スワップチェーン
	InitializeBackBuffers(_backBuffers, _rtvHeaps, _dev.Get(), _swapchain.Get());// バックバッファ
	InitializeDepthBuffer(_depthBuffer, _dsvHeap, width, height, _dev.Get());// 深度バッファ

	_viewport = CD3DX12_VIEWPORT(_backBuffers[0]);
	_scissorrect = CD3DX12_RECT(0, 0, width, height);

	InitializeMatrixes();
	CreateSceneView();

	// テクスチャ系の初期化
	InitializeTextureLoaderTable(_loadLambdaTable);
}

Dx12Wrapper::~Dx12Wrapper()
{
#ifdef _DEBUG
	// メモリ未開放の詳細表示
	ComPtr<ID3D12DebugDevice> debugInterface;
	if (SUCCEEDED(_dev->QueryInterface(debugInterface.GetAddressOf())))
	{
		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debugInterface->Release();
	}
#endif// _DEBUG
}

float angle = 0.0f;
void Dx12Wrapper::BeginDraw()
{
	//DirectX処理
	//バックバッファのインデックスを取得
	UINT bbIdx = _swapchain->GetCurrentBackBufferIndex();

	CD3DX12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET); // これだけで済む
	_cmdList->ResourceBarrier(1, &BarrierDesc);// バリア指定実行

	//レンダーターゲットを指定
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvH(_rtvHeaps->GetCPUDescriptorHandleForHeapStart());
	rtvH.Offset(bbIdx, _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvH(_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);

	// 画面クリア
	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 全画面表示設定
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorrect);
}

void Dx12Wrapper::ApplySceneDescHeap()
{
	angle += 0.1f;

	_worldMat = XMMatrixRotationY(angle);
	_mappedSceneData->world = _worldMat;
	_mappedSceneData->view = _viewMat;
	_mappedSceneData->proj = _projMat;

	_cmdList->SetDescriptorHeaps(1, _sceneDescHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(0,
		_sceneDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::EndDraw()
{
	UINT bbIdx = _swapchain->GetCurrentBackBufferIndex();

	// 前後だけ入れ替える
	CD3DX12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// 命令のクローズ
	_cmdList->Close();

	// コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	////待ち
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

	if (_fence->GetCompletedValue() != _fenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);// イベントハンドルの取得
		WaitForSingleObject(event, INFINITE);// イベントが発生するまで無限に待つ
		CloseHandle(event);// イベントハンドルを閉じる
	}

	_cmdAllocator->Reset(); // キューをクリア
	_cmdList->Reset(_cmdAllocator.Get(), nullptr); // 再びコマンドリストをためる準備

	// フリップ
	_swapchain->Present(1, 0);
}
