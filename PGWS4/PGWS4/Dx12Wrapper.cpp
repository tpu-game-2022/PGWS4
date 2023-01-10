#include "Dx12Wrapper.h"
#include <map>
#include <exception>

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

static  inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

#ifdef _DEBUG
void EnableDebugLayer()
{
	ComPtr<ID3D12Debug> debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	if (!SUCCEEDED(result))return;

	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}
#endif // DEBUG

static std::string GetExtension(const std::string& path) {
	int idx = static_cast<int>(path.rfind('.'));
	return path.substr(
		idx + 1, path.length() - idx - 1);
}

static std::wstring GetWideStringFromString(const std::string& str)
{
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr;
	wstr.resize(num1);

	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1);

	assert(num1 == num2);
	return wstr;
}

ComPtr<IDXGIFactory6> Dx12Wrapper::InitializeGraphicsInterface()
{
	ComPtr<IDXGIFactory6> dxgiFactory = nullptr;

#ifdef _DEBUG
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
#else
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
#endif // _DEBUG

	return dxgiFactory;
}

ComPtr<ID3D12Device> Dx12Wrapper::InitializeDevice(IDXGIFactory6* dxgiFactory)
{
	D3D_FEATURE_LEVEL levels[] =
	{
	 D3D_FEATURE_LEVEL_12_1,
	 D3D_FEATURE_LEVEL_12_0,
	 D3D_FEATURE_LEVEL_11_1,
	 D3D_FEATURE_LEVEL_11_0,
	};

	std::vector<IDXGIAdapter*>adapters;
	IDXGIAdapter* temAdapter = nullptr;
	for (int i = 0;
		dxgiFactory->EnumAdapters(i, &temAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {

		adapters.push_back(temAdapter);

	}

	for (IDXGIAdapter* adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;

		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			temAdapter = adpt;
			break;
		}
		else if (strDesc.find(L"Radeon") != std::string::npos)
		{
			temAdapter = adpt;
			break;
		}
	}

	D3D_FEATURE_LEVEL featureLevel;
	for (D3D_FEATURE_LEVEL lv : levels)
	{
		ComPtr<ID3D12Device> dev;
		if (D3D12CreateDevice(temAdapter, lv, IID_PPV_ARGS(dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			return dev; // 生成可能なバージョンが見つかったらループを打ち切り
		}
	}

	return nullptr;
}

void Dx12Wrapper::InitializeCommand(
	ComPtr<ID3D12CommandAllocator>& cmdAllocator, ComPtr<ID3D12GraphicsCommandList>& cmdList,
	ComPtr<ID3D12CommandQueue>& cmdQueue, ID3D12Device* dev) {

	ThrowIfFailed(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(cmdAllocator.ReleaseAndGetAddressOf())));

	ThrowIfFailed(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAllocator.Get(), nullptr,
		IID_PPV_ARGS(cmdList.ReleaseAndGetAddressOf())));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(cmdQueue.ReleaseAndGetAddressOf())));
}

ComPtr<IDXGISwapChain4> Dx12Wrapper::CreateSwapChain(unsigned int width, unsigned int height,
	IDXGIFactory6* dxgiFactory, ID3D12CommandQueue* cmdQueue, HWND hwnd) {

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = width;
	swapchainDesc.Height = height;
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

	ComPtr<IDXGISwapChain4> swapchain = nullptr;
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
		cmdQueue, hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)swapchain.ReleaseAndGetAddressOf()));

	return swapchain;
}

void Dx12Wrapper::InitializeBackBuffers(std::vector<ID3D12Resource*>& backBuffers, ComPtr<ID3D12DescriptorHeap>& rtvHeaps,
	ID3D12Device* dev, IDXGISwapChain4* swapchain) {

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

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

void Dx12Wrapper::InitializeDepthBuffer(ComPtr<ID3D12Resource>& depthBuffer, ComPtr<ID3D12DescriptorHeap>& dsvHeap,
	unsigned int width, unsigned int height, ID3D12Device* dev)
{
	//深度バッファーの作成
	D3D12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		width, height,
		1U, 1U, 1U, 0U,//arraySize,mipLevels,sampleCount,SampleQuality
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	//深度値ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	//このクリアする際の値が重要な意味を持つ
	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	ThrowIfFailed(dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf())));

	//深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	ThrowIfFailed(dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf())));

	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	dev->CreateDepthStencilView(
		depthBuffer.Get(),
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());

}

UINT64 Dx12Wrapper::InitializeFence(ComPtr<ID3D12Fence>& fence, ID3D12Device* dev)
{
	UINT64 fenceVal = 0;
	ThrowIfFailed(dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
	return fenceVal;
}

void Dx12Wrapper::InitializeMatrixes() {

	//定数バッファー作成
	//_worldMat = XMMatrixRotationY(XM_PIDIV4);

	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	_viewMat = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up)
	);

	_projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,
		static_cast<float>(_screen_size[0])
		/ static_cast<float>(_screen_size[1]),
		1.0f,
		100.0f
	);
}

