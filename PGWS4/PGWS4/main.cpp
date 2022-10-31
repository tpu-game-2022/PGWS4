#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>
#include<d3dcompiler.h>
#include<DirectXTex.h>

#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"DirectXTex.lib")
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
#endif
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

// アライメントにそろえたサイズを返す
// @param size 元のサイズ
// @param alignment アライメントサイズ
// @return アライメントをそろえたサイズ
size_t AlignmentedSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
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
	ID3D12CommandQueue* _cmdQueue = nullptr;
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
	result = _dev->CreateCommandQueue(&cmdQueueDese, IID_PPV_ARGS(&_cmdQueue));

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

	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue, hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)&_swapchain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //レンダーターゲットビューなので　RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;                      //裏表の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //特に指定なし

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);

	// sRGB レンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //ガンマ補正あり(sRGB)
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

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

	/*
	XMFLOAT3 vertices[] = {
		{-0.4f,-0.7f,0.0f},  //左下
		{-0.4f, +0.7f,0.0f },  //左上
		{+0.4f,-0.7f,0.0f },  //右下
		{+0.4f,+0.7f,0.0f},  //右上
	};*/

	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};

	Vertex vertices[] = {
		{{-0.4f,-0.7f,0.0f}, {0.0f,1.0f}},  //左下
		{{-0.4f,+0.7f,0.0f}, {0.0f,0.0f}},  //左上
		{{+0.4f,-0.7f,0.0f}, {1.0f,1.0f}},  //右下
		{{+0.4f,+0.7f,0.0f}, {1.0f,0.0f}},  //右上
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

	Vertex* vertMap = nullptr;
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
		"BasicVS", "vs_5_0",  //関数はBasicVS,対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,  //デバッグ用および最適化なし
		0,
		&_vsBlob, &errorBlob);  //エラー時はerrorBlobにメッセージが入る
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
		{ // uv(追加)
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
			0,D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
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
	gpipline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;  //0～1に正規化されたRGBA

	gpipline.SampleDesc.Count = 1;  //サンプリングは1ピクセルにつき1
	gpipline.SampleDesc.Quality = 0;  //クオリティは最低

	D3D12_ROOT_SIGNATURE_DESC rootSignaterDesc = {};
	rootSignaterDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange = {};
	descTblRange.NumDescriptors = 1;	//テクスチャ1つ
	descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	//種別はテクスタチャ
	descTblRange.BaseShaderRegister = 0;	//0番スロットから
	descTblRange.OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//ピクセルシェーダーから見える
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//ディスクリプタレンジのアドレス
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
	//ディスクリプタレンジ数
	rootparam.DescriptorTable.NumDescriptorRanges = 1;

	rootSignaterDesc.pParameters = &rootparam;	//ルートパラメーターの先頭アドレス
	rootSignaterDesc.NumParameters = 1;	//ルートパラメーター数

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//奥行きの繰り返し
	samplerDesc.BorderColor =
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	//ぼーだーは黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	//線形補間
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;	//ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;	//ミップマップ最小値
	samplerDesc.ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;	//ピクセルシェーダーから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;	// リサンプリングしない

	// ノイズをくっきりさせたい場合
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; //線形補間

	rootSignaterDesc.pStaticSamplers = &samplerDesc;
	rootSignaterDesc.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignaterDesc,  //ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0,  //ルートシグネチャバージョン
		&rootSigBlob,  //シェーダを作ったときと同じ
		&errorBlob);  //エラー処理も同じ

	ID3D12RootSignature* rootSigunature = nullptr;
	result = _dev->CreateRootSignature(
		0,  //nodemask。0でよい
		rootSigBlob->GetBufferPointer(), //シェーダの時と同様
		rootSigBlob->GetBufferSize(), //シェーダの時と同様
		IID_PPV_ARGS(&rootSigunature));
	rootSigBlob->Release(); //不要になったので解放

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

	//WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	result = LoadFromWICFile(
		L"img/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg);

	auto img = scratchImg.GetImage(0, 0, 0); //生データ抽出

/*
	struct TexRGBA
	{
		unsigned char R, G, B, A;
	};

	std::vector<TexRGBA> texturedata(256 * 256);
	for (auto& rgba : texturedata)
	{
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = 255;	// αは1.0とする
	}
*/


	//中間バッファーとしてのアップロードヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProp = {};

	// マップ可能にするため、UPLOADにする
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	// アップロード用に使用すること前提なのでUNKNOWNでよい
	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	uploadHeapProp.CreationNodeMask = 0;	// 単一アダプターのため0
	uploadHeapProp.VisibleNodeMask = 0;	// 単一アダプターのため0


	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_UNKNOWN; //単なるデータの塊なのでUNKNOWN
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	//単なるバッファーとして指定

	resDesc.Width = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;
	resDesc.Height = 1; // 高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;

	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; //連続したデータ
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; //特にフラグなし

	resDesc.SampleDesc.Count = 1; //通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0; //クオリティは最低

	//中間バッファー作成
	ID3D12Resource* uploadbuff = nullptr;

	result = _dev->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE, //特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_RESOLVE_DEST,
		nullptr,
		IID_PPV_ARGS(&uploadbuff)
	);


	// テクスチャのためのヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};

	texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;	//テクスチャ用
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texHeapProp.CreationNodeMask = 0;		// 単一アダプターのため0
	texHeapProp.VisibleNodeMask = 0;		// 単一アダプターのため0

	//リソース設定(変数は使いまわし)
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);	//幅
	resDesc.Height = static_cast<UINT>(metadata.height);	//幅
	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
	resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;  //レイアウトは決定しない

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, //コピー先
		nullptr,
		IID_PPV_ARGS(&texbuff));

	uint8_t* mapforImg = nullptr;	// image->pixels と同じ型にする
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);	 //マップ

	auto srcAddress = img->pixels;

	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	for (int y = 0; y < img->height; ++y) 
	{
		std::copy_n(srcAddress, rowPitch, mapforImg); //コピー

		//1行ごとのつじつまを合わせる
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}

	//std::copy_n(img->pixels, img->slicePitch, mapforImg);	//コピー
	uploadbuff->Unmap(0, nullptr); //アンマップ

	result = texbuff->WriteToSubresource(
		0,
		nullptr,	// 全領域へコピー
		img->pixels,	//元データアドレス
		static_cast<UINT>(img->rowPitch),	  //1ラインサイズ
		static_cast<UINT>(img->slicePitch)	  //全サイズ
	);

	ID3D12DescriptorHeap* texDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	// シェーダーから見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// マスクは0
	descHeapDesc.NodeMask = 0;
	// ビューは今のところ1つだけ
	descHeapDesc.NumDescriptors = 1;
	//シェーダ―リソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	// 生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap));


	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = texbuff;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	_cmdList->ResourceBarrier(1, &BarrierDesc);
	_cmdList->Close();

	//コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	D3D12_TEXTURE_COPY_LOCATION src = {};

	//コピー元(アップロード側)設定
	src.pResource = uploadbuff; //中間バッファー
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; //フットプリント指定
	src.PlacedFootprint.Offset = 0;
	src.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
	src.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
	src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
	src.PlacedFootprint.Footprint.RowPitch =
		static_cast<UINT>(AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	src.PlacedFootprint.Footprint.Format = img->format;

	D3D12_TEXTURE_COPY_LOCATION dst = {};

	//コピー先設定
	dst.pResource = texbuff;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

	_cmdQueue->Signal(_fence, ++_fenceVal);

	if (_fence->GetCompletedValue() != _fenceVal) 
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	_cmdAllocator->Reset(); //キューをクリア
	_cmdList->Reset(_cmdAllocator, nullptr);


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;	// RGBA(0.0f～1.0fに正規化)
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	// 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;	//ミップマップは使用しないので1

	_dev->CreateShaderResourceView(
		texbuff,	// ビューと関連付けるバッファー
		&srvDesc,		// 先ほど設定したテクスチャ設定情報
		texDescHeap->GetCPUDescriptorHandleForHeapStart()	// ヒープのどこに割り当てるか
	);

	float alpha = 0;
	while (true)
	{
		MSG msg;
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
		_cmdList->OMSetRenderTargets(1, &rtvH, 0, nullptr);

		//画面クリア
		//alpha = alpha + 0.01f;
		//if (alpha > 1.0f)
		//{
		//	alpha = 0.0f;
		//}
		float clearColor[] = { 0.0f,0.0f,0.3f,1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		_cmdList->SetGraphicsRootSignature(rootSigunature);
		_cmdList->SetDescriptorHeaps(1, &texDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(
			0,	// ルートパラメーターインデックス
			texDescHeap->GetGPUDescriptorHandleForHeapStart());//ヒープアドレス

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
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		//_swapchain->Present(1, 0);

		//待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);

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

