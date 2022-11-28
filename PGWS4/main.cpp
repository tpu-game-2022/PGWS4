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
//ライブラリ読み込み
#include<DirectXTex.h>
//コード減らし用
#include<d3dx12.h>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace std;
using namespace DirectX;

//@brief コンソール画面にフォーマット付き文字列を表示
//@param format フォーマット(%d、%f等)
//@param 可変長引数
//@remarks デバッグ用関数（デバッグ時にしか表示されない）

//ディスクリプタレンジ種類
typedef enum
{
	SRV,
	CBV,
	NONE,
}RangeNumber;

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

//アライメントにそろえたサイズを返す
size_t AlignmentedSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
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

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//ガンマ補正あり
	//rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//ガンマ補正なし
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

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
		_dev->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
	}

	//深度バッファーの作成
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension =
		D3D12_RESOURCE_DIMENSION_TEXTURE2D; //2次元のテクスチャデータ
	depthResDesc.Width = window_width;
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//重要　クリアする値
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer));
	//------------------------------------------------
	// 深度の為のディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//深度に使う
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
	//------------------------------------------------
	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(
		depthBuffer,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());
	//------------------------------------------------

	//フェンスオブジェクトの宣言と生成（待ちで必要）
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	//四角形を生成する用--------------------------------------------------
	//頂点データ構造体
#if 0
	struct Vertex
	{
		XMFLOAT3 pos; //xyz座標
		XMFLOAT2 uv;  //uv座標
	};
	//4つの頂点を定義
	Vertex vertices[]
	{
		//左下
		{{ -1.0f,-1.0f,0.0f},{0.0f,1.0f}},
		//左上
		{{-1.0f,+1.0f,0.0f},{0.0f,0.0f}},
		//右下
		{{+1.0f,-1.0f,0.0f},{1.0f,1.0f}},
		//右上
		{{+1.0f,+1.0f,0.0f},{1.0f,0.0f}},
	};
	//--------------------------------------------------------------------
		//頂点バッファの生成
	ID3D12Resource* vertBuff = nullptr;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourcepDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices));

	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourcepDesc,
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
#endif

	//モデル系
	// //------------------------------------------------------------------------------
	//構造体の読み込み
	struct PMDHeader
	{
		float version; //例 00 00 80 3F == 1.00
		char model_name[20]; //モデル名
		char comment[256]; //モデルコメント
	};

	char signature[3] = {}; //シグネチャ
	PMDHeader pmdheader = {};
	FILE* fp;
	fopen_s(&fp, "Model/初音ミク.pmd", "rb");

	//fpがnullptrの対応をしてない警告
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	//頂点を全て読み込む
	constexpr size_t pmdvertex_size = 38; //頂点一つ辺りのあサイズ

	unsigned int vertNum; //頂点数を取得
	fread(&vertNum, sizeof(vertNum), 1, fp);

#pragma pack(push,1)
	struct PMD_VERTEX
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
		uint16_t bone_no[2];
		uint8_t weight;
		uint8_t EdgeFlag;
		uint16_t dummy;
	};
#pragma pack(pop)
#pragma pack(1)
	//PMDマテリアル構造
	struct PMDMaterial
	{
		XMFLOAT3 diffuse;
		float alpha;
		float specularity;
		XMFLOAT3 specular;
		XMFLOAT3 ambient;
		unsigned char toonIdx;
		unsigned char edgeFlg;

		//2バイトのパディング

		unsigned int indicesNum;

		char texFilePath[20];
	};//72バイトになる//パッキング指定を解除