void Dx12Wrapper::CreateSceneTransformView() {

	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_sceneConstBuff.ReleaseAndGetAddressOf())));

	ThrowIfFailed(_sceneConstBuff->Map(0, nullptr, (void**)&_mapScene));//マップ
	//_mapScene->word = _worldMat;//行列の内容をコピー
	_mapScene->view = _viewMat;//行列の内容をコピー
	_mapScene->proj = _projMat;
	_mapScene->eye = XMFLOAT3(-_viewMat.r[3].m128_f32[0], -_viewMat.r[3].m128_f32[1], -_viewMat.r[3].m128_f32[2]);


	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	//シェーダーからは見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//マスクは0
	descHeapDesc.NodeMask = 0;
	//SRV一つとCBV一つ
	descHeapDesc.NumDescriptors = 1;
	//シェーダーリソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//生成
	ThrowIfFailed(_dev->CreateDescriptorHeap(&descHeapDesc,
		IID_PPV_ARGS(_sceneDescHeap.ReleaseAndGetAddressOf())));


	//デスクリプタの先頭ハンドルを取得しておく
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHeapHandle(
		_sceneDescHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _sceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(_sceneConstBuff->GetDesc().Width);

	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
}

ComPtr<ID3D12Resource> Dx12Wrapper::GetTextureByPath(const char* texpath)
{
	auto it = _textureTable.find(texpath);
	if (it != _textureTable.end()) {return _textureTable[texpath];}
	else { return ComPtr<ID3D12Resource>(LoadTextureFromFile(texpath));}
}

void Dx12Wrapper::InitializeTextureLoaderTable(std::map<std::string, LoadLambda_t>& loadLambdaTable) {

	loadLambdaTable["sph"]
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)-> HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};
	loadLambdaTable["tga"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)-> HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};
	loadLambdaTable["dds"]
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
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	const D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		static_cast<UINT>(metadata.width),
		static_cast<UINT>(metadata.height),
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

	_dxgiFactory = InitializeGraphicsInterface();
	_dev = InitializeDevice(_dxgiFactory.Get());
	InitializeCommand(_cmdAllocator, _cmdList, _cmdQueue, _dev.Get());
	_fenceVal = InitializeFence(_fence, _dev.Get());

	_swapchain = CreateSwapChain(width, height, _dxgiFactory.Get(), _cmdQueue.Get(), hwnd);
	InitializeBackBuffers(_backBuffers, _rtvHeaps, _dev.Get(), _swapchain.Get());
	InitializeDepthBuffer(_depthBuffer, _dsvHeap, width, height, _dev.Get());

	_viewport = CD3DX12_VIEWPORT(_backBuffers[0]);
	_scissorrect = CD3DX12_RECT(0, 0, width, height);

	InitializeMatrixes();
	CreateSceneTransformView();

	InitializeTextureLoaderTable(_loadLambdaTable);
}

Dx12Wrapper::~Dx12Wrapper()
{
#ifdef _DEBUG
	ComPtr<ID3D12DebugDevice> debugInterface;
	if (SUCCEEDED(_dev->QueryInterface(debugInterface.GetAddressOf())))
	{
		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debugInterface->Release();
	}
#endif// _DEBUG
}

void Dx12Wrapper::BeginDraw()
{
	UINT bbIdx = _swapchain->GetCurrentBackBufferIndex();
	CD3DX12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvH(_rtvHeaps->GetCPUDescriptorHandleForHeapStart());
	rtvH.Offset(bbIdx, _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvH(_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);

	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorrect);
}

void Dx12Wrapper::ApplySceneDescHeap()
{
	_mapScene->view = _viewMat;
	_mapScene->proj = _projMat;

	ID3D12DescriptorHeap* sceneheaps[] = { _sceneDescHeap.Get() };
	_cmdList->SetDescriptorHeaps(1, sceneheaps);
	_cmdList->SetGraphicsRootDescriptorTable(0,
	_sceneDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::EndDraw()
{
	UINT bbIdx = _swapchain->GetCurrentBackBufferIndex();
	CD3DX12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	_cmdList->Close();
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
	if (_fence->GetCompletedValue() != _fenceVal) {
		HANDLE event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);// イベントハンドルの取得
		WaitForSingleObject(event, INFINITE);// イベントが発生するまで無限に待つ
		CloseHandle(event);// イベントハンドルを閉じる
	}

	_cmdAllocator->Reset(); // キューをクリア
	_cmdList->Reset(_cmdAllocator.Get(), nullptr); // 再びコマンドリストをためる準備

	// フリップ
	_swapchain->Present(1, 0);
}