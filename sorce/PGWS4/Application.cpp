#include "Application.h"
#include <tchar.h>
#include <assert.h>
#include <vector>
#include <exception>
#include <d3dcompiler.h>

#pragma comment (lib, "DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

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

//モデルとパスとテクスチャのパスから合成パスを得る
//@param modelPath アプリケーションから見たpmdモデルのパス
//@param texPath PMDモデルから見たテクスチャのパス
//@return アプリケーションから見たテクスチャのパス
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath)
{
	int pathIndex1 = static_cast<int>(modelPath.rfind('/'));
	int pathIndex2 = static_cast<int>(modelPath.rfind('\\'));
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = modelPath.substr(0, pathIndex + 1);
	return folderPath + texPath;
}

//ファイル名から拡張子取得
std::string GetExtension(const std::string& path)
{
	int idx = static_cast<int>(path.rfind('.'));
	return path.substr(idx + 1, path.length() - idx - 1);
}

//テクスチャのパスをセパレーター文字で分離する
std::pair<std::string, std::string>SplitFileName(
	const std::string& path, const char splitter = '*')
{
	//[*]等の位置を調べる
	int idx = static_cast<int>(path.find(splitter));
	std::pair<std::string, std::string> ret;
	//「*」等の前の文字列
	ret.first = path.substr(0, idx);
	//「*」等の後の文字列
	ret.second = path.substr(idx + 1, path.length() - idx - 1);

	return ret;
}

//Char文字列からwchar_t文字列への変換
std::wstring GetWideStringFromString(const std::string& str)
{
	//呼び出し1回目
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr;
	wstr.resize(num1);

	//呼び出し2回目
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1);

	//一応チェック
	assert(num1 == num2);
	return wstr;
}

//テクスチャロード
ComPtr<ID3D12Resource> Application::LoadTextureFromFile(std::string& texPath, ID3D12Device* dev)
{
	//ファイル名とパスとリソースのマップテーブル

	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end())
	{
		//テーブル内にあったらロードするのではなくマップ内のリソースを返す
		return it->second;
	}

	//WICテクスチャのロード

	if (_loadLambdaTable.empty())
	{
		_loadLambdaTable["sph"]
			= _loadLambdaTable["spa"]
			= _loadLambdaTable["bmp"]
			= _loadLambdaTable["png"]
			= _loadLambdaTable["jpg"]
			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
		{
			return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
		};
		_loadLambdaTable["tga"]
			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
		{
			return LoadFromTGAFile(path.c_str(), meta, img);
		};
		_loadLambdaTable["dds"]
			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
		{
			return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
		};
	}

	//以下エラーコード
	//テクスチャロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	//テクスチャのファイルパス
	std::wstring wtexpath = GetWideStringFromString(texPath);
	//拡張子を取得
	std::string ext = GetExtension(texPath);
	if (_loadLambdaTable.find(ext) == _loadLambdaTable.end()) {
		return nullptr;
	}
	auto result = _loadLambdaTable[ext](
		wtexpath,
		&metadata,
		scratchImg);
	if (FAILED(result)) { return nullptr; }

	//WriteToSubresource1で転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	const D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		static_cast<UINT>(metadata.width),
		static_cast<UINT>(metadata.height),
		static_cast<UINT16>(metadata.arraySize),
		static_cast<UINT16>(metadata.mipLevels));

	//バッファー作成
	ComPtr<ID3D12Resource> texbuff = nullptr;
	result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) { return nullptr; }

	//生データ湧出
	const Image* img = scratchImg.GetImage(0, 0, 0);

	result = texbuff->WriteToSubresource(
		0,
		nullptr,
		img->pixels,
		static_cast<UINT>(img->rowPitch), //1ラインサイズ
		static_cast<UINT>(img->slicePitch)//全サイズ
	);

	if (FAILED(result)) { return nullptr; }

	_resourceTable[texPath] = texbuff.Get();

	return texbuff.Get();
}

