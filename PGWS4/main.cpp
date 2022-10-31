#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>
#include<d3dcompiler.h>
#include<DirectXTex.h>
#include<d3dx12.h>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace std;
using namespace DirectX;

//@brief コンソール画面にフォーマット付き文字列を表示
//@param format フォーマット(%dとか%fとかの)
//@param可変長引数
//@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // DEBUG
}

//面倒だけど書かなければいけない関数
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

///アライメントに揃えたサイズを返す
///@param size 元のサイズ
///@param alignment アライメントサイズ
///@return アライメントをそろえたサイズ
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

	debugLayer->EnableDebugLayer();//デバッグレイヤーを有効化する
	debugLayer->Release();//有効化したらインターフェイスを開放する
}
#endif
#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
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
	w.lpszClassName = _T("DX12Sample");//アプリケーションクラス名(適当でよい)
	w.hInstance = GetModuleHandle(nullptr);//ハンドルの取得

	RegisterClassEx(&w);//アプリケーションクラス(ウィンドウクラスの指定をOSに伝える)

	RECT wrc = { 0, 0, window_width, window_height };//ウィンドウサイズを決める

	//関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,//クラス名指定
		_T("DX12 テスト"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,//表示x座標はOSにお任せ
		CW_USEDEFAULT,//表示y座標はOSにお任せ
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		w.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメーター

	//HRESULT D3D12CreateDevice(
	//	IUnknown * pAdapter,//ひとまずはnullptrでOK(自動でグラフィックドライバを選択)
	//	D3D_FEATURE_LEVEL MinimumFeatureLevel,//最低限必要なフィーチャーレベル
	//	REFIID riid, //後述(受け取りたいオブジェクトの方を識別するためのID)
	//	void** ppDevice//後述
	//);

#ifdef _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif // 

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
#endif
	//アダプターの列挙用
	std::vector<IDXGIAdapter*>adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0;
		_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);//アダプターの説明オブジェクト取得
		std::wstring strDesc = adesc.Description;
		//探したいアダプターの名前を確認
		if (strDesc.find(L"Intel") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}
	//Direct3D　デバイスの初期化
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
	//アダプターを1つしか使わない時は0でよい
	cmdQueueDesc.NodeMask = 0;
	//プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	//コマンドリスと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//キュー生成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	//バックバッファーは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	//フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//特に指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//ウィンドウ、フルスクリーン切り替え可能
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue, hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)&_swapchain);//本来はQueryIterfaceなどを用いて
	//IDXGISwapChain4* への変更チェックをするが、
	// ここではわかりやすさ重視のためキャストで対応
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなのでRTV
	heapDesc.NodeMask = 0;;
	heapDesc.NumDescriptors = 2;//裏表の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);

	//SRGBレンダーターゲットビュー設定
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
		_dev->CreateRenderTargetView(_backBuffers[idx],&rtvDesc, handle);
	}
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	//XMFLOAT3 vertices[] = {
	//	{-0.4f,-0.7f,0.0f},//左下
	//	{-0.4f,+0.7f,0.0f} ,//左上
	//	{+0.4f,-0.7f,0.0f} ,//右下
	//	{+0.4f,+0.7f,0.0f} //右上
	//};

	struct Vertex {
		XMFLOAT3 pos; // xyz座標
		XMFLOAT2 uv; // uv座標
	};

	Vertex vertices[] = {
		{{-0.1f,-0.7f,0.0f},{0.0f,1.0f}}, // 左下
		{{-0.1f,+0.7f,0.0f},{0.0f,0.0f}}, // 左上
		{{+0.7f,-0.7f,0.0f},{1.0f,1.0f}}, // 右下
		{{+0.7f,+0.7f,0.0f},{1.0f,0.0f}}, // 右上
	};
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices));
	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	copy(begin(vertices), end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファーの仮想アドレス
	vbView.SizeInBytes = sizeof(vertices);//全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]);//1頂点当たりのバイト数
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",//シェーダー名
		nullptr,//defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//includeはデフォルト
		"BasicVS", "vs_5_0",//関数はBasicVS,対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ及び最適化なし
		0,
		&_vsBlob, &errorBlob);//エラー時はerrorBlobにメッセージが入る
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
			if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
			{
				::OutputDebugStringA("ファイルが見当たりません");
			}
			else
			{
				string errstr;
				errstr.resize(errorBlob->GetBufferSize());
				copy_n((char*)errorBlob->GetBufferPointer(),
					errorBlob->GetBufferSize(), errstr.begin());
				errstr += "\n";
				OutputDebugStringA(errstr.c_str());
			}
		exit(1);
	}
	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",//シェーダー名
		nullptr,//defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//includeはデフォルト
		"BasicPS", "ps_5_0",//関数はBasicPS,対象シェーダーはps_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ及び最適化なし
		0,
		&_psBlob, &errorBlob);//エラー時はerrorBlobにメッセージが入る
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else
		{
			string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{ // uv(追加)
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
			0, D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
	};

	unsigned short indices[] =
	{
		0, 1, 2,
		2, 1, 3,
	};

	ID3D12Resource* idxBuff = nullptr;
	//設定は、バッファーのサイズ以外、頂点バッファーの設定を使いまわしてよい
	resourceDesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));
	//作ったバッファーにインデックスデータをコピー
	unsigned short* mappedidx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedidx);
	copy(begin(indices), end(indices), mappedidx);
	idxBuff->Unmap(0, nullptr);

	//インデックスビューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	gpipeline.pRootSignature = nullptr;//後で設定する

	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
	//gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト専用アドレス
	//gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数


	//デフォルトのサンプルマスクを表す定数(0xffffffff)
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//まだアンチエイリアスは使わないためfalse
	gpipeline.RasterizerState.MultisampleEnable = false;

	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	//ひとまず加算や乗算やαブレンディングは使用しない
	renderTargetBlendDesc.BlendEnable = false;
	//ひとまず論理演算は使用しない
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列の要素数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline.NumRenderTargets = 1;//今回は一つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;
	gpipeline.SampleDesc.Quality = 0;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange = {};
	descTblRange.NumDescriptors = 1;//テクスチャ1つ
	descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別はテクスチャ
	descTblRange.BaseShaderRegister = 0;//0番スロットから
	descTblRange.OffsetInDescriptorsFromTableStart = 
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//ピクセルシェーダから見える
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//ディスクリプタレンジのアドレス
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
	//ディスクリプタレンジ数
	rootparam.DescriptorTable.NumDescriptorRanges = 1;	

	rootSignatureDesc.pParameters = &rootparam;//ルートパラメーターの先頭アドレス
	rootSignatureDesc.NumParameters = 1;//ルートパラメーター数

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行きの繰り返し
	samplerDesc.BorderColor =
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーの時は黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//線形補間
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;//ミップマップ最小値
	samplerDesc.ShaderVisibility = 
		D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダからのみ見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//オーバーサンプリングの際リサンプリングしない？

	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,//ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0,//ルートシグネチャバージョン
		&rootSigBlob,//シェーダーを作った時と同じ
		&errorBlob);//エラー処理も同じ

	ID3D12RootSignature* rootsignature = nullptr;
	result = _dev->CreateRootSignature(
		0,//nodemask 0でよい
		rootSigBlob->GetBufferPointer(),//シェーダーの時と同様
		rootSigBlob->GetBufferSize(),//シェーダーの時と同様
		IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();//不要になったので開放

	gpipeline.pRootSignature = rootsignature;

	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;//出力先の幅(ピクセル数)
	viewport.Height = window_height;//出力先の高さ(ピクセル数)
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値

	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + window_width;
	scissorrect.bottom = scissorrect.top + window_height;

	//WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(L"img/textest.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	auto img = scratchImg.GetImage(0, 0, 0);//生データ抽出

	//WriteToSubresourceで転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	//特殊な設定なのでdefaultでもuploadでもない
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	//ライトバック
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	//転送がL0、つまりCPU側から直接行う
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	//単一アダプターのため0
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;//RGBAフォーマット
	resDesc.Width = static_cast<UINT>(metadata.width);//幅
	resDesc.Height = static_cast<UINT>(metadata.height);//高さ
	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
	resDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0;//クオリティ最低
	resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//テクスチャ用指定
		nullptr,
		IID_PPV_ARGS(&texbuff));

	result = texbuff->WriteToSubresource(
		0,
		nullptr,//全領域へコピー
		img->pixels,//元データアドレス
		static_cast<UINT>(img->rowPitch),//1ラインサイズ
		static_cast<UINT>(img->slicePitch)//全サイズ
	);

	//中間バッファとしてのUploadヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProp = {};

	// マップ可能にするためUPLOADにする
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	// アップロード用に使用すること前提なのでUNKNOWNでよい
	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	uploadHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	uploadHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	//D3D12_RESOURCE_DESC resDesc = {};

	resDesc.Format = DXGI_FORMAT_UNKNOWN;// 単なるデータの塊なのでUNKNOWN
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;//単なるバッファとして設定

	resDesc.Width =
		AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
		* img->height;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;

	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;//連続したデータ
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	resDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0;

	//中間バッファ作成
	ID3D12Resource* uploadbuff = nullptr;

	result = _dev->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,//CPUから書き込み可能
		nullptr,
		IID_PPV_ARGS(&uploadbuff)
	);

	//テクスチャのためのヒープ設定
	//D3D12_HEAP_PROPERTIES texHeapProp = {};

	texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;//テクスチャ用
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	ID3D12DescriptorHeap* texDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	//シェーダーから見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//マスクは0
	descHeapDesc.NodeMask = 0;
	//ビューは今のところ１つだけ
	descHeapDesc.NumDescriptors = 1;
	//シェーダリソースビュー
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap));

	//リソース設定(変数は使いまわし)
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);//幅
	resDesc.Height = static_cast<UINT>(metadata.height);//高さ
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトは決定しない

	//ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,//コピー先
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	uint8_t* mapforImg = nullptr;//image->pixelsと同じ型にする
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);//マップ

	auto srcAddress = img->pixels;

	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	for (int y = 0; y < img->height; ++y)
	{
		std::copy_n(srcAddress,
			rowPitch,
			mapforImg); // コピー

		// 1行ごとのつじつまを合わせる
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}

	//std::copy_n(img->pixels,img->slicePitch,mapforImg);//コピー
	//uploadbuff->Unmap(0, nullptr);//アンマップ

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = 
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1

	_dev->CreateShaderResourceView(
		texbuff, //ビューと関連付けるバッファー
		&srvDesc, //先ほど設定したテクスチャ設定情報
		texDescHeap->GetCPUDescriptorHandleForHeapStart()//ヒープのどこに割り当てるか
	);

	D3D12_TEXTURE_COPY_LOCATION src = {};

	//コピー元(アップロード側)設定
	src.pResource = uploadbuff;//中間バッファ
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;//フットプリント指定
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

	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = texbuff;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore =
		D3D12_RESOURCE_STATE_COPY_DEST;//ここが重要
	BarrierDesc.Transition.StateAfter =
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;//ここも重要

	_cmdList->ResourceBarrier(1, &BarrierDesc);
	_cmdList->Close();

	//コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);
	////待ち
	_cmdQueue->Signal(_fence, ++_fenceVal);

	_cmdAllocator->Release();
	_cmdList->Reset(_cmdAllocator, nullptr);

	// 課題用ここから
	Vertex vertices2[] = {
		{{-0.85f,-0.4f,0.0f},{0.0f,1.0f}},//左下
		{{-0.85f,+0.4f,0.0f},{0.0f,0.0f}},//左上
		{{-0.25f,-0.4f,0.0f},{1.0f,1.0f}},//右下
		{{-0.25f,+0.4f,0.0f},{1.0f,0.0f}},//右上
	};

	ID3D12Resource* vertBuff2 = nullptr;

	auto heapProp2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourcepDesc2 = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices2));
	result = _dev->CreateCommittedResource(
		&heapProp2,
		D3D12_HEAP_FLAG_NONE,
		&resourcepDesc2,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff2));

	Vertex* vertMap2 = nullptr;
	result = vertBuff2->Map(0, nullptr, (void**)&vertMap2);
	std::copy(std::begin(vertices2), std::end(vertices2), vertMap2);
	vertBuff2->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView2 = {};
	vbView2.BufferLocation = vertBuff2->GetGPUVirtualAddress();//バッファーの仮想アドレス
	vbView2.SizeInBytes = sizeof(vertices2);//全バイト数
	vbView2.StrideInBytes = sizeof(vertices2[0]);//１頂点あたりのバイト数

	unsigned short indices2[] = {
		0,1,2,
		2,1,3,
	};

	ID3D12Resource* idxBuff2 = nullptr;
	//設定は、バッファーのサイズ以外、頂点バッファーの設定を使い回してよい
	resourcepDesc2.Width = sizeof(indices2);
	result = _dev->CreateCommittedResource(
		&heapProp2,
		D3D12_HEAP_FLAG_NONE,
		&resourcepDesc2,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff2));

	//作ったバッファーにインデックスデータををコピー
	unsigned short* mapppedIdx2 = nullptr;
	idxBuff2->Map(0, nullptr, (void**)&mapppedIdx2);
	std::copy(std::begin(indices2), std::end(indices2), mapppedIdx2);
	idxBuff2->Unmap(0, nullptr);

	//インデックスバッファービューを作成
	D3D12_INDEX_BUFFER_VIEW ibView2 = {};
	ibView2.BufferLocation = idxBuff2->GetGPUVirtualAddress();
	ibView2.Format = DXGI_FORMAT_R16_UINT;
	ibView2.SizeInBytes = sizeof(indices2);


	D3D12_INPUT_ELEMENT_DESC inputLayout2[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{//uv(追加)
			"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,
			0,D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline2 = {};

	gpipeline2.pRootSignature = nullptr;

	gpipeline2.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline2.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline2.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline2.PS.BytecodeLength = _psBlob->GetBufferSize();

	//デフォルトのサンプルマスクを表す定数（0xffffffff）
	gpipeline2.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//まだアンチエイリアスは使わないため（false）
	gpipeline2.RasterizerState.MultisampleEnable = false;

	gpipeline2.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline2.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline2.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

	gpipeline2.BlendState.AlphaToCoverageEnable = false;
	gpipeline2.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc2 = {};
	//ひとまず加算や乗算やαブレンディングは使用しない
	renderTargetBlendDesc2.BlendEnable = false;
	//ひとまず論理演算は使用しない
	renderTargetBlendDesc2.LogicOpEnable = false;
	renderTargetBlendDesc2.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline2.BlendState.RenderTarget[0] = renderTargetBlendDesc2;

	gpipeline2.InputLayout.pInputElementDescs = inputLayout2;//レイアウト先頭アドレス
	gpipeline2.InputLayout.NumElements = _countof(inputLayout2);//レイアウト配列数

	gpipeline2.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし

	gpipeline2.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline2.NumRenderTargets = 1;//今は一つのみ
	gpipeline2.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//0～１に正規化されたRGBA

	gpipeline2.SampleDesc.Count = 1;//サンプリングは１ピクセルにつき１
	gpipeline2.SampleDesc.Quality = 0;//クオリティは最低　　

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc2 = {};
	rootSignatureDesc2.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange2 = {};
	descTblRange2.NumDescriptors = 1;//テクスチャは一つ
	descTblRange2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別はテクスチャ
	descTblRange2.BaseShaderRegister = 0;//0番スロットから
	descTblRange2.OffsetInDescriptorsFromTableStart
		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam2 = {};
	rootparam2.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//ピクセルシェーダーから見える
	rootparam2.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//ディスクリプタレンジのアドレス
	rootparam2.DescriptorTable.pDescriptorRanges = &descTblRange2;
	//ディスクリプタレンジ数
	rootparam2.DescriptorTable.NumDescriptorRanges = 1;

	rootSignatureDesc2.pParameters = &rootparam2;
	rootSignatureDesc2.NumParameters = 1;

	D3D12_STATIC_SAMPLER_DESC samplerDesc2 = {};
	samplerDesc2.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横方向の繰り返し
	samplerDesc2.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦方向の繰り返し
	samplerDesc2.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行きの繰り返し
	samplerDesc2.BorderColor =
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーは黒
	samplerDesc2.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//線形補間 LINEAR; 
	samplerDesc2.MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
	samplerDesc2.MinLOD = 0.0f;//ミップマップ最小値
	samplerDesc2.ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダーから見える
	samplerDesc2.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//リサンプリングしない

	rootSignatureDesc2.pStaticSamplers = &samplerDesc2;
	rootSignatureDesc2.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob2 = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc2,//ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0,//ルートシグネチャバージョン
		&rootSigBlob2,//シェーダーを作ったときと同じ
		&errorBlob);//エラー処理も同じ

	ID3D12RootSignature* rootsignature2 = nullptr;
	result = _dev->CreateRootSignature(
		0,//nodemask。０でよい
		rootSigBlob2->GetBufferPointer(),//シェーダーのときと同様
		rootSigBlob2->GetBufferSize(),//シェーダーのときと同様
		IID_PPV_ARGS(&rootsignature2));
	rootSigBlob2->Release();//不要になったので解放

	gpipeline2.pRootSignature = rootsignature2;

	ID3D12PipelineState* _pipelinestate2 = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline2, IID_PPV_ARGS(&_pipelinestate2));

	D3D12_VIEWPORT viewport2 = {};
	viewport2.Width = window_width;
	viewport2.Height = window_height;
	viewport2.TopLeftX = 0;
	viewport2.TopLeftY = 0;
	viewport2.MaxDepth = 1.0f;
	viewport2.MinDepth = 0.0f;

	D3D12_RECT scissorrect2 = {};
	scissorrect2.top = 0;
	scissorrect2.left = 0;
	scissorrect2.right = scissorrect2.left + window_width;
	scissorrect2.bottom = scissorrect2.top + window_height;

	//WICテクスチャ
	TexMetadata metadata2 = {};
	ScratchImage scratchImag2 = {};

	result = LoadFromWICFile(
		L"img/wood.png", WIC_FLAGS_NONE,
		&metadata2, scratchImag2
	);
	auto img2 = scratchImag2.GetImage(0, 0, 0);//生データ抽出

	//中間バッファーとしてのアップロードヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProp2 = {};

	//マップ可能にするため、UPLOADにする
	uploadHeapProp2.Type = D3D12_HEAP_TYPE_UPLOAD;

	//アップロード用に使用すること前提なのでUNKNOWNでよい。
	uploadHeapProp2.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp2.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	uploadHeapProp2.CreationNodeMask = 0;//単一アダプターのため()
	uploadHeapProp2.VisibleNodeMask = 0;//単一アダプターのため()

	D3D12_RESOURCE_DESC resDesc2 = {};
	resDesc2.Format = DXGI_FORMAT_UNKNOWN;//単なるデータの塊なのでUNKNOWN
	resDesc2.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;//単なるバッファーとして指定
	resDesc2.Width = AlignmentedSize(img2->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img2->height;//データサイズ
	resDesc2.Height = 1;
	resDesc2.DepthOrArraySize = 1;
	resDesc2.MipLevels = 1;
	resDesc2.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;//連携したデータ
	resDesc2.Flags = D3D12_RESOURCE_FLAG_NONE;//特にflagなし
	resDesc2.SampleDesc.Count = 1;//通常テクスチャなのでアンチエイリアシングしない
	resDesc2.SampleDesc.Quality = 0;//クオリティは最低

	//中間バッファーの作成
	ID3D12Resource* uploadbuff2 = nullptr;
	result = _dev->CreateCommittedResource(
		&uploadHeapProp2,
		D3D12_HEAP_FLAG_NONE,
		&resDesc2,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadbuff2)
	);

	//子テクスチャのためのヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp2 = {};
	texHeapProp2.Type = D3D12_HEAP_TYPE_DEFAULT;//子テクスチャ用
	texHeapProp2.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texHeapProp2.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texHeapProp2.CreationNodeMask = 0;//単一アダプターのため0
	texHeapProp2.VisibleNodeMask = 0;//単一アダプターのため0

	//リソース設定(使いまわし)
	resDesc2.Format = metadata2.format;
	resDesc2.Width = static_cast<UINT>(metadata2.width);//幅
	resDesc2.Height = static_cast<UINT>(metadata2.height);//高さ
	resDesc2.DepthOrArraySize = static_cast<uint16_t>(metadata2.arraySize);
	resDesc2.MipLevels = static_cast<uint16_t>(metadata2.mipLevels);
	resDesc2.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata2.dimension);
	resDesc2.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトは決定しない

	ID3D12Resource* texbuff2 = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp2,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc2,
		D3D12_RESOURCE_STATE_COPY_DEST,//コピー先
		nullptr,
		IID_PPV_ARGS(&texbuff2));

	uint8_t* mapforImg2 = nullptr;//image->pixelsと同じ型にする
	result = uploadbuff2->Map(0, nullptr, (void**)&mapforImg2);//マップ

	auto srcAddress2 = img2->pixels;
	auto rowPitch2 = AlignmentedSize(img2->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	for (int y = 0; y < img2->height; ++y)
	{
		std::copy_n(srcAddress2, rowPitch2, mapforImg2); // コピー

		// 1 行ごとのつじつまを合わせる
		srcAddress2 += img2->rowPitch;
		mapforImg2 += rowPitch2;
	}

	uploadbuff2->Unmap(0, nullptr);//アンマップ

	D3D12_TEXTURE_COPY_LOCATION src2 = {};
	//コピー元(アップロード側)設定
	src2.pResource = uploadbuff2;//中間バッファー
	src2.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src2.PlacedFootprint.Offset = 0;
	src2.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata2.width);
	src2.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata2.height);
	src2.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata2.depth);
	src2.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(AlignmentedSize(img2->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	src2.PlacedFootprint.Footprint.Format = img2->format;

	D3D12_TEXTURE_COPY_LOCATION dst2 = {};
	//コピー先設定
	dst2.pResource = texbuff2;
	dst2.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst2.SubresourceIndex = 0;

	_cmdList->CopyTextureRegion(&dst2, 0, 0, 0, &src2, nullptr);

	D3D12_RESOURCE_BARRIER BarrierDesc2 = {};
	BarrierDesc2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc2.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc2.Transition.pResource = texbuff2;
	BarrierDesc2.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc2.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	BarrierDesc2.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	_cmdList->ResourceBarrier(1, &BarrierDesc2);
	_cmdList->Close();

	//コマンドリストの実行
	ID3D12CommandList* cmdlists2[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists2);

	_cmdQueue->Signal(_fence, ++_fenceVal);
	if (_fence->GetCompletedValue() != _fenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	_cmdAllocator->Reset();
	_cmdList->Reset(_cmdAllocator, nullptr);


	result = texbuff2->WriteToSubresource(
		0,
		nullptr,//全領域へのコピー
		img2->pixels,//元のデータアドレス
		static_cast<UINT>(img2->rowPitch),//1ラインサイズ
		static_cast<UINT>(img2->slicePitch)//全サイズ
	);

	ID3D12DescriptorHeap* texDescHeap2 = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc2 = {};
	//シェーダーからは見えるように
	descHeapDesc2.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//マスクは0
	descHeapDesc2.NodeMask = 0;
	//ビューは今のところ1つだけ
	descHeapDesc2.NumDescriptors = 1;
	//シェーダーリソースビュー用
	descHeapDesc2.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc2, IID_PPV_ARGS(&texDescHeap2));

	D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc2 = {};
	srcDesc2.Format = metadata2.format;
	srcDesc2.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
	srcDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srcDesc2.Texture2D.MipLevels = 1;//ミップマップは使用しないので1

	_dev->CreateShaderResourceView(
		texbuff2,//ビューと関連付けるバッファー
		&srcDesc2,//先ほど設定したテクスチャ設定情報
		texDescHeap2->GetCPUDescriptorHandleForHeapStart()//ヒープのどこを割り当てるか
	);
	//課題用ここまで

	if (_fence->GetCompletedValue() != _fenceVal)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	_cmdAllocator->Reset();//キューをクリア
	_cmdList->Reset(_cmdAllocator, nullptr);

	while (true)
	{
		MSG msg = {};
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
		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		_cmdList->SetPipelineState(_pipelinestate);

		auto BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		_cmdList->ResourceBarrier(1, &BarrierDesc);//バリア指定実行
		//レンダ―ターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);
		//画面クリア
		//float clearColor[] = { 1.0f,1.0f,0.0f,1.0f };//黄色

		//前後だけ入れ替える
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->SetDescriptorHeaps(1, &texDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(
			0, // ルートパラメーターインデックス
			texDescHeap->GetGPUDescriptorHandleForHeapStart()); // ヒープアドレス
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		_cmdList->DrawIndexedInstanced(15, 1, 0, 0, 0);

		//課題用 ここから
		_cmdList->SetGraphicsRootSignature(rootsignature2);
		_cmdList->SetDescriptorHeaps(1, &texDescHeap2);
		_cmdList->SetGraphicsRootDescriptorTable(
			0, // ルートパラメーターインデックス
			texDescHeap->GetGPUDescriptorHandleForHeapStart()); // ヒープアドレス
		_cmdList->RSSetViewports(1, &viewport2);
		_cmdList->RSSetScissorRects(1, &scissorrect2);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView2);
		_cmdList->IASetIndexBuffer(&ibView2);
		_cmdList->DrawIndexedInstanced(15, 1, 0, 0, 0);
		//ここまで

		//命令のクローズ
		_cmdList->Close();

		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		//待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);
		if (_fence->GetCompletedValue() != _fenceVal)
		{
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);//イベントハンドルの取得
			WaitForSingleObject(event, INFINITE);//イベントが発生するまで待つ
			CloseHandle(event);//イベントハンドルを閉じる
		}
		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, nullptr);//再びコマンドリストをためる準備
		//フリップ
		_swapchain->Present(1, 0);
	}
	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}