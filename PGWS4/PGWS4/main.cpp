#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<d3dx12.h>
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

//モデルのパスとテクスチャのパスから合成パスを得る
//@param modelPath アプリケーションから見たpmdモデルのパス
//@param texPath PMD モデルから見たテクスチャのパス
//@return アプリケーションから見たテクスチャのパス
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath)
{
	//ファイルのフォルダー区切りは\と/の2種類が使用される可能性があり
	//ともかく末尾の\か/を得られればいいので、双方のrfindを比較する
	// (int型に代入しているのは、見つからなかった場合は
	//rfindがepos(-1→0xffffffff)を返すため
	int pathIndex1 = static_cast<int>(modelPath.rfind('/'));
	int pathIndex2 = static_cast<int>(modelPath.rfind('\\'));
	int pathIndex = max(pathIndex1, pathIndex2);
	string folderPath = modelPath.substr(0, pathIndex + 1);
	return folderPath + texPath;
}

//std::string（マルチバイト文字列）からstd::wstring（ワイド文字列）を得る
//@param str マルチバイト文字列
//@return　変換されたワイド文字列
std::wstring GetWideStringFromString(const std::string& str)
{
	//呼び出し1回目（文字列数を得る）
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr;  //string のwchar_t版
	wstr.resize(num1);  //得られた文字列数でリサイズ

	//呼び出し2回目（確保済みのwstrに変換文字列をコピー）
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1);

	assert(num1 == num2);   //一応チェック
	return wstr;
}

ID3D12Resource* LoadtextureFromFile(std::string& texpath, ID3D12Device* dev)
{
	//WIC テクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	HRESULT result = LoadFromWICFile(
		GetWideStringFromString(texpath).c_str(),
		WIC_FLAGS_NONE,
		&metadata,
		scratchImg);

	if (FAILED(result)) { return nullptr; }

	//WriteToSubresourceで転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty =
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;  //単一アダプターの為0
	texHeapProp.VisibleNodeMask = 0;  //単一アダプターの為0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);  //幅
	resDesc.Height = static_cast<UINT>(metadata.height);  //高さ
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
	resDesc.SampleDesc.Count = 1;  //通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0;  //クオリティは最低
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;  //レイアウトは決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;  //特にフラグなし

	//バッファー作成
	ID3D12Resource* texbuff = nullptr;
	result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,  //特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);
	if (FAILED(result)) {return nullptr;}

	const Image* img = scratchImg.GetImage(0, 0, 0);  //生データ抽出

	result = texbuff->WriteToSubresource(
		0,
		nullptr,  //全領域へコピー
		img->pixels,  //元データアドレス
		static_cast<UINT>(img->rowPitch),  //1ラインサイズ
		static_cast<UINT>(img->slicePitch) //全サイズ
	);
	if (FAILED(result)) {return nullptr;}

	return texbuff;
}

ID3D12Resource* CreateWhiteTexture(ID3D12Device* dev)
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};

	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty =
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	resDesc.Width = 4;  //幅
	resDesc.Height = 4;  //高さ
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
		D3D12_HEAP_FLAG_NONE,  //特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff)
	);
	if (FAILED(result)) { return nullptr; }

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	//データ転送
	result = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		static_cast<UINT>(data.size()));

	return whiteBuff;
}


//アライメントのそろえたサイズを返す
//@param size　元のサイズ
//@param alignment アライメントサイズ
//@return　アライメントをそろえたサイズ
size_t AligmentSize(size_t size, size_t aligment)
{
	return size + aligment - size % aligment;
}

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
		std::wstring strDesc = adesc.Description;
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
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //レンダーターゲットビューなので　RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;                      //裏表の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //特に指定なし

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);

	//SRGBレンダラーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;  //ガンマ補正あり(sRGB)
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	std::vector<ID3D12Resource*>_backBuffers(swcDesc.BufferCount);
	for (unsigned int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		D3D12_CPU_DESCRIPTOR_HANDLE handle
			= rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_dev->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
	}

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	////頂点データ構造体
	//struct Vertex {
	//	XMFLOAT3 pos; //xyz座標
	//	XMFLOAT2 uv;  //uv座標
	//};

	//PMDヘッダー構造体
	struct PMDHeader
	{
		float version;  //例:00 00 80 3F==1.00
		char model_name[20];  //モデル名
		char comment[256];  //モデルコメント
	};

	char signature[3] = {};  //シグネチャ
	PMDHeader pmdheader = {};
	std::string strModelPath = "Model/初音ミク.pmd";
	FILE* fp;
	fopen_s(&fp, strModelPath.c_str(), "rb");

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	constexpr size_t pmdvertex_size = 38;  //頂点1つあたりのサイズ

	unsigned int vertNum;  //頂点数を取得
	fread(&vertNum, sizeof(vertNum), 1, fp);

#pragma pack(1)  //ここから１バイトパッキングとなり、アライメントは発生しない

	//PMD マテリアル構造体
	struct  PMDMaterial
	{
		XMFLOAT3 diffuse;		// ディフューズ色
		float alpha;			// ディフューズα
		float specularity;		// スペキュラの強さ（乗算値）
		XMFLOAT3 specular;		// スペキュラ色
		XMFLOAT3 ambient;		// アンビエント色
		unsigned char toonIdx;	// トゥーン番号
		unsigned char edgeFlg;	// マテリアルごとの輪郭線フラグ

		// 注意：ここに2 バイトのパディングがある！！

		unsigned int indicesNum;// このマテリアルが割り当てられる
								// インデックス数（
		char texFilePath[20];	// テクスチャファイルパス＋α
	};

#pragma pack()  //パッキング指定を解除

#pragma pack(push,1)
	struct PMD_VERTEX
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
		uint16_t bone_no[2];
		uint8_t  weight;
		uint8_t  EdgeFlag;
		uint16_t dummy;
	};
#pragma pack(pop)
	std::vector<PMD_VERTEX>vertices(vertNum);  //バッファーの確保
	for (unsigned int i = 0; i < vertNum; i++)
	{
		fread(&vertices[i], pmdvertex_size, 1, fp);
	}

	ID3D12Resource* vertBuff = nullptr;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resoucepDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMD_VERTEX));

	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoucepDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	PMD_VERTEX* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();  //バッファーの仮想アドレス
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX));  //全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]); //1頂点あたりのバイト数

	//頂点インデックス
	unsigned int indicesNum; //インデックス数

	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	std::vector<unsigned short>indices;
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	ID3D12Resource* idxbuff = nullptr;

	//設定は、バッファーのサイズ以外、頂点バッファーの設定を使いまわしても良い
	resoucepDesc.Width = indices.size() * sizeof(indices[0]);
	result = _dev->CreateCommittedResource(
		&heapProp,  //UPLOAD ヒープとして
		D3D12_HEAP_FLAG_NONE,
		&resoucepDesc,  //サイズに応じて適切な設定をしてくれる
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxbuff));

	//作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxbuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxbuff->Unmap(0, nullptr);

	unsigned int materialNum;  //マテリアル数
	fread(&materialNum, sizeof(materialNum), 1, fp);


	//インデックスバッファービューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxbuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	std::vector<PMDMaterial>pmdMaterials(materialNum);

	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp);

	//シェーダー側に投げられるマテリアルデータ
	struct MaterialForHlsl
	{
		XMFLOAT3 diffuse;  //ディフューズ色
		float alpha;  //ディフューズα
		XMFLOAT3 specular;  //スペキュラ色
		float specularity;  //スペキュラの強さ（乗算値）
		XMFLOAT3 ambient;  //アンビエント色
	};

	//それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		std::string texPath;  //テクスチャファイルパス
		int toonIdx;  //トゥーン番号
		bool edgeFlag;  //マテリアルごとの輪郭線フラグ
	};

	//全体をまとめるデータ
	struct Material
	{
		unsigned int indicesNum;  //インデックス数
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	std::vector<Material>materials(pmdMaterials.size());
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
	const D3D12_RESOURCE_DESC resDesdMat = CD3DX12_RESOURCE_DESC::Buffer(
		materialBuffSize * materialNum);
	result = -_dev->CreateCommittedResource(
		&heapPropMat,
		D3D12_HEAP_FLAG_NONE,
		&resDesdMat,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff));

	//マップマテリアルにコピー
	char* mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;  //データコピー
		mapMaterial += materialBuffSize;  //次のアライメント位置まで進める（256の倍数）
	}
	materialBuff->Unmap(0, nullptr);

	ID3D12Resource* whiteTex = CreateWhiteTexture(_dev);
	vector<ID3D12Resource*>textureResource(materialNum);
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			textureResource[i] = nullptr;
		}
		//モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
		auto texFilePath = GetTexturePathFromModelAndTexPath(
			strModelPath,
			pmdMaterials[i].texFilePath);
		textureResource[i] = LoadtextureFromFile(texFilePath, _dev);
	}

	ID3D12DescriptorHeap* materialDescHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;
	materialDescHeapDesc.NumDescriptors = materialNum * 2;  //マテリアル数×2
	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(
		&materialDescHeapDesc, IID_PPV_ARGS(&materialDescHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};

	matCBVDesc.BufferLocation =
		materialBuff->GetGPUVirtualAddress();  //バッファアドレス
	matCBVDesc.SizeInBytes =
		static_cast<UINT>(materialBuffSize);  //マテリアルの256アライメントサイズ

	//通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  //デフォルト
	srvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension =
		D3D12_SRV_DIMENSION_TEXTURE2D;  //2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;  //ミップは使用しないので1

	//先頭を記録
	auto matDescHeapH =
		materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	UINT inc = _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (UINT i = 0; i < materialNum; ++i)
	{
		//マテリアル用定数バッファービュー
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		
		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += materialBuffSize;

		//シェーダーリソースビュー
		if (textureResource[i] == nullptr)
		{
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex, &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = textureResource[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				textureResource[i],
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;
	}

	fclose(fp);

	//深度バッファーの作成
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension =
		D3D12_RESOURCE_DIMENSION_TEXTURE2D;  //2次元のテクスチャーデータ
	depthResDesc.Width = window_width;  //幅と高さはレンダーターゲットと同じ
	depthResDesc.Height = window_height;  //同上
	depthResDesc.DepthOrArraySize = 1;  //テクスチャ配列でも、3Dテクスチャでもない
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;  //深度値書き込み用フォーマット
	depthResDesc.SampleDesc.Count = 1;  //サンプルはピクセルあたり1つ
	depthResDesc.Flags =
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;  //デプスステンシルとして使用

	//深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;  //DEFAILTなのであとはUNKNOWNでよい
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;  //32ビットfloat値としてクリア

	//このクリアする際の値が重要な意味を持つ
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f; //不可さ1.0f（最大値）でクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;  //32ビットloat値としてクリア

	ID3D12Resource* depthbuffe = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,  //深度値書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(&depthbuffe));

	//深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};  //深度に使うことがわかればよい
	dsvHeapDesc.NumDescriptors = 1;  //深度ビューは1つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;  //デプスステンシルビューとして使う
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;  //深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;  //2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;  //フラグは特になし

	_dev->CreateDepthStencilView(
		depthbuffe,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());


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
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
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

	//深度ステンシル
	gpipline.DepthStencilState.DepthEnable = true;  //深度バッファーを使う
	gpipline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;  //書き込む
	gpipline.DepthStencilState.DepthFunc =
		D3D12_COMPARISON_FUNC_LESS;  //小さいほうを採用
	gpipline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

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
	gpipline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;  //0～1に正規化されtPGBA

	gpipline.SampleDesc.Count = 1;  //サンプリングは1ピクセルにつき1
	gpipline.SampleDesc.Quality = 0;  //クオリティは最低

	D3D12_ROOT_SIGNATURE_DESC rootSignaterDesc = {};
	rootSignaterDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange[3] = {};  //テクスチャと定数の2つ

	//テクスチャ用レジスター0番
	descTblRange[0].NumDescriptors = 1;  //テクスチャ1つ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;  //種別はテクスチャ
	descTblRange[0].BaseShaderRegister = 0;  //0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//定数用レジスター1番
	descTblRange[1].NumDescriptors = 1;  //	定数1つ
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;  //種別は定数
	descTblRange[1].BaseShaderRegister = 1;  //1番スロットから
	descTblRange[1].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//テクスチャ1つ目（マテリアルとペア）
	descTblRange[2].NumDescriptors = 1;  //テクスチャ1つ
	descTblRange[2].RangeType =
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  //種別はテクスチャ
	descTblRange[2].BaseShaderRegister = 0;  //0版スロットから
	descTblRange[2].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  //横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  //縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  //奥行きの繰り返し
	samplerDesc.BorderColor =
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;  //ボーダーは黒
	//補間しない（ニアレストネイバー法：最近傍補間)
	//samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;  //線形補間
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;  //ミップマップ最大値
	samplerDesc.MinLOD = 0.0f; //ミップマップ最小値
	samplerDesc.ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;  //ピクセルシェーダーから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;  //リサンプリングしない

	rootSignaterDesc.pStaticSamplers = &samplerDesc;
	rootSignaterDesc.NumStaticSamplers = 1;

	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//配列先頭アドレス
	rootparam[0].DescriptorTable.pDescriptorRanges = descTblRange;
	//ディスクリプタレンジ数
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;
	//ピクセルシェーダーが見える
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges =
		&descTblRange[1];  //ディスクリプタレンジのアドレス
	rootparam[1].DescriptorTable.NumDescriptorRanges = 2;  //ディスクリプタレンジ数
	rootparam[1].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;  //ピクセルシェーダーから見える

	rootSignaterDesc.pParameters = rootparam;  //ルートパラメータの戦闘アドレス
	rootSignaterDesc.NumParameters = 2;  //ルートパラメータ数

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

	//WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	result = LoadFromWICFile(
		L"img/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg);

	auto img = scratchImg.GetImage(0, 0, 0);

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


	//WriteToSubresourceで送信するためのヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProp = {};
	//マップ可能にするために、UPLOAADにする
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	//アップロード用に辞要すること前提なのでUNKNOWNでよい
	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//単一アダプターのため0
	uploadHeapProp.CreationNodeMask = 0;
	uploadHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_UNKNOWN;  //単なるデータの塊なのでUNKOWN
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; //単なるバッファーとして指定

	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;

	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;  //連続したデータ
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;  //特にフラグなし

	resDesc.SampleDesc.Count = 1;  //通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0;  //クオリティは最低


	//中間バッファ作成
	ID3D12Resource* uploadbuff = nullptr;

	resDesc.Width
		= AligmentSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
		* img->height;

	result = _dev->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadbuff)
	);


	//ランダムなノイズ画像を描画
	struct TexRGBA
	{
		unsigned char R, G, B, A;
	};


	D3D12_HEAP_PROPERTIES texheapProp = {};

	texheapProp.Type = D3D12_HEAP_TYPE_DEFAULT;  //テクスチャ用
	texheapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texheapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texheapProp.CreationNodeMask = 0;  //単一アダプターの為0
	texheapProp.VisibleNodeMask = 0;  //単一アダプターの為0

	//リソース設定（変数は使いまわし）
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);  //幅
	resDesc.Height = static_cast<UINT>(metadata.height);  //高さ
	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;  //レイアウトは決定しない

	ID3D12Resource* texbuff = nullptr;

	//std::vector<TexRGBA>texturedata(256 * 256);
	//for (auto& rgba : texturedata)
	//{
	//	rgba.R = rand() % 256;
	//	rgba.G = rand() % 256;
	//	rgba.B = rand() % 256;
	//	rgba.A = 255;  //αは1.0とする
	//}


//	result = texbuff->WriteToSubresource(
//		0,
//		nullptr,  //全領域へコピー
//		img->pixels,  //元データアドレス
//		static_cast<UINT>(img->rowPitch),  //1ラインサイズ
//		static_cast<UINT>(img->slicePitch)  //全サイズ
//	);

	/*ID3D12Resource* texbuff = nullptr;*/
	result = _dev->CreateCommittedResource(
		&texheapProp,
		D3D12_HEAP_FLAG_NONE,  //特になし
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,  //コピー先
		nullptr,
		IID_PPV_ARGS(&texbuff));

	uint8_t* mapforImg = nullptr;  //image->pixelsと同じ型にする
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);  //マップ

	auto srcAddress = img->pixels;

	auto rowPitch = AligmentSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	for (int y = 0; y < img->height; ++y)
	{
		std::copy_n(srcAddress,
			rowPitch,
			mapforImg); // コピー

		// 1 行ごとのつじつまを合わせる
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}

	//std::copy_n(img->pixels, img->slicePitch, mapforImg);  //コピー
	uploadbuff->Unmap(0, nullptr);  //アンマップ


	struct MatricesData
	{
		XMMATRIX world;  //モデル本体を回転させたり移動させたりする行列
		XMMATRIX viewproj;  //ビューとプロジェクション合成行列
	};

	D3D12_TEXTURE_COPY_LOCATION src = {};

	//コピー元（アップロード側）設定
	src.pResource = uploadbuff;  //中間バッファ
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; // フットプリント指定
	src.PlacedFootprint.Offset = 0;
	src.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
	src.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
	src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
	src.PlacedFootprint.Footprint.RowPitch =
		static_cast<UINT>(AligmentSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	src.PlacedFootprint.Footprint.Format = img->format;

	D3D12_TEXTURE_COPY_LOCATION dst = {};

	//コピー先設
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
	_cmdQeue->ExecuteCommandLists(1, cmdlists);

	_cmdQeue->Signal(_fence, ++_fenceVal);

	if (_fence->GetCompletedValue() != _fenceVal)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	_cmdAllocator->Reset();//キューをクリア
	_cmdList->Reset(_cmdAllocator, nullptr);

	//定数バッファ作成
	XMMATRIX worldMat = XMMatrixRotationY(XM_PIDIV4);

	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	auto viewMat = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,   //画角は90°
		static_cast<float>(window_width)
		/ static_cast<float>(window_height),  //アスペクト比
		1.0f,  //近いほう
		100.0f  //遠いほう
	);

	//matrix.r[0].m128_f32[0] = +1.0 / window_width;
	//matrix.r[1].m128_f32[1] = -1.0f / window_height;

	//matrix.r[3].m128_f32[0] = -1.0f;
	//matrix.r[3].m128_f32[1] = +1.0f;

	ID3D12Resource* constBuff = nullptr;
	//auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);
	_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);

	MatricesData* mapMatrix;  //マップ先を示すポインター
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);  //マップ
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;
	//*mapMatrix = matrix;  //行列の内容をコピー

	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHearDesc = {};
	//シェーダーから見えるように
	descHearDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//マスクは0
	descHearDesc.NumDescriptors = 0;
	//SRV1つとCBV1つ
	descHearDesc.NumDescriptors = 1;
	//シェーダーリソースビュー用
	descHearDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//生成
	result = _dev->CreateDescriptorHeap(&descHearDesc, IID_PPV_ARGS(&basicDescHeap));

	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Format = metadata.format;  //RGBA(0.0f~1.0fに正規化)
	//srvDesc.Shader4ComponentMapping =
	//	D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  //2Dテクスチャ
	//srvDesc.Texture2D.MipLevels = 1;  //ミップマップは使用しない


	//ディスクリプタの先頭ハンドルを取得しておく
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	//_dev->CreateShaderResourceView(
	//	texbuff,  //ビューと関連付けるバッファー
	//	&srvDesc,  //先ほど設定したテクスチャ情報
	//	basicHeapHandle  //ヒープのどこに割り当てるかv
	//);

	////次の場所に移動
	//basicHeapHandle.ptr +=
	//	_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdvDesc = {};
	cdvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cdvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);

	//定数バッファービューの作成
	_dev->CreateConstantBufferView(&cdvDesc, basicHeapHandle);


	//チャレンジ問題用
	//float alpha = 0;
	float angle = 0.0f;

	//*mapMatrix = worldMat * viewMat * projMat; 
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

		angle = 0.1f;
		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;
		mapMatrix->viewproj = viewMat * projMat;
		//*mapMatrix = worldMat * viewMat * projMat;

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
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);

		//チャレンジ問題用
		//alpha = alpha + 0.01f;
		//if (alpha > 1.0f)
		//{
		//	alpha = 0.0f;
		//}

		//画面クリア
		float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };  //白色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		_cmdList->SetGraphicsRootSignature(rootSigunature);
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);

		_cmdList->SetGraphicsRootDescriptorTable(0,
			basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(
			1,
			materialDescHeap->GetGPUDescriptorHandleForHeapStart());
		//auto heapHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		//_cmdList->SetGraphicsRootDescriptorTable(
		//	0,  //ルートパラメータインデックス
		//	heapHandle);  //ヒープアドレス
		//heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(
		//	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//_cmdList->SetGraphicsRootDescriptorTable(1, heapHandle);

		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);

		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		auto materialH = materialDescHeap->
			GetGPUDescriptorHandleForHeapStart(); // ヒープ先頭

		unsigned int idx0ffset = 0;  //最初はオフセットなし

		UINT cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 2;  //2倍

		for (auto& m : materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, materialH);

			_cmdList->DrawIndexedInstanced(
				m.indicesNum, 1, idx0ffset, 0, 0);

			//ヒープポインターとインデックスを次に進める
			materialH.ptr += cbvsrvIncSize;
			idx0ffset += m.indicesNum;
		}

		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

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

		_cmdAllocator->Reset(); // キューをクリア
		_cmdList->Reset(_cmdAllocator, _pipelinesture); //キューをクリア
		_swapchain->Present(1, 0); //再びコマンドリストを溜める準備
	}
	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
	DebugOutPutFormatStaring("Show window test.");
	getchar();
	return 0;
}
