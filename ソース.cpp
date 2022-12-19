#include<windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>
#include<map>
#include<d3dcompiler.h>
#include<DirectXTex.h>
#include<d3dx12.h>
#ifdef _DEBUG
#include<iostream>
#endif // DEBUG

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace std;
using namespace DirectX;

XMFLOAT3 vertices[3];

void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // DEBUG

}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) 
{
	if (msg == WM_DESTROY) 
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//「_DEBUG」のアンダーバーを忘れない！！！

#ifdef _DEBUG
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	if (!SUCCEEDED(result)) return;

	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}
//アライメントにそろえたサイズを返す
//@param size元のサイズ
//@param alignmantアライメントサイズ
//@returnアライメントをそろえたサイズ
size_t AlignmentedSize(size_t size, size_t alignment) {
	return size + alignment - size % alignment;
}
//モデルのパスとテクスチャのパスから合成パスを得る
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath)
{
	// ファイルのフォルダー区切りは\ と/ の2 種類が使用される可能性があり
	// ともかく末尾の\ か/ を得られればいいので、双方のrfind を取り比較する
	// （int 型に代入しているのは、見つからなかった場合は
	// rfind がepos(-1 → 0xffffffff) を返すため）
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');
	auto pathIndex = max(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex + 1);
	return folderPath + texPath;
}

//ファイル明から拡張子を取得する
//@param path 対象のパス文字列
//@return拡張子
std::string GetExtension(const std::string& path)
{
	int idx = static_cast<int>(path.rfind('.'));
	return path.substr(
		idx + 1, path.length() - idx - 1);
}

//テクスチャのパスをセパレーター文字で分離する
//@param path対象のパス文字列
//@param splitter区切り文字
//@return分離前後の文字列ペア
std::pair < std::string, std::string>SplitFileName(
	const std::string& path, const char splitter = '*') 
{
	int idx = static_cast<int>(path.find(splitter));
	pair<std::string, std::string>ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(
		idx + 1, path.length() - idx - 1);
	return ret;
}
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

	//呼び出し２回目
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
ID3D12Resource* LoadTextureFromFile(std::string& texPath, ID3D12Device* dev)
{
	// ファイル名パスとリソースのマップテーブル
	std::map<std::string, ID3D12Resource*> _resourceTable;

	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end())
	{
		// テーブル内にあったらロードするのではなく
		// マップ内のリソースを返す
		return it->second;
	}

	using LoadLambda_t = std::function<
		HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;
	static std::map<std::string, LoadLambda_t> loadLambdaTable;

	if (loadLambdaTable.empty()) {
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

	//WIC テクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	wstring wtexpath = GetWideStringFromString(texPath);//テクスチャのファイルパス
	string ext = GetExtension(texPath);//拡張子を取得
	if (loadLambdaTable.find(ext) == loadLambdaTable.end()) { return nullptr; }//おかしな拡張子
	auto result = loadLambdaTable[ext](
		wtexpath,
		&metadata,
		scratchImg);
	if (FAILED(result)) { return nullptr; }

	/*HRESULT result = LoadFromWICFile(
		GetWideStringFromString(texPath).c_str(),
		WIC_FLAGS_NONE,
		&metadata,
		scratchImg);

	if (FAILED(result)) { return nullptr; }*/

	//writeToSubresource転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty =
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);
	resDesc.Height = static_cast<UINT>(metadata.height);
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//バッファー作成
	ID3D12Resource* texbuff = nullptr;
	result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	if (FAILED(result)) {
		return nullptr;
	}

	const Image* img = scratchImg.GetImage(0, 0, 0);

	result = texbuff->WriteToSubresource(
		0,
		nullptr,
		img->pixels,
		static_cast<UINT>(img->rowPitch),
		static_cast<UINT>(img->slicePitch)
	);
	if (FAILED(result)) {
		return nullptr;
	}
	_resourceTable[texPath] = texbuff;

	return texbuff;
}
ID3D12Resource* CreateMonoTexture(ID3D12Device* dev, unsigned int val)
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};

	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty =
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4; // 幅
	resDesc.Height = 4; // 高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* whiteBuff = nullptr;
	auto result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff)
	);
	if (FAILED(result)) { return nullptr; }

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), val);

	// データ転送
	result = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		static_cast<UINT>(data.size()));

	return whiteBuff;
}

