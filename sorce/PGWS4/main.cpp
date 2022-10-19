#include<Windows.h>
#include<Windowsnumerics.h>
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
	else if (*r >= 1.0f && *g < 1.0f)
	{
		*g += 0.01f;
	}
	//緑
	else if (*r >= 0.0f && *g >= 1.0f)
	{
		*r -= 0.01f;
	}
	//水色
	else if (*g >= 1.0f && *b < 1.0f && *r < 0.0f)
	{
		*b += 0.01f;
	}
	//青色
	else if (*b >= 1.0f && *r < 0.0f && *g >= 0.0f)
	{
		*g -= 0.01f;
	}
	//紫
	else if (*b >= 1.0f && *r < 1.0f && *g < 0.0f)
	{
		*r += 0.01f;
	}
	//黒
	else
	{
		*g += 0.01f;
	}

	//float3 col = saturate(abs(6 * t - float3(3, 2, 4)) * float3(1, -1, -1) + float3(-1,2 , - 2));

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
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
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
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));

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

	//四角形を生成する用--------------------------------------------------
	//頂点データ構造体
	struct Vertex
	{
		XMFLOAT3 pos; //xyz座標
		XMFLOAT2 uv;  //uv座標
	};
	//4つの頂点を定義
	Vertex vertices[]
	{
		//左下
		{{ -0.4f,-0.7f,0.0f},{0.0f,1.0f}},
		//左上
		{{-0.4f,+0.7f,0.0f},{0.0f,0.0f} },
		//右下
		{{+0.4f,-0.7f,0.0f},{1.0f,1.0f} },
		//右上
		{{+0.4f,+0.7f,0.0f},{1.0f,0.0f}},
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
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	//確保したメインメモリに頂点データをコピー
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
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

	result = D3DCompileFromFile(
		//シェーダー名
		L"BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&_psBlob, &errorBlob);

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
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//uv
		{
			"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,
			0,D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
	};

	//頂点シェーダー・ピクセルシェーダーを作成------------------------------------------
	//シェーダーの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	gpipeline.pRootSignature = nullptr; //後で設定

	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	//デフォルトのサンプルマスクを表す定数(0xffffffff)
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//まだアンチエイリアスは使わない為のfalse
	gpipeline.RasterizerState.MultisampleEnable = false;
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

	//ブレンドステートの設定
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	//加算や乗算やaブレンディング設定
	renderTargetBlendDesc.BlendEnable = false;
	//倫理演算設定
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	//入力レイアウトの設定
	gpipeline.InputLayout.pInputElementDescs = inputLayout; //レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout); //レイアウト配列の要素数
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; //ストリップ時のカット無し
	//ポリゴンは三角形　細分化用、多角形のデータ用は_PATCH
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; //三角形で構成

	//その他設定
	//レンダーターゲットの設定
	gpipeline.NumRenderTargets = 1;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; //0～1に正規化されたRGBA
	//アンチエイリアスのサンプル数設定
	gpipeline.SampleDesc.Count = 1; //サンプリングは1ピクセルに付き1
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低
	//-----------------------------------------------------------------------------------

	//ルートシグネチャでのディスクリプタの指定-------------------------------------------
	//空のルートシグネチャ(頂点情報のみ)
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//ディスクリプタレンジ
	//ルートシグネチャにスロットとテクスチャの関連を記述する
	//ディスクリプタテーブル：定数バッファーやテクスチャなどのレジスタ番号の指定をまとめているもの
	//ルートシグネチャの追加
	D3D12_DESCRIPTOR_RANGE descTblRange = {};
	descTblRange.NumDescriptors = 1; //テクスチャ1つ
	descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; //種別はテクスチャ
	descTblRange.BaseShaderRegister = 0; //0番スロットから
	descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//ピクセルシェーダーから見える
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//ディスクリプタレンジのアドレス
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
	//ディスクリプタレンジ数
	rootparam.DescriptorTable.NumDescriptorRanges = 1;

	//テクスチャ用
	rootSignatureDesc.pParameters = &rootparam; //ルータパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 1;//ルートパラメーター数

	//サンプラーの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	//横方向の繰り返し
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//縦方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//奥行きの繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//ボーダーは黒
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	//線形補間
	//samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; //補間しない
	//ミニマップ最大値
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	//ミニマップ最小値
	samplerDesc.MinLOD = 0.0f;
	//ピクセルシェーダーから見える
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//リサンプリングしない
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	//ルートシグネチャオブジェクトの作成・設定
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, //ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0, //ルートシグネチャバージョン
		&rootSigBlob, //シェーダーを作ったときと同じ
		&errorBlob);//エラー処理も同様

	ID3D12RootSignature* rootsignature = nullptr;
	result = _dev->CreateRootSignature(
		0, //nodemask
		rootSigBlob->GetBufferPointer(),//シェーダーの時と同様
		rootSigBlob->GetBufferSize(), //シェーダーの時と同様
		IID_PPV_ARGS(&rootsignature));
	//不要になったので解放
	rootSigBlob->Release();

	gpipeline.pRootSignature = rootsignature;

	ID3D12PipelineState* _piplinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_piplinestate));
	//-----------------------------------------------------------------------------------

	//ビューポート-----------------------------------------------------------------------
	//出力されるRTのスクリーン座標系での位置と大きさ
	//・Depthは深度バッファ
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width; //出力先の幅(ピクセル数)
	viewport.Height = window_height; //出力先の高さ(ピクセル数)
	viewport.TopLeftX = 0; //出力先左上座標X
	viewport.TopLeftY = 0; //出力先左上Y
	viewport.MaxDepth = 1.0f; //深度最大値
	viewport.MinDepth = 0.0f; //深度最小値

	//シザー矩形：ビューポートの中で実際に描画される範囲
	D3D12_RECT scissorrect = {};
	scissorrect.top = 0; //切り抜き上座標
	scissorrect.left = 0;//切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;//切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;//切り抜き下座標
	//-----------------------------------------------------------------------------------

	//インデックスバッファの作成---------------------------------------------------------
	//インデックスを導入
	unsigned short indices[] =
	{
		0,1,2,
		2,1,3,
	};
	ID3D12Resource* idxBuff = nullptr;
	//設定はバッファーサイズ以外頂点バッファーの設定を使いまわして良い
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	//作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//インデックスバッファービューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	//プロシージャルテクスチャ-------------------------------------------
	struct TexRGBA
	{
		unsigned char R, G, B, A;
	};

	std::vector<TexRGBA>texturedata(256 * 256);
	for (auto& rgba : texturedata)
	{
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = 255; //aは1.0
	}
	//-------------------------------------------------------------------
	//テクスチャバッファー---------------------------------------------
	//ヒープの設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	//ライトバック
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	//転送はL0->CPU側から直接行う
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	//単一アダプターのための0
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	//リソースの設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //RGBAフォーマット
	resDesc.Width = 256;
	resDesc.Height = 256;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//リソースの生成
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff));
	//-----------------------------------------------------------------
	//テクスチャバッファーにデータ転送-----------------
	result = texbuff->WriteToSubresource(
		0,
		nullptr,
		texturedata.data(),
		sizeof(TexRGBA) * 256,
		sizeof(TexRGBA) * (UINT)texturedata.size()
	);
	//--------------------------------------------------
	//シェーダーリソースビュー--------------------------------------
	//ディスクリプタヒープを作成
	ID3D12DescriptorHeap* texDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	//シェーダーから見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//マスクは0
	descHeapDesc.NodeMask = 0;
	//ビューは一つ
	descHeapDesc.NumDescriptors = 1;
	//シェーダーリソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap));
	
	//シェーダーリソースビューを作る
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //RGBAを正規化
	srvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;//ミニマップ使用しない

	_dev->CreateShaderResourceView(
		texbuff,
		&srvDesc,
		texDescHeap->GetCPUDescriptorHandleForHeapStart()//ヒープのどこに割り当てるか
	);
	//------------------------------------------------------------------------------

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

		//パイプラインステートのセット
		_cmdList->SetPipelineState(_piplinestate);

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

		//画面クリア
		float clearColor[] = { 0.0f,0.0f,0.3f, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		//ルートシグネチャのセット
		_cmdList->SetGraphicsRootSignature(rootsignature);
		//ディスクリプタヒープの指定
		_cmdList->SetDescriptorHeaps(1, &texDescHeap);
		//ルートパラメーターとディスクリプタヒープの関連付け
		_cmdList->SetGraphicsRootDescriptorTable(
			//ルートパラメーターインデックス
			0,
			//ヒープアドレス
			texDescHeap->GetGPUDescriptorHandleForHeapStart());

		//多角形制作
		//ビューポートとシザー矩形のセット
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		//プリミティブトポロジのセット
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//頂点情報のセット
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		//描画命令(index数)
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

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