ComPtr<ID3D12Resource> CreateMonoTexture(ID3D12Device* dev, unsigned int val)
{
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	auto result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(whiteBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) { return nullptr; }

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), val); // 全部valで埋める

	// データ転送
	result = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		static_cast<UINT>(data.size()));

	return whiteBuff;
}

ComPtr<ID3D12Resource> CreateWhiteTexture(ID3D12Device* dev)
{
	return CreateMonoTexture(dev, 0xff);
}

ComPtr<ID3D12Resource> CreateBlackTexture(ID3D12Device* dev)
{
	return CreateMonoTexture(dev, 0x00);
}

//デフォルトグラデーションテクスチャ
ComPtr<ID3D12Resource> CreateGrayGradationTexture(ID3D12Device* dev)
{
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	ComPtr<ID3D12Resource> gradBuff = nullptr;
	HRESULT result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(gradBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) { return nullptr; }

	//うえがしろき下が黒いテクスチャデータを作成
	std::vector<unsigned int>data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		//RGBAが逆並びの為RGBマクロと0xff<<24を用いて表す
		unsigned int col = (0xff) << 24 | RGB(c, c, c);
		fill(it, it + 4, col);
		--c;
	}
	result = gradBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * static_cast<UINT>(sizeof(unsigned int)),
		static_cast<UINT>(sizeof(unsigned int) * data.size()));

	return gradBuff;
}

//アライメントにそろえたサイズを返す
size_t AlignmentedSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}

#ifdef _DEBUG
void EnableDebugLayer()
{
	ComPtr<ID3D12Debug> debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	if (!SUCCEEDED(result)) return;

	//デバッグレイヤーを有効にする
	debugLayer->EnableDebugLayer();
	//有効かしたらインターフェイスを解放する
	debugLayer->Release();
}
#endif// _DEBUG

//ウィンドウ生成
HWND Application::CreateGameWindow(WNDCLASSEX _windowClass,int _window_width,int _window_height)
{
	//クラス生成
	_windowClass.cbSize = sizeof(WNDCLASSEX);
	//コールバック関数の指定
	_windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;
	//アプリケーションクラス名
	_windowClass.lpszClassName = _T("DX12Sample");
	//ハンドルの取得
	_windowClass.hInstance = GetModuleHandle(nullptr);

	//アプリケーションクラス(ウィンドウクラスの指定をOSに伝える)
	RegisterClassEx(&_windowClass);

	//ウィンドウサイズを決める
	RECT wrc = { 0,0,(LONG)_window_width,(LONG)_window_height };

	//関数を使ってウィンドウのサイズを設定する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	return CreateWindow(
		//クラス名指定
		_windowClass.lpszClassName,
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
		_windowClass.hInstance,
		//追加パラメータ-
		nullptr);
}

//コマンドリスト作成
void Application::InitializeCommand()
{
	ThrowIfFailed(_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf())));
	ThrowIfFailed(_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocator.Get(), nullptr,
		IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf())));

	//コマンドキュー作成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	//タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//アダプターを1つしか使わない場合は0で可
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	//コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//キュー作成
	ThrowIfFailed(_dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf())));
}

//スワップチェーン作成
void Application::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = _window_width;
	swapchainDesc.Height = _window_height;
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
	ThrowIfFailed(_dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue.Get(), _hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)_swapchain.ReleaseAndGetAddressOf()));
}

//バックバッファの作成
void Application::InitializeBackBuffers()
{
	//スワップチェーンのバッファを取得
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	//レンダーターゲットビューなのでRTV
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	//裏表の2つ
	heapDesc.NumDescriptors = 3;
	//指定なし
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf())));
	//スワップチェーンのリソースとRTVを関連付ける
	//スワップチェーンの情報の取得
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	ThrowIfFailed(_swapchain->GetDesc(&swcDesc));

	//レンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//ガンマ補正なし
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//ディスクリプタをバッファの数だけ生成
	_backBuffers = std::vector<ComPtr<ID3D12Resource>>(swcDesc.BufferCount);
	for (UINT idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		//バッファのポインタを取得
		ThrowIfFailed(_swapchain->GetBuffer(idx, IID_PPV_ARGS(_backBuffers[idx].ReleaseAndGetAddressOf())));
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_rtvHeaps->GetCPUDescriptorHandleForHeapStart());

		handle.Offset(idx, _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		_dev->CreateRenderTargetView(_backBuffers[idx].Get(), &rtvDesc, handle);
	}
}

//深度バッファの作成
void Application::InitializeDepthBuffer()
{
	//深度バッファーの作成(ヘルパー構造体は何をしているのかわからなくなる為、使わない方がいい)
	D3D12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		_window_width, _window_height,
		1U, 1U, 1U, 0U, //arraySize,mipLevels,sampleCount,SampleQuality
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	//深度値用ヒーププロパティ
	const D3D12_HEAP_PROPERTIES depthHeapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	//重要　クリアする値
	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	ThrowIfFailed(_dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(_depthBuffer.ReleaseAndGetAddressOf())));

	// 深度の為のディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//深度に使う
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(_dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf())));

	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(
		_depthBuffer.Get(),
		&dsvDesc,
		_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

//テクスチャ系初期化
void Application::InitialixeTextureLoaderTable(FILE* fp, std::string strModelPath)
{
	//頂点を全て読み込む
	constexpr size_t pmdvertex_size = 38; //頂点一つ辺りのあサイズ

	unsigned int vertNum; //頂点数を取得
	fread(&vertNum, sizeof(vertNum), 1, fp);

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

	//バッファーの確保
	std::vector<PMD_VERTEX> vertices(vertNum);
	for (unsigned int i = 0; i < vertNum; i++)
	{
		//読み込み
		fread(&vertices[i], pmdvertex_size, 1, fp);
	}

	auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMD_VERTEX));

	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc, //サイズ変更
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_vertBuff.ReleaseAndGetAddressOf())));

	//頂点データをコピ-
	PMD_VERTEX* vertMap = nullptr; //型をVertexに変更
	ThrowIfFailed(_vertBuff->Map(0, nullptr, (void**)&vertMap));
	//確保したメインメモリに頂点データをコピー
	copy(begin(vertices), end(vertices), vertMap);
	//CPUで設定は完了したのでビデオメモリに転送
	_vertBuff->Unmap(0, nullptr);

	//頂点バッファビュー(頂点バッファの説明)
	//バッファーの仮想アドレス
	_vbView.BufferLocation = _vertBuff->GetGPUVirtualAddress();
	//全バイト数
	_vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX));
	//1頂点当たりのバイト数
	_vbView.StrideInBytes = sizeof(vertices[0]);

	//インデックスバッファの作成
	unsigned int indicesNum; //インデックス数
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	std::vector<unsigned short> indices;
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	//設定はバッファーサイズ以外頂点バッファーの設定を使いまわして良い
	resdesc.Width = indices.size() * sizeof(indices[0]);
	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_idxBuff.ReleaseAndGetAddressOf())));

	//作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	_idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	_idxBuff->Unmap(0, nullptr);

	//インデックスバッファービューを作成
	_ibView.BufferLocation = _idxBuff->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R16_UINT;
	_ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	//マテリアル数
	fread(&_materialNum, sizeof(_materialNum), 1, fp);

	std::vector<PMDMaterial>pmdMaterials(_materialNum);

	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp); //一気に読み込む

	materials.resize(pmdMaterials.size());
	//コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
		materials[i].additional.toonIdx = pmdMaterials[i].toonIdx;
	}

	//テクスチャ呼び出し処理
	_textureResources.resize(_materialNum);
	//スフィアマップの読み込み
	_sphResources.resize(_materialNum);
	//加算スフィアマップの読み込み
	_spaResources.resize(_materialNum);
	//トゥーンシェーダー
	_toonResources.resize(_materialNum);

	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		//トゥーンリソース読み込み
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, "toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		toonFilePath += toonFileName;
		_toonResources[i] = LoadTextureFromFile(toonFilePath, _dev.Get());

		if (strlen(pmdMaterials[i].texFilePath) == 0) { continue; }

		//[*]がある時だけ処理する
		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			//スプリッタがある
			auto namepair = SplitFileName(texFileName);
			//spaもスフィアマップ(加算スフィアマップ)
			if (GetExtension(namepair.first) == "sph")
			{
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else
			{
				//まずはテクスチャファースト
				texFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph")
				{
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa")
				{
					spaFileName = namepair.second;
				}
			}
		}
		else
		{
			std::string ext = GetExtension(pmdMaterials[i].texFilePath);
			//スフィアマップだけでアベル度がない場合にも対応
			if (ext == "sph")
			{
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (ext == "spa")
			{
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
		}

		//モデルとテクスチャパスからアプリケーション空のテクスチャパスを得る
		auto texFilePath = GetTexturePathFromModelAndTexPath(
			strModelPath,
			texFileName.c_str());
		_textureResources[i] = LoadTextureFromFile(texFilePath, _dev.Get());

		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
			_sphResources[i] = LoadTextureFromFile(sphFilePath, _dev.Get());
		}

		if (spaFileName != "")
		{
			auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
			_spaResources[i] = LoadTextureFromFile(spaFilePath, _dev.Get());
		}
	}

	//マテリアルバッファーを作成
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	const D3D12_HEAP_PROPERTIES heapPropMat =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC resDescMat = CD3DX12_RESOURCE_DESC::Buffer(
		materialBuffSize * _materialNum);
	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapPropMat,
		D3D12_HEAP_FLAG_NONE,
		&resDescMat,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_materialBuff.ReleaseAndGetAddressOf()))
	);

	//マップマテリアルにコピー
	char* mapMaterial = nullptr;
	ThrowIfFailed(_materialBuff->Map(0, nullptr, (void**)&mapMaterial));
	for (auto& m : materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; //データをコピー
		mapMaterial += materialBuffSize; //次のアライメント位置まで進める（256の倍数）
	}
	_materialBuff->Unmap(0, nullptr);

	//マテリアル用出来スクリプタヒープとビュ-の作成
	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;
	materialDescHeapDesc.NumDescriptors = _materialNum * 5;
	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ThrowIfFailed(_dev->CreateDescriptorHeap(
		&materialDescHeapDesc, IID_PPV_ARGS(_materialDescHeap.ReleaseAndGetAddressOf())));

	//ディスクリプタヒープ上にビューを作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};

	matCBVDesc.BufferLocation =
		_materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes =
		static_cast<UINT>(materialBuffSize);

	//通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension =
		D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	//先頭を記録
	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(
		_materialDescHeap->GetCPUDescriptorHandleForHeapStart());
	UINT incSize = _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//ビューを作成
	for (UINT i = 0; i < _materialNum; ++i)
	{
		//マテリアル用定数バッファービュー
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);

		matDescHeapH.ptr += incSize;
		matCBVDesc.BufferLocation += materialBuffSize;

		//シェーダーリソースビュー
		if (_textureResources[i] == nullptr)
		{
			srvDesc.Format = _whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(_whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = _textureResources[i]->GetDesc().Format;

			_dev->CreateShaderResourceView(
				_textureResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += incSize;

		if (_sphResources[i] == nullptr)
		{
			srvDesc.Format = _whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(_whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = _sphResources[i]->GetDesc().Format;

			_dev->CreateShaderResourceView(
				_sphResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += incSize;

		if (_spaResources[i] == nullptr)
		{
			srvDesc.Format = _blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(_blackTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = _spaResources[i]->GetDesc().Format;

			_dev->CreateShaderResourceView(
				_spaResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += incSize;

		//トゥーンテクスチャ用のビュー作成
		if (_toonResources[i] == nullptr)
		{
			srvDesc.Format = _gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(_gradTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = _toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(_toonResources[i].Get(), &srvDesc, matDescHeapH);
		}

		matDescHeapH.ptr += incSize;
	}

}

//ルートシグネチャの追加
void Application::CreateRootSignature(ComPtr<ID3DBlob> errorBlob)
{
	HRESULT result;

	CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {};
	//座標変換用
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//マテリアル用
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//テクスチャ4つ
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	CD3DX12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);
	rootparam[1].InitAsDescriptorTable(2, &descTblRange[1]);

	//サンプラーの設定
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	//空のルートシグネチャ(頂点情報のみ)
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(
		2, rootparam,
		2, samplerDesc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//ルートシグネチャオブジェクトの作成・設定
	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, //ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0, //ルートシグネチャバージョン
		&rootSigBlob, //シェーダーを作ったときと同じ
		&errorBlob);//エラー処理も同様

	ThrowIfFailed(_dev->CreateRootSignature(
		0, //nodemask
		rootSigBlob->GetBufferPointer(),//シェーダーの時と同様
		rootSigBlob->GetBufferSize(), //シェーダーの時と同様
		IID_PPV_ARGS(_rootsigunature.ReleaseAndGetAddressOf())));
}

ComPtr<ID3DBlob> LoadShader(LPCWSTR hlsl, LPCSTR basic, LPCSTR ver)
{
	ComPtr<ID3DBlob> blob;
	HRESULT result;

	ComPtr<ID3DBlob> errorBlob = nullptr;

	result = D3DCompileFromFile(
		//シェーダー名
		hlsl,
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		basic, ver,
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&blob, &errorBlob);

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

	return blob;
}

bool Application::Init()
{
	ThrowIfFailed(CoInitializeEx(0, COINIT_MULTITHREADED));

	//ウィンドウオブジェクトの生成
	_hwnd = CreateGameWindow(_windowClass,_window_width,_window_height);

#ifdef _DEBUG
	//デバッグレイヤーオン
	EnableDebugLayer();
#endif // DEBUG
	//D3D12Deviceの生成
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

#ifdef _DEBUG
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf())));
#else
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf())));
#endif // _DEBUG

	//アダプターの列挙と検索
	//アダプター列挙用
	std::vector<IDXGIAdapter*> adapters;
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
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			break;
		}
	}

	//コマンドリスト作成
	InitializeCommand();

	//スワップチェーンの作成
	CreateSwapChain();
	//バックバッファ
	InitializeBackBuffers();
	//深度バッファー
	InitializeDepthBuffer();

	//フェンスオブジェクトの宣言と生成（待ちで必要）
	ThrowIfFailed(_dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())));

	//デフォルトグラデーションテクスチャ
	_gradTex = CreateGrayGradationTexture(_dev.Get());
	//ホワイトテクスチャ呼び出し
	_whiteTex = CreateWhiteTexture(_dev.Get());
	//黒テクスチャ作成
	_blackTex = CreateBlackTexture(_dev.Get());

	//モデル系
	//構造体の読み込み
	struct PMDHeader
	{
		float version; //例 00 00 80 3F == 1.00
		char model_name[20]; //モデル名
		char comment[256]; //モデルコメント
	};

	char signature[3] = {}; //シグネチャ
	PMDHeader pmdheader = {};

	std::string strModelPath = "Model/初音ミク.pmd";
	FILE* fp;
	fopen_s(&fp, strModelPath.c_str(), "rb");

	//fpがnullptrの対応をしてない警告
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	//テクスチャ系
	InitialixeTextureLoaderTable(fp, strModelPath);

	//閉じる
	fclose(fp);

	ComPtr<ID3DBlob> errorBlob = nullptr;

	ComPtr<ID3DBlob> _vsBlob = LoadShader(L"BasicVertexShader.hlsl", "BasicVS", "vs_5_0");
	ComPtr<ID3DBlob> _psBlob = LoadShader(L"BasicPixelShader.hlsl", "BasicPS", "ps_5_0");

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

	//頂点シェーダー・ピクセルシェーダーを作成
	//シェーダーの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	gpipeline.pRootSignature = nullptr; //後で設定

	gpipeline.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());

	//デフォルトのサンプルマスクを表す定数(0xffffffff)
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//まだアンチエイリアスは使わない為のfalse
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//深度ステンシル
	gpipeline.DepthStencilState.DepthEnable = true;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//書き込み

	gpipeline.DepthStencilState.DepthFunc =
		D3D12_COMPARISON_FUNC_LESS;//小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//ブレンドステートの設定
	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

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

	//ルートシグネチャでのディスクリプタの指定
	//ディスクリプタレンジ
	//ルートシグネチャにスロットとテクスチャの関連を記述する
	//ディスクリプタテーブル：定数バッファーやテクスチャなどのレジスタ番号の指定をまとめているもの
	//ルートシグネチャの追加
	CreateRootSignature(errorBlob.Get());

	gpipeline.pRootSignature = _rootsigunature.Get();

	ThrowIfFailed(_dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf())));

	//ビューポート
	//出力されるRTのスクリーン座標系での位置と大きさ
	//Depthは深度バッファ
	_viewport = CD3DX12_VIEWPORT(_backBuffers[0].Get());
	//シザー矩形：ビューポートの中で実際に描画される範囲
	_scissorrect = CD3DX12_RECT(0, 0, _window_width, _window_height);

	//WICテクスチャロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	ThrowIfFailed(LoadFromWICFile(
		L"img/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg));

	//生データ抽出
	auto img = scratchImg.GetImage(0, 0, 0);

	//中間バッファーとしてのロードヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

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

	//中間バッファー作成
	ComPtr<ID3D12Resource> uploadbuff = nullptr;

	ThrowIfFailed(_dev->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadbuff.ReleaseAndGetAddressOf())));

	//テクスチャバッファ
	//ヒープの設定
	const D3D12_HEAP_PROPERTIES texHeapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	//メタデータの再利用
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);
	resDesc.Height = static_cast<UINT>(metadata.height);
	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
	resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトは決定しない

	HRESULT result;
	//リソースの生成
	ComPtr<ID3D12Resource> texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf()));

	//アップロードリソースへのマップ
	uint8_t* mapforImg = nullptr;
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);//マップ

	//ズレの発症を抑える
	auto srcAddress = img->pixels;

	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	for (int y = 0; y < img->height; y++)
	{
		std::copy_n(srcAddress,
			rowPitch,
			mapforImg);//コピー

		//1行ごとのつじつまあわせ
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}

	//copy_n(img->pixels, img->slicePitch, mapforImg);//コピー
	uploadbuff->Unmap(0, nullptr);//アンマップ

	//CopyTextureReGion()を使うコード
	D3D12_TEXTURE_COPY_LOCATION src = {};
	//コピー元設定
	src.pResource = uploadbuff.Get();
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
	dst.pResource = texbuff.Get();
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	
	//バリアフェンス設定
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = texbuff.Get();
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore =
		D3D12_RESOURCE_STATE_COPY_DEST;// ここが重要
	BarrierDesc.Transition.StateAfter =
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;// ここも重要

	_cmdList->ResourceBarrier(1, &BarrierDesc);
	_cmdList->Close();

	//コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	//フェンス待ち
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
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
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);

	//回転制御
	_worldMat = XMMatrixRotationY(XM_PIDIV4);

	//ビュー行列
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	_viewMat = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	//プロジェクション行列
	_projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,//画角は90°
		static_cast<float>(_window_width)
		/ static_cast<float>(_window_height), //アスペクト比
		1.0f,//近いほう
		100.0f//遠いほう
	);

	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//256バイトアラインメント、下位8ビットが0であればそのまま
	//そうでなければ256単位で切り上げ
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_constBuff.ReleaseAndGetAddressOf())
	);

	//マップにより定数のコピー
	ThrowIfFailed(_constBuff->Map(0, nullptr, (void**)&_mapScene)); //マップ
	_mapScene->world = _worldMat;
	_mapScene->view = _viewMat;
	_mapScene->proj = _projMat;
	_mapScene->eye = eye;

	//定数バッファービューをディスクリプタヒープに追加
	//シェーダーリソースビュ
	//ディスクリプタヒープを作成
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
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(_basicDescHeap.ReleaseAndGetAddressOf()));

	//ディスクリプタの先頭ハンドルを取得しておく
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHeapHandle(_basicDescHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	//GPUの位置を指定
	cbvDesc.BufferLocation = _constBuff->GetGPUVirtualAddress();
	//コンスタントバッファービューがシェーダーリソースビューの後に入れるようにする
	cbvDesc.SizeInBytes = static_cast<UINT>(_constBuff->GetDesc().Width);

	//定数バッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

	return true;
}