#pragma pack()

	//バッファーの確保
	vector<PMD_VERTEX> vertices(vertNum);
	for (unsigned int i = 0; i < vertNum; i++)
	{
		//読み込み
		fread(&vertices[i], pmdvertex_size, 1, fp);
	}

	//------------------------------------------------------------------------------

	//頂点バッファの生成
	ID3D12Resource* vertBuff = nullptr;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourcepDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMD_VERTEX));

	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourcepDesc, //サイズ変更
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	//頂点データをコピ-
	PMD_VERTEX* vertMap = nullptr; //型をVertexに変更
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	//確保したメインメモリに頂点データをコピー
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	//CPUで設定は完了したのでビデオメモリに転送
	vertBuff->Unmap(0, nullptr);
	//------------------------------------------------------------------------------

	//頂点バッファビュー(頂点バッファの説明)
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	//バッファーの仮想アドレス
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	//全バイト数
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX));
	//1頂点当たりのバイト数
	vbView.StrideInBytes = sizeof(vertices[0]);


	//インデックスバッファの作成---------------------------------------------------------
	unsigned int indicesNum; //インデックス数
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	std::vector<unsigned short> indices;
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	ID3D12Resource* idxBuff = nullptr;
	//設定はバッファーサイズ以外頂点バッファーの設定を使いまわして良い
	resourcepDesc.Width = indices.size() * sizeof(indices[0]);
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourcepDesc,
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
	ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	unsigned int materialNum; //マテリアル数
	fread(&materialNum, sizeof(materialNum), 1, fp);

	std::vector<PMDMaterial>pmdMaterials(materialNum);

	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp); //一気に読み込む

	//シェーダー側に投げられるマテリアルデータ
	struct MaterialForHlsl
	{
		//ディフューズ色
		XMFLOAT3 diffuse;
		float alpha;
		//スペキュラ
		XMFLOAT3 specular;
		float specularity;
		//アンビエント
		XMFLOAT3 ambient;
	};

	//それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	//全体をまとめるデータ（アプリ用）
	struct Material
	{
		unsigned int indicesNum;
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	vector<Material>materials(pmdMaterials.size());
	//コピー
	for (int i = 0; i < pmdMaterials.size(); i++)
	{
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
	}

	//マテリアルバッファーを作成
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	ID3D12Resource* materialBuff = nullptr;

	const D3D12_HEAP_PROPERTIES heapPropMat =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC resDescMat = CD3DX12_RESOURCE_DESC::Buffer(
		materialBuffSize * materialNum);
	result = _dev->CreateCommittedResource(
		&heapPropMat,
		D3D12_HEAP_FLAG_NONE,
		&resDescMat,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff)
	);

	//マップマテリアルにコピー
	char* mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto& m : materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; //データをコピー
		mapMaterial += materialBuffSize; //次のアライメント位置まで進める（256の倍数）
	}
	materialBuff->Unmap(0, nullptr);

	//マテリアル用出来スクリプタヒープとビュ＾の作成
	ID3D12DescriptorHeap* materialDescHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;
	materialDescHeapDesc.NumDescriptors = materialNum;
	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(
		&materialDescHeapDesc, IID_PPV_ARGS(&materialDescHeap));

	//ディスクリプタヒープ上にビューを作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};

	matCBVDesc.BufferLocation =
		materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes =
		static_cast<UINT>(materialBuffSize);
	//先頭を記録
	auto matDescHeapH =
		materialDescHeap->GetCPUDescriptorHandleForHeapStart();

	//ビューを作成
	for (UINT i = 0; i < materialNum; ++i)
	{
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr +=
			_dev->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		matCBVDesc.BufferLocation += materialBuffSize;
	}

	//閉じる
	fclose(fp);

	//-------------------------------------------------------------------

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

	//頂点データに合わせた頂点レイアウトの変更
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

	};

	//頂点レイアウト
	//D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	//{
	//	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
	//	D3D12_APPEND_ALIGNED_ELEMENT,
	//	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	//	//uv
	//	{
	//		"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,
	//		0,D3D12_APPEND_ALIGNED_ELEMENT,
	//		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
	//	},
	//};

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

	//深度ステンシル
	gpipeline.DepthStencilState.DepthEnable = true;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//書き込み
	gpipeline.DepthStencilState.DepthFunc =
		D3D12_COMPARISON_FUNC_LESS;//小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

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
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //0～1に正規化されたRGBA
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
	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};
	//テクスチャ用レジスター0番
	descTblRange[0].NumDescriptors = 1; //テクスチャ1つ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; //種別はテクスチャ
	descTblRange[0].BaseShaderRegister = 0; //0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//定数用レジスター1番
	descTblRange[1].NumDescriptors = 1; //テクスチャ1つ
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; //種別は定数
	descTblRange[1].BaseShaderRegister = 1; //1番スロットから
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//配列先頭アドレス
	rootparam[0].DescriptorTable.pDescriptorRanges = descTblRange;
	//ディスクリプタレンジ数
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;
	//全てのシェーダーから見えるようにする
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//配列先頭アドレス
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];
	//ディスクリプタレンジ数
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1;
	//全てのシェーダーから見えるようにする
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//ルートパラメーターの先頭アドレス
	rootSignatureDesc.pParameters = rootparam;
	//ルートパラメーター数
	rootSignatureDesc.NumParameters = 2;
	//-----------------------------------------------------------------------------------

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
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; //補間しない
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

	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	//-----------------------------------------------------------------------------------

	//シェーダー側に渡すための基本的な行列データ
	struct MatricesData
	{
		XMMATRIX world;
		XMMATRIX viewproj;
	};

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

	//テクスチャロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	result = LoadFromWICFile(
		L"img/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg);

	//生データ抽出
	auto img = scratchImg.GetImage(0, 0, 0);

	//--------------------------------------------------
	//中間バッファーとしてのロードヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProp = {};
	//マップ可能にする
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	uploadHeapProp.CreationNodeMask = 0;
	uploadHeapProp.VisibleNodeMask = 0;
	//--------------------------------------------------

	//メタデータの利用
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

	resDesc.Width =
		AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
		* img->height;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;

	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;

	//------------------------------------------------------------------------------
	//中間バッファー作成
	ID3D12Resource* uploadbuff = nullptr;

	result = _dev->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadbuff)
	);

	//--------------------------------------------------
	//テクスチャバッファー---------------------------------------------
	//ヒープの設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	//ライトバック
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//転送はL0->CPU側から直接行う
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//単一アダプターのための0
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	//------------------------------------------------------------------------------

	//メタデータの再利用
	resDesc.Format = metadata.format;

	resDesc.Width = static_cast<UINT>(metadata.width);
	resDesc.Height = static_cast<UINT>(metadata.height);
	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
	resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトは決定しない

	//リソースの生成
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texbuff));
	//-----------------------------------------------------------------

	//------------------------------------------------------------------------------------
	//アップロードリソースへのマップ
	uint8_t* mapforImg = nullptr;
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);//マップ

	//ズレの発症を抑える
	auto srcAddress = img->pixels;

	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	for (int y = 0; y < img->height; y++)
	{
		copy_n(srcAddress,
			rowPitch,
			mapforImg);//コピー

		//1行ごとのつじつまあわせ
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}

	//copy_n(img->pixels, img->slicePitch, mapforImg);//コピー
	uploadbuff->Unmap(0, nullptr);//アンマップ
	//--------------------------------------------------
	//CopyTextureReGion()を使うコード
	D3D12_TEXTURE_COPY_LOCATION src = {};
	//コピー元設定
	src.pResource = uploadbuff;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
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
	//------------------------------------------------------------------------------------
	//バリアフェンス設定
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = texbuff;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore =
		D3D12_RESOURCE_STATE_COPY_DEST;// ここが重要
	BarrierDesc.Transition.StateAfter =
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;// ここも重要

	_cmdList->ResourceBarrier(1, &BarrierDesc);
	_cmdList->Close();

	//コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	//フェンス待ち
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

	//--------------------------------------------------

	//回転制御
	XMMATRIX worldMat = XMMatrixRotationY(XM_PIDIV4);

	//ビュー行列
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	auto viewMat = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	//プロジェクション行列
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,//画角は90°
		static_cast<float>(window_width)
		/ static_cast<float>(window_height), //アスペクト比
		1.0f,//近いほう
		100.0f//遠いほう
	);

	//------------------------------------------------------------------------------
	ID3D12Resource* constBuff = nullptr;
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//256バイトアラインメント、下位8ビットが0であればそのまま
	//そうでなければ256単位で切り上げ
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);
	_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);
	//------------------------------------------------------------------------------
	//マップにより定数のコピー
	MatricesData* mapMatrix;
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix); //マップ
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;
	//------------------------------------------------------------------------------
	//定数バッファービューをディスクリプタヒープに追加
	//シェーダーリソースビュー--------------------------------------
	//ディスクリプタヒープを作成
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	//シェーダーから見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//マスクは0
	descHeapDesc.NodeMask = 0;
	//ビューSRV1ととCBV1つ(SRV→画像とかのテクスチャ CBV→位置など)
	descHeapDesc.NumDescriptors = 1;
	//シェーダーリソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));

	//ディスクリプタの先頭ハンドルを取得しておく
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	//GPUの位置を指定
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	//コンスタントバッファービューがシェーダーリソースビューの後に入れるようにする
	cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);

	//定数バッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
	//------------------------------------------------------------------------------

	//回転用角度
	float angle = 0.0f;

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

		//回転
		angle += 0.025f;
		worldMat = XMMatrixRotationY(angle);
		//ポリゴンの面を外側に廻す
		//worldMat = XMMatrixTranslation(1,2,3) * XMMatrixRotationY(angle);
		//ポリゴンは常に正面に向いて廻す
		//worldMat = XMMatrixRotationY(-angle) * XMMatrixTranslation(1, 2, 3) * XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;
		mapMatrix->viewproj = viewMat * projMat;

		//---------------------------------------------------------------------------
		//スワップチェーンの動作
		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		auto BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		_cmdList->ResourceBarrier(1, &BarrierDesc);
		//パイプラインステートのセット
		_cmdList->SetPipelineState(_pipelinestate);
		//---------------------------------------------------------------------------

		//レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);
		//---------------------------------------------------------------------------

		//画面クリア
		float clearColor[] = { 1.0f,1.0f,1.0f, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		//---------------------------------------------------------------------------

		//ルートシグネチャのセット
		_cmdList->SetGraphicsRootSignature(rootsignature);
		//ディスクリプタヒープの指定
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		//ルートパラメーターとディスクリプタヒープの関連付け
		_cmdList->SetGraphicsRootDescriptorTable(0,
			basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		//---------------------------------------------------------------------------

		//オブジェクト制作
		//ビューポートとシザー矩形のセット
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		//プリミティブトポロジのセット
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//頂点情報のセット
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		//描画命令(index数)
		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);
		//---------------------------------------------------------------------------
		_cmdList->IASetIndexBuffer(&ibView);

		//---------------------------------------------------------------------------
		//マテリアルの設定
		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		auto materialH = materialDescHeap->GetGPUDescriptorHandleForHeapStart();//ヒープ先頭

		unsigned int idx0ffset = 0; //最初はオフセットなし

		for (auto& m : materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, materialH);

			_cmdList->DrawIndexedInstanced(
				m.indicesNum, 1, idx0ffset, 0, 0);

			//ヒープポインターとインデックスを次に進める
			materialH.ptr +=
				_dev->GetDescriptorHandleIncrementSize(
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			idx0ffset += m.indicesNum;
		}
		//---------------------------------------------------------------------------

		//前後だけ入れ替える
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
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
		_cmdList->Reset(_cmdAllocator, _pipelinestate);

		_swapchain->Present(1, 0);
	}

	_cmdList->Close();
	_cmdAllocator->Reset(); // キューをクリア
	////待ち
	_cmdQueue->Signal(_fence, ++_fenceVal);

	if (_fence->GetCompletedValue() != _fenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);// イベントハンドルの取得
		WaitForSingleObject(event, INFINITE);// イベントが発生するまで無限に待つ
		CloseHandle(event);// イベントハンドルを閉じる
	}

	// これでは開放しきれない...
	_pipelinestate->Release();
	rootsignature->Release();
	_psBlob->Release();
	_vsBlob->Release();
	idxBuff->Release();
	vertBuff->Release();
	_fence->Release();
	rtvHeaps->Release();
	_swapchain->Release();
	_cmdQueue->Release();
	_cmdList->Release();
	_cmdAllocator->Release();
	_dxgiFactory->Release();
	_dev->Release();

#ifdef _DEBUG
	// メモリ未開放の詳細表示
	ID3D12DebugDevice* debugInterface;
	if (SUCCEEDED(_dev->QueryInterface(&debugInterface)))
	{
		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debugInterface->Release();
	}
#endif// _DEBUG

	// もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);


	//クラスはもう使用しないので登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