ID3D12Resource* CreateWhiteTexture(ID3D12Device* dev)
{
	return CreateMonoTexture(dev, 0xff);
}

ID3D12Resource* CreateBlackTexture(ID3D12Device* dev)
{
	return CreateMonoTexture(dev, 0x00);
}

ID3D12Resource* CreateGravGradationTexture(ID3D12Device* dev)
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 256;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* gradBuff = nullptr;
	HRESULT result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&gradBuff)
	);
	if (FAILED(result)) {return nullptr;}

	//上が白くて下が黒いテクスチャデータを作成
	std::vector<unsigned int>data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{//RGBAが逆並びのためRBGマクロと0xff<<24を用いて表す
		unsigned int col = (0xff << 24) | RGB(c, c, c);
		std::fill(it, it + 4, col);
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


int main()
{

#else


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif

	const unsigned int window_width = 1280;
	const unsigned int window_height = 720;

	ID3D12Device* _dev = nullptr;
	IDXGIFactory6*_dxgiFactory = nullptr;
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQueue = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;

	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;
	w.lpszClassName = _T("DX12Sample");
	w.hInstance = GetModuleHandle(nullptr);

	RegisterClassEx(&w);


	RECT wrc = { 0,0,window_width,window_height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("DX12 テスト "),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		w.hInstance,
		nullptr);

	HRESULT D3D12CreateDevuce(
		IUnknown * pAdapter,
		D3D_FEATURE_LEVEL MiniumFeatureLevel,
		REFIID riid,
		void** ppDevice
	);

#ifdef _DEBUG
	//デバッグレイヤーをオン
	EnableDebugLayer();
#endif
		

	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

#ifdef _DEBUG
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#else
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#endif

	std::vector<IDXGIAdapter*>adapters;

	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0;
		_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for(auto adpt :adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;

		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels) 
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK) 
		{
			featureLevel = lv;
			break;
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocator, nullptr,
		IID_PPV_ARGS(&_cmdList));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	cmdQueueDesc.NodeMask = 0;

	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

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
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);

	//SRGBレンダーターゲットビュー設定
	//D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	//rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//ガンマ補正ある（sRGB）
	//rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//レンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//ガンマ補正なし
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	std::vector<ID3D12Resource*>_backBuffers(swcDesc.BufferCount);
	for (UINT idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
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
		D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元のテクスチャデータ
	depthResDesc.Width = window_width;//幅と高さはレンダーターゲット
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;//テクスチャ配列でも、3Dテクスチャでもない
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;//深度値書き込み用フォーマット
	depthResDesc.SampleDesc.Count = 1;//サンプルは1ピクセル当たり1つ
	depthResDesc.Flags =
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//デプスステンシルとして使用

	//深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;//DEFAULTなので後はUNKNOWNでよい
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//このクリアする際の値が重要な意味を持つ
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;//深さ1.0f（最大値）でクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;//32ビットfloat値としてクリア

	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,//深度値書き込み用に使用
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer));

	//深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//深度に使うことが分かればよい
	dsvHeapDesc.NumDescriptors = 1;//深度ビューは1つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//デプスステンシルビューとして使う
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//フラグは特になし

	_dev->CreateDepthStencilView(
		depthBuffer,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);

	ID3D12Resource* whiteTex = CreateWhiteTexture(_dev);
	ID3D12Resource* blackTex = CreateBlackTexture(_dev);
	ID3D12Resource* gradTex = CreateGravGradationTexture(_dev);


	struct PMDHeader 
	{
		float version;//例：00 00 80 3F == 1.00
		char model_name[20];//モデル名
		char comment[256];//モデルコメント
	};
	char signature[3] = {};//シグネチャ
	PMDHeader pmdheader = {};
	
	std::string strModelPath = "Model/初音ミクmetal.pmd";
	//std::string strModelPath = "Model/巡音ルカ.pmd";
	//std::string strModelPath = "Model/初音ミク.pmd";
	FILE* fp;
	fopen_s(&fp, strModelPath.c_str(), "rb");

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	constexpr size_t pmdvertex_size = 38;//頂点1つ当たりのサイズ

	unsigned int vertNum;//頂点数を取得
	fread(&vertNum, sizeof(vertNum), 1, fp);

#pragma pack(1)//ここから１バイトパッキングとなり、アライメントは発生しない

	//PMDマテリアル構造体
	struct PMDMaterial {
		XMFLOAT3 diffuse;       //ディフューズ色
		float alpha;            //ディフューズα
		float specularity;      //スペキュラの強さ（乗算値）
		XMFLOAT3 specular;      //スペキュラ色
		XMFLOAT3 ambient;       //アンビエント色
		unsigned char toonIdx;  //トゥーン番号（後述）
		unsigned char edgeFlg;  //マテリアルごとのの輪郭線フラグ

		//注意：ここに２バイトのパディングがある！！

		unsigned int indicesNum;//このマテリアルが割り当てられる
		                        //インデックス数（後述）
		char texFilePath[20];   //テクスチャファイルパス＋α
	};
#pragma pack()//パッキング指定を解除（デフォルトに戻す）

#pragma pack(push,1)
	struct  PMD_VERTEX
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
	std::vector<PMD_VERTEX>vertices(vertNum);//バッファーの確保
	for (unsigned int i = 0; i < vertNum; i++) {
		fread(&vertices[i], pmdvertex_size, 1, fp);
	}

	ID3D12Resource* vertBuff = nullptr;
	auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size()*sizeof(PMD_VERTEX));
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,//サイズ変更
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	PMD_VERTEX* vertMap = nullptr;//肩をVertexに変更
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	
	

	unsigned int indicesNum;//インデックス数
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	std::vector<unsigned short>indices;
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	

	

	ID3D12Resource* idxBuff = nullptr;

	resdesc.Width = indices.size() * sizeof(indices[0]);
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);


	unsigned int materialNum;//マテリアル数
	fread(&materialNum, sizeof(materialNum), 1, fp);//初音ミク.pmdであればマテリアル数は17

	std::vector<PMDMaterial>pmdMaterials(materialNum);

	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp);//一気に読み込む

	

	//シェ0打0側に投げられるマテリアルデータ
	struct MaterialForHlsl 
	{
		XMFLOAT3 diffuse;//ディフューズ色
		float alpha;//ディフューズα
		XMFLOAT3 specular;//スペキュラ色
		float specularity;//スペキュラの強さ（乗算値）
		XMFLOAT3 ambient;//アンビエント色
	};

	//それ以外のマテリアルデータ
	struct AdditionalMaterial 
	{
		std::string texPath;//テクスチャファイルパス
		int toonIdx;//トゥーン番号
		bool edgeFlg;//マテリアルごとの輪郭線フラグ
	};

	//全体をまとめるデータ
	struct Material 
	{
		unsigned int indicesNum;//インデックス数
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	

	std::vector<Material>materials(pmdMaterials.size());
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

	vector<ID3D12Resource*> textureResources(materialNum);
	vector<ID3D12Resource*>sphResources(materialNum, nullptr);
	vector<ID3D12Resource*>spaResources(materialNum, nullptr);
	vector<ID3D12Resource*>toonResources(materialNum, nullptr);
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		//トゥーンリソースの読み込み
		string toonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, "toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		toonFilePath += toonFileName;
		toonResources[i] = LoadTextureFromFile(toonFilePath, _dev);

		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			textureResources[i] = nullptr;
		}

		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{//スプリッタがある
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph")
			{
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if(GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else {
				texFileName = namepair.first;//他の拡張子でもとにかく最初のものを入れておく
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
			string ext = GetExtension(pmdMaterials[i].texFilePath);
			if (ext == "sph") {
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (ext == "spa") {
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
		}

		// モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
		auto texFilePath = GetTexturePathFromModelAndTexPath(
			strModelPath,
			//pmdMaterials[i].texFilePath
			texFileName.c_str());
		textureResources[i] = LoadTextureFromFile(texFilePath, _dev);

		if (sphFileName != "") {
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
			sphResources[i] = LoadTextureFromFile(sphFilePath, _dev);
		}

		if (spaFileName != "") {
			auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
			spaResources[i] = LoadTextureFromFile(spaFilePath, _dev);
		}
	}
	//マテリアルバッファーの作製
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize * 0xff) & ~0xff;

	ID3D12Resource* materialBuff = nullptr;

	const D3D12_HEAP_PROPERTIES heapPropMat =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC resDescMat = CD3DX12_RESOURCE_DESC::Buffer(
		materialBuffSize * materialNum);//もったいないが仕方ない
	result = _dev->CreateCommittedResource(
		&heapPropMat,
		D3D12_HEAP_FLAG_NONE,
		&resDescMat,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff)
	);

	//マテリアルにコピー
	char* mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;//データコピー
		mapMaterial += materialBuffSize;//次のアライメント位置まで進める（256の倍数）
	}
	materialBuff->Unmap(0, nullptr);

	ID3D12DescriptorHeap* materialDescHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;
	materialDescHeapDesc.NumDescriptors = materialNum * 5;//マテリアル数(定数１つ、テクスチャ4つ)
	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(
		&materialDescHeapDesc, IID_PPV_ARGS(&materialDescHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};

	matCBVDesc.BufferLocation =
		materialBuff->GetGPUVirtualAddress();//バッファーアドレス
	matCBVDesc.SizeInBytes =
		static_cast<UINT>(materialBuffSize);//マテリアルの256アライメントサイズ

	//通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.Shader4ComponentMapping = 
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension =
		D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1


	//先頭を記録
	auto matDescHeapH =
		materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	UINT incSize = _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (UINT i = 0; i < materialNum; ++i)
	{
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);

		matDescHeapH.ptr += incSize;

		matCBVDesc.BufferLocation += materialBuffSize;

		// シェーダーリソースビュー（ここからが追加部分）
		if (textureResources[i] == nullptr)
		{
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex, &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = textureResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				textureResources[i],
				&srvDesc,
				matDescHeapH);
		}
		matDescHeapH.ptr += incSize;


		if (sphResources[i] == nullptr)
		{
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex, &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = sphResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				sphResources[i], &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;


		if (spaResources[i] == nullptr)
		{
			srvDesc.Format = blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				blackTex, &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = spaResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				spaResources[i], &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

		if (toonResources[i] == nullptr)
		{
			srvDesc.Format = gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(gradTex, &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(toonResources[i], &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;
	}

	fclose(fp);


	/*ID3D12Resource* vertBuff = nullptr;

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
	*/
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX));
	vbView.StrideInBytes = sizeof(vertices[0]);


	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* errorBlob = nullptr;
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&_vsBlob, &errorBlob);

	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ){
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
				errstr.resize(errorBlob->GetBufferSize());
				std::copy_n((char*)errorBlob->GetBufferPointer(),
					errorBlob->GetBufferSize(),errstr.begin());
				errstr+="\n";
				OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&_psBlob, &errorBlob);

	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
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
		{
			"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{
			"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{//uv(追加)
			"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,
			0,D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{
			"BONE_NO",0,DXGI_FORMAT_R16G16_UINT,
			0,D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{
			"WEIGHT",0,DXGI_FORMAT_R8_UINT,
			0,D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{
			"EDGE_FLG",0,DXGI_FORMAT_R8_UINT,
			0,D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	gpipeline.pRootSignature = nullptr;//後で設定する

	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	//デフォルトのサンプルマスクを表す定数（0xffffffff）
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//まだアンチエイリアスは使わないためfalse
	gpipeline.RasterizerState.MultisampleEnable = false;

	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

	//深度ステンシル
	gpipeline.DepthStencilState.DepthEnable = true;//深度バッファーを使う
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//書き込む
	gpipeline.DepthStencilState.DepthFunc =
		D3D12_COMPARISON_FUNC_LESS;//小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;




	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	//ひとまず加算や蒸散やαブレンディングは使わない
	renderTargetBlendDesc.BlendEnable = false;
	//ひとまず論理演算は使用しない
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列の要素数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	//gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;//点で構成
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline.NumRenderTargets = 1;//今は一つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA
	//gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき1
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低

	ID3D12RootSignature* rootsignature = nullptr;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange[3] = {};

	//テクスチャ用レジスター０番
	descTblRange[0].NumDescriptors = 1;//テクスチャ1つ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//種別はテクスチャ
	descTblRange[0].BaseShaderRegister = 0;//0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	
	//定数用レジスター０番
	descTblRange[1].NumDescriptors = 1;//テクスチャ1つ
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//種別は定数
	descTblRange[1].BaseShaderRegister = 1;//1番スロットから
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//テクスチャ一つ目
	descTblRange[2].NumDescriptors = 4;//テクスチャ4つ（基本とsphとspaとtoon）
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別は定数
	descTblRange[2].BaseShaderRegister = 0;//0番スロットから
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	
	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;//ディスクリプタレンジ数
	rootparam[0].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_ALL;//ピクセルシェーダーから見える

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];
	rootparam[1].DescriptorTable.NumDescriptorRanges = 2;//ディスクリプタレンジ数
	rootparam[1].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;//すべてのシェーダーから見える

	rootSignatureDesc.pParameters = rootparam;//ルートパラメーターの先頭アドレス
	rootSignatureDesc.NumParameters = 2;//ルートパラメーター数

	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横方向の繰り返し
	samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦方向の繰り返し
	samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行きの繰り返し
	samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーは黒
	samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//線形補間
	//samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//線形補間しない
	//samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
	samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
	samplerDesc[0].MinLOD = 0.0f;//ミップマップ最小値
	samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダーから見える
	samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//リサンプリングしない
	samplerDesc[0].ShaderRegister = 0;//シェーダースロット番号を忘れないように

	samplerDesc[1] = samplerDesc[0];
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//横方向
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//縦方向
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//奥行方向
	samplerDesc[1].Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;//補間しない
	samplerDesc[1].ShaderRegister = 1;//シェーダースロット番号を忘れないように

	rootSignatureDesc.pStaticSamplers = &samplerDesc[0];
	rootSignatureDesc.NumStaticSamplers = 2;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,//ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0,//ルートシグネチャバージョン
		&rootSigBlob,//シェーダーを作ったときと同じ
		&errorBlob);//エラーの同じ処理


	result = _dev->CreateRootSignature(
		0, // nodemask。0 でよい
		rootSigBlob->GetBufferPointer(), // シェーダーのときと同様
		rootSigBlob->GetBufferSize(), // シェーダーのときと同様
		IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release(); // 不要になったので解放

	gpipeline.pRootSignature = rootsignature;

	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;//出力先の幅（ピクセル数）
	viewport.Height = window_height;//出力先の高さ（ピクセル数）
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値

	D3D12_RECT scissorrect = {};//ビューポートのすべてを表示する設定
	scissorrect.top = 0;//切り抜き上座標
	scissorrect.left = 0;//切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;//切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;//切り抜き下座標

	//WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	result = LoadFromWICFile(
		L"img/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg);

	auto img = scratchImg.GetImage(0, 0, 0);//生データ抽出
	
	
	/*struct TexRGBA {
		unsigned char R, G, B, A;
	};

	std::vector<TexRGBA>texturedata(256 * 256);
	for (auto& rgba : texturedata) 
	{
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = 255;//αは1.0とする
	}*/

	//WriteToSubresourceで転送するためのヒープ設定
	//D3D12_HEAP_PROPERTIES texHeapProp = {};
	//特殊な設定なのでDEFAULTでもUPLOADでもない
	//texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	//ライトバック
	//texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	//転送はL0、つまりCPU側から直接行う
	//texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	//単一アダプターのため0
	//texHeapProp.CreationNodeMask = 0;
	//texHeapProp.VisibleNodeMask = 0;

	//中間バッファーとしてのアップロードヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProp = {};

	//マップ可能にするため、UPLOADにする
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	//アップロード用に使用すること前提なのでUNKNOWNでよい
	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	uploadHeapProp.CreationNodeMask = 0;//単一アダプターのため0
	uploadHeapProp.VisibleNodeMask = 0;//単一アダプターのため0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_UNKNOWN;//単なるデータの塊なのでUNKNOWN
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;//単なるバッファーとして指定

	resDesc.Width = 
		AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
		* img->height;//データサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;

	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;//レイアウトは決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//特にフラグなし

	resDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0;//クオリティは最低
	
	//中間バッファー作成
	ID3D12Resource* uploadbuff = nullptr;

	result = _dev->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadbuff)
	);

	//テクスチャのためのヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};

	texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;//テクスチャ用
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texHeapProp.CreationNodeMask = 0;//単一アダプターのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプターのため0

	//リソース設定（変数は使いまわし）
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);//幅
	resDesc.Height = static_cast<UINT>(metadata.height);//高さ
	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
	resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトは決定しない

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特になし
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,//コピー先
		nullptr,
		IID_PPV_ARGS(&texbuff));

	uint8_t* mapforImg = nullptr;//image->pixelsと同じ型にする
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);//マップ

	auto srcAddress = img->pixels;

	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	for (int y = 0; y < img->height; ++y) {
		std::copy_n(srcAddress,
			rowPitch,
			mapforImg);//コピー

		//1行ごとの辻褄を合わせる
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}
	
	//std::copy_n(img->pixels, img->slicePitch, mapforImg);//コピー
	uploadbuff->Unmap(0, nullptr);//アンマップ

	/*
	result = texbuff->WriteToSubresource
	(
		0,
		nullptr,//全領域へコピー
		//texturedata.data(),//元データアドレス
		//sizeof(TexRGBA) * 256,//1ラインサイズ
		//sizeof(TexRGBA) * (UINT)texturedata.size()//全サイズ
		img->pixels,//元データアドレス
		static_cast<UINT>(img->rowPitch),//1ラインサイズ
		static_cast<UINT>(img->slicePitch)//全サイズ
	);
	*/
		D3D12_TEXTURE_COPY_LOCATION src = {};

	// コピー元（アップロード側）設定
	src.pResource = uploadbuff; // 中間バッファー
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; // フットプリント指定
	src.PlacedFootprint.Offset = 0;
	src.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
	src.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
	src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
	src.PlacedFootprint.Footprint.RowPitch =
		static_cast<UINT>(AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	src.PlacedFootprint.Footprint.Format = img->format;

	D3D12_TEXTURE_COPY_LOCATION dst = {};

	// コピー先設定
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
		D3D12_RESOURCE_STATE_COPY_DEST;// ここが重要
	BarrierDesc.Transition.StateAfter = 
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;// ここも重要

	_cmdList->ResourceBarrier(1, &BarrierDesc);
	_cmdList->Close();

	// コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	_cmdQueue->Signal(_fence, ++_fenceVal);

	if (_fence->GetCompletedValue() != _fenceVal)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	_cmdAllocator->Reset();//キューをクリア
	_cmdList->Reset(_cmdAllocator, nullptr);

	//シェーダー側に渡すための基本的な行列データ
	struct SceneData 
	{
		XMMATRIX world;//モデル本体を回転させたり移動させたりする行列
		XMMATRIX view;//ビュー行列
		XMMATRIX proj;//プロジェクション行列
		XMFLOAT3 eye;//視点座標
	};
	

	//定数バッファ作成
	XMMATRIX worldMat = XMMatrixRotationY(XM_PIDIV4);

	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	auto viewMat = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,//画角は90°
		static_cast<float>(window_width)
		/ static_cast<float>(window_height),//アスペクト比
		1.0f,//近いほう
		100.0f//遠いほう
	);

	
	worldMat.r[0].m128_f32[0] = +2.0f / window_width;
	worldMat.r[1].m128_f32[1] = -2.0f / window_height;

	//XMMATRIXは行優先
	worldMat.r[3].m128_f32[0] = -1.0f;
	worldMat.r[3].m128_f32[1] = +1.0f;
	

	ID3D12Resource* constBuff = nullptr;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);

	SceneData* mapScene;//マップ先を示すポインター
	result = constBuff->Map(0, nullptr, (void**)&mapScene);//マップ
	mapScene->world = worldMat;
	mapScene->view = viewMat;
	mapScene->proj = projMat;
	mapScene->eye = eye;
	//*mapMatrix = matrix;//行列内容をコピー


	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	//シェーダーから見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//マスクは0
	descHeapDesc.NodeMask = 0;
	//SRV1つとCBV1つ
	descHeapDesc.NumDescriptors = 1;
	//シェーダーリソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));

	/*D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA(0.0f～1.0ffに正規化)
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;//みっぷマップは使用しないので1*/



	//デスクリプタの先頭ハンドルを取得しておく
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	/*_dev->CreateShaderResourceView
	(
		texbuff,//ビューと関連付けるバッファー
		&srvDesc,//先ほど設定したテクスチャ設定情報
		basicDescHeap->GetCPUDescriptorHandleForHeapStart()//ヒープのどこに割り当てるか
	);


	//次の場所に移動
	basicHeapHandle.ptr +=
		_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);*/

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);

	//定数バッファービューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

	unsigned int frame = 0;

	float angle = 0.0f;

	
	while (true)
	{
		MSG msg;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			
		}
		angle += 0.1f;
		worldMat = XMMatrixRotationY(angle);
		mapScene->world = worldMat;
		mapScene->view = viewMat;
		mapScene->proj = projMat;

		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		auto BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET); // これだけで済む
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		_cmdList->SetPipelineState(_pipelinestate);

		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) 
		{
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, nullptr);

		//レンダーターゲットの指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);

		//画面クリア
		/*float r, g, b;
		r = (float)(0xff & frame >> 16) / 255.0f;
		g = (float)(0xff & frame >> 3) / 5.0f;
		b = (float)(0xff & frame >> 0) / 255.0f;*/
		float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };//白色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		++frame;

		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(0,
			basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(
			1,
			materialDescHeap->GetGPUDescriptorHandleForHeapStart());

		_cmdList->SetGraphicsRootSignature(rootsignature);

		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);

		//_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);//点で構成
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);//三角形で構成

		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);

		////ルートシグネチャの指定
		//_cmdList->SetGraphicsRootSignature(rootsignature);
		////ディスクリプタヒープの指定
		//_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		////ルートパラメーターとディスクリプタヒープの関連付け
		//auto heapHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		//_cmdList->SetGraphicsRootDescriptorTable(
		//	0,//ルートパラメーターインデックス
		//	heapHandle);//ヒープアドレス
		//heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(
		//	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//_cmdList->SetGraphicsRootDescriptorTable(1, heapHandle);
		//	


		////_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
		//_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		auto materialH = materialDescHeap->GetGPUDescriptorHandleForHeapStart();

		unsigned int idx0ffset = 0;

		UINT cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

		for (Material& m : materials) {
			_cmdList->SetGraphicsRootDescriptorTable(1, materialH);

			_cmdList->DrawIndexedInstanced(
				m.indicesNum, 1, idx0ffset, 0, 0);

			materialH.ptr +=cbvsrvIncSize;

			idx0ffset += m.indicesNum;
		}

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

		////待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) 
		{
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}

		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, nullptr);//再びコマンドリストをためる準備

		//フリップ
		_swapchain->Present(1, 0);
	}


		UnregisterClass(w.lpszClassName, w.hInstance);

		return 0;
	
}