void Application::Run()
{
	//ウィンドウ表示
	ShowWindow(_hwnd, SW_SHOW);

	//回転用角度
	float angle = 0.0f;
	float index = 0.0f;

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
		angle += 0.01f;
		_worldMat = XMMatrixRotationY(angle);
		_mapScene->world = _worldMat;
		_mapScene->view = _viewMat;
		_mapScene->proj = _projMat;

		//スワップチェーンの動作
		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		auto BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		_cmdList->ResourceBarrier(1, &BarrierDesc);
		//パイプラインステートのセット
		_cmdList->SetPipelineState(_pipelinestate.Get());

		//レンダーターゲットを指定
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvH(_rtvHeaps->GetCPUDescriptorHandleForHeapStart());
		rtvH.Offset(bbIdx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvH(_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);

		//画面クリア
		float clearColor[] = { 1.0f,1.0f,1.0f, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		//ルートシグネチャのセット
		_cmdList->SetGraphicsRootSignature(_rootsigunature.Get());
		//ディスクリプタヒープの指定
		_cmdList->SetDescriptorHeaps(1, _basicDescHeap.GetAddressOf());
		//ルートパラメーターとディスクリプタヒープの関連付け
		_cmdList->SetGraphicsRootDescriptorTable(0,
			_basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		//オブジェクト制作
		//ビューポートとシザー矩形のセット
		_cmdList->RSSetViewports(1, &_viewport);
		_cmdList->RSSetScissorRects(1, &_scissorrect);
		//プリミティブトポロジのセット
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//頂点情報のセット
		_cmdList->IASetVertexBuffers(0, 1, &_vbView);
		_cmdList->IASetIndexBuffer(&_ibView);

		//マテリアルの設定
		_cmdList->SetDescriptorHeaps(1, _materialDescHeap.GetAddressOf());

		auto materialH = _materialDescHeap->GetGPUDescriptorHandleForHeapStart();//ヒープ先頭

		unsigned int idx0ffset = 0; //最初はオフセットなし

		UINT cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

		for (Material& m : materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, materialH);

			_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idx0ffset, 0, 0);

			//ヒープポインターとインデックスを次に進める
			materialH.ptr += cbvsrvIncSize;
			idx0ffset += m.indicesNum;
		}

		//前後だけ入れ替える
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//命令のクローズ
		_cmdList->Close();

		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);

		//待ち
		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
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
		_cmdList->Reset(_cmdAllocator.Get(), _pipelinestate.Get());

		//フリップ
		_swapchain->Present(1, 0);
	}
}

void Application::Terminate()
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

	// もうクラスは使わないので登録解除する
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
