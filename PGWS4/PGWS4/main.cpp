#include "Application.h"

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	Application& app = Application::Instance();

	if (!app.Init()) {
		return -1;
	}
	app.Run();
	
	app.Terminate();

	return 0;
}

////ウィンドウ表示＆DirectX初期化
//#include <Windows.h>
//#include <tchar.h>
//#include <d3d12.h>
//#include <d3dx12.h>
//#include <dxgi1_6.h>
//#include <DirectXMath.h>
//#include <map>
//#include <vector>
//#include <string>
//#include <d3dcompiler.h>
//#include <DirectXTex.h>
//#ifdef _DEBUG
//#include <iostream>
//#endif
//#include <wrl.h>
//
//#pragma comment(lib,"d3d12.lib")
//#pragma comment(lib,"dxgi.lib")
//#pragma comment(lib,"d3dcompiler.lib")
//#pragma comment(lib,"DirectXTex.lib")
//
//using namespace std;
//using namespace DirectX;
//using namespace Microsoft::WRL;
//
/////@brief コンソール画面にフォーマット付き文字列を表示
/////@param format フォーマット(%dとか%fとかの)
/////@param 可変長引数
/////@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
//void DebugOutputFormatString(const char* format, ...) {
//#ifdef _DEBUG
//	va_list valist;
//	va_start(valist, format);
//	vprintf(format, valist);
//	va_end(valist);
//#endif
//}
//
////面倒ですが、ウィンドウプロシージャは必須なので書いておきます
//LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
//	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら呼ばれます
//		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
//		return 0;
//	}
//	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
//}
//#ifdef _DEBUG
//void EnableDebugLayer()
//{
//	ComPtr<ID3D12Debug> debugLayer = nullptr;
//	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
//	if (!SUCCEEDED(result))return;
//
//	debugLayer->EnableDebugLayer(); // デバッグレイヤーを有効かする
//	debugLayer->Release(); // 有効かしたらインターフェイスを解放する
//}
//#endif // _DEBUG
//
//// モデルのパスとテクスチャのパスから合成パスを得る
//// @param modelPath アプリケーションから見たpmd モデルのパス
//// @param texPath PMD モデルから見たテクスチャのパス
//// @return アプリケーションから見たテクスチャのパス
//std::string GetTexturePathFromModelAndTexPath(
//	const std::string& modelPath,
//	const char* texPath)
//{
//	// ファイルのフォルダ一区切りは\ と/ の2 種類が使用される可能性があり
//	// ともかく末尾の\ か/ を得られればいいので、双方のrfind を取り比較する
//	// （int 型に代入しているのは、見つからなかった場合は
//	// rfind がepos(-1 → 0xffffffff) を返すため）
//	int pathIndex1 = static_cast<int>(modelPath.rfind('/'));
//	int pathIndex2 = static_cast<int>(modelPath.rfind('\\'));
//	int pathIndex = max(pathIndex1, pathIndex2);
//	string folderPath = modelPath.substr(0, pathIndex + 1);
//	return folderPath + texPath;
//}
//
//// std::string（マルチバイト文字列）からstd::wstring（ワイド文字列）を得る
//// @param str マルチバイト文字列
//// @return 変換されたワイド文字列
//std::wstring GetWideStringFromString(const std::string& str)
//{
//	// 呼び出し1 回目（文字列数を得る）
//	auto num1 = MultiByteToWideChar(
//		CP_ACP,
//		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
//		str.c_str(),
//		-1,
//		nullptr,
//		0);
//
//	std::wstring wstr; // string のwchar_t 版
//	wstr.resize(num1); // 得られた文字列数でリサイズ
//
//	// 呼び出し2 回目（確保済みのwstr に変換文字列をコピー）
//	auto num2 = MultiByteToWideChar(
//		CP_ACP,
//		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
//		str.c_str(),
//		-1,
//		&wstr[0],
//		num1);
//
//	assert(num1 == num2); // 一応チェック
//	return wstr;
//}
//
//// ファイル名から拡張子を取得する
//// @param path 対象のパス文字列
//// @return 拡張子
//std::string GetExtension(const std::string& path)
//{
//	int idx = static_cast<int>(path.rfind('.'));
//	return path.substr(
//		idx + 1, path.length() - idx - 1);
//}
//
//// テクスチャのパスをセパレーター文字で分離する
//// @param path 対象のパス文字列
//// @param splitter 区切り文字
//// @return 分離前後の文字列ペア
//std::pair<std::string, std::string> SplitFileName(
//	const std::string& path, const char splitter = '*')
//{
//	int idx = static_cast<int>(path.find(splitter));
//	pair<std::string, std::string>ret;
//	ret.first = path.substr(0, idx);
//	ret.second = path.substr(
//		idx + 1, path.length() - idx - 1);
//	return ret;
//}
//
//ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath, ID3D12Device* dev)
//{
//	// ファイル名パスとリソースのマップテーブル
//	std::map<string, ComPtr<ID3D12Resource>>_resourceTable;
//
//	auto it = _resourceTable.find(texPath);
//	if (it != _resourceTable.end())
//	{
//		// テーブル内にあったらロードするのではなく
//		// マップ内のリソースを返す
//		return it->second;
//	}
//
//	using LoadLambda_t = std::function<
//		HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;
//	static std::map<std::string, LoadLambda_t> loadLambdaTable;
//
//	if (loadLambdaTable.empty())
//	{
//		loadLambdaTable["sph"]
//			= loadLambdaTable["spa"]
//			= loadLambdaTable["bmp"]
//			= loadLambdaTable["png"]
//			= loadLambdaTable["jpg"]
//			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
//		{
//			return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
//		};
//		loadLambdaTable["tga"]
//			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
//		{
//			return LoadFromTGAFile(path.c_str(), meta, img);
//		};
//		loadLambdaTable["dds"]
//			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
//		{
//			return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
//		};
//	}
//
//	// テクスチャのロード
//	TexMetadata metadata = {};
//	ScratchImage scratchImg = {};
//
//	wstring wtexpath = GetWideStringFromString(texPath); // テクスチャのファイルパス
//	string ext = GetExtension(texPath); // 拡張子を取得
//	if (loadLambdaTable.find(ext) == loadLambdaTable.end()) { return nullptr; } // おかしな拡張子
//	auto result = loadLambdaTable[ext](
//		wtexpath,
//		&metadata,
//		scratchImg);
//	if (FAILED(result)) { return nullptr; }
//
//	if (FAILED(result)) { return nullptr; }
//
//	// WriteToSubresource で転送する用のヒープ設定
//	const D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
//		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
//		D3D12_MEMORY_POOL_L0);
//
//	const D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
//		metadata.format,
//		static_cast<UINT>(metadata.width), // 幅
//		static_cast<UINT>(metadata.height), // 高さ
//		static_cast<UINT16>(metadata.arraySize),
//		static_cast<UINT16>(metadata.mipLevels));
//
//	// バッファー作成
//	ComPtr<ID3D12Resource> texbuff = nullptr;
//	result = dev->CreateCommittedResource(
//		&texHeapProp,
//		D3D12_HEAP_FLAG_NONE, // 特に指定なし
//		&resDesc,
//		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//		nullptr,
//		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf())
//	);
//	if (FAILED(result)) { return nullptr; }
//
//	const Image* img = scratchImg.GetImage(0, 0, 0); // 生データ抽出
//
//	result = texbuff->WriteToSubresource(
//		0,
//		nullptr, // 全領域へコピー
//		img->pixels, // 元データアドレス
//		static_cast<UINT>(img->rowPitch), // 1 ラインサイズ
//		static_cast<UINT>(img->slicePitch) // 全サイズ
//	);
//	if (FAILED(result)) { return nullptr; }
//
//	_resourceTable[texPath] = texbuff;
//	return texbuff;
//}
//
//ComPtr<ID3D12Resource> CreateMonoTexture(ID3D12Device* dev, unsigned int val)
//{
//	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
//		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
//		D3D12_MEMORY_POOL_L0);
//
//	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
//		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);
//
//	ComPtr<ID3D12Resource> whiteBuff = nullptr;
//	auto result = dev->CreateCommittedResource(
//		&texHeapProp,
//		D3D12_HEAP_FLAG_NONE, // 特に指定なし
//		&resDesc,
//		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//		nullptr,
//		IID_PPV_ARGS(whiteBuff.ReleaseAndGetAddressOf())
//	);
//	if (FAILED(result)) { return nullptr; }
//
//	std::vector<unsigned char> data(4 * 4 * 4);
//	std::fill(data.begin(), data.end(), val); // 全部valで埋める
//
//	// データ転送
//	result = whiteBuff->WriteToSubresource(
//		0,
//		nullptr,
//		data.data(),
//		4 * 4,
//		static_cast<UINT>(data.size()));
//
//	return whiteBuff;
//}
//
//// デフォルトグラデーションテクスチャ
//ComPtr<ID3D12Resource> CreateGrayGradationTexture(ID3D12Device* dev)
//{
//	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
//		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
//		D3D12_MEMORY_POOL_L0);
//
//	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
//		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);
//
//	ComPtr<ID3D12Resource> gradBuff = nullptr;
//	HRESULT result = dev->CreateCommittedResource(
//		&texHeapProp,
//		D3D12_HEAP_FLAG_NONE, // 特に指定なし
//		&resDesc,
//		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//		nullptr,
//		IID_PPV_ARGS(gradBuff.ReleaseAndGetAddressOf())
//	);
//	if (FAILED(result)) { return nullptr; }
//
//	// 上が白くて下が黒いテクスチャデータを作成
//	std::vector<unsigned int>data(4 * 256);
//	auto it = data.begin();
//	unsigned int c = 0xff;
//	for (; it != data.end(); it += 4)
//	{	// RGBAが逆並びのためRGBマクロと0xff<<24を用いて表す
//		unsigned int col = (0xff << 24) | RGB(c, c, c);
//		std::fill(it, it + 4, col);
//		--c;
//	}
//	result = gradBuff->WriteToSubresource(
//		0,
//		nullptr,
//		data.data(),
//		4 * static_cast<UINT>(sizeof(unsigned int)),
//		static_cast<UINT>(sizeof(unsigned int) * data.size()));
//
//	return gradBuff;
//}
//
//ComPtr<ID3D12Resource> CreateWhiteTexture(ID3D12Device* dev)
//{
//	return CreateMonoTexture(dev, 0xff);
//}
//
//ComPtr<ID3D12Resource> CreateBlackTexture(ID3D12Device* dev)
//{
//	return CreateMonoTexture(dev, 0x00);
//}
//
//// アライメントにそろえたサイズを返す
//// @param size 元のサイズ
//// @param alignment アライメントサイズ
//// @return アライメントをそろえたサイズ
//size_t AlignmentedSize(size_t size, size_t alignment) 
//{
//	return size + alignment - size % alignment;
//}
//
//#ifdef _DEBUG
//	int main() {
//#else
//	#include<Windows.h>
//	int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
//#endif
//	
//	const unsigned int window_width = 1280;
//	const unsigned int window_height = 720;
//
//	ComPtr<ID3D12Device> _dev = nullptr;
//	ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
//	ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
//	ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
//	ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;
//	ComPtr<IDXGISwapChain4> _swapchain = nullptr;
//
//	HINSTANCE hInst = GetModuleHandle(nullptr);
//	//ウィンドウクラス生成＆登録
//	WNDCLASSEX w = {};
//	w.cbSize = sizeof(WNDCLASSEX);
//	w.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
//	w.lpszClassName = _T("DirectXTest");//アプリケーションクラス名(適当でいいです)
//	w.hInstance = GetModuleHandle(0);//ハンドルの取得
//	RegisterClassEx(&w);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)
//
//	RECT wrc = { 0,0, window_width, window_height };//ウィンドウサイズを決める
//	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
//	//ウィンドウオブジェクトの生成
//	HWND hwnd = CreateWindow(w.lpszClassName,//クラス名指定
//		_T("DX12テスト"),//タイトルバーの文字
//		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
//		CW_USEDEFAULT,//表示X座標はOSにお任せします
//		CW_USEDEFAULT,//表示Y座標はOSにお任せします
//		wrc.right - wrc.left,//ウィンドウ幅
//		wrc.bottom - wrc.top,//ウィンドウ高
//		nullptr,//親ウィンドウハンドル
//		nullptr,//メニューハンドル
//		w.hInstance,//呼び出しアプリケーションハンドル
//		nullptr);//追加パラメータ
//
//	HRESULT D3D12CreateDevice(
//		IUnknown * pAdapter, // ひとまずは nullptr で OK
//		D3D_FEATURE_LEVEL MinimumFeatureLevel, // 最低限必要なフィーチャーレベル
//		REFIID riid, // 後述
//		void** ppDevice // 後述
//	);
//
//	HRESULT CreateSwapChainForHwnd(
//		IUnknown * pDevice, //コマンドキューオブジェクト
//		HWND hWnd, //ウィンドウハンドル
//		const DXGI_SWAP_CHAIN_DESC1 * pDesc, // スワップチェーン設定
//		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pFullscreenDesc, // ひとまず nullptr でよい
//		IDXGIOutput * pRestrictToOutput, // これも nullptr でよい
//		IDXGISwapChain1 * *ppSwapChain // スワップチェーンオブジェクト取得用
//	);
//
//#ifdef _DEBUG
//	// デバッグレイヤーをオンに
//	EnableDebugLayer();
//#endif
//
//	D3D_FEATURE_LEVEL levels[] =
//	{
//		D3D_FEATURE_LEVEL_12_1,
//		D3D_FEATURE_LEVEL_12_0,
//		D3D_FEATURE_LEVEL_11_1,
//		D3D_FEATURE_LEVEL_11_0,
//	};
//
//#ifdef _DEBUG
//	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
//#else
//	auto result = CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
//#endif
//
//	// アダプターの列挙用
//	std::vector <IDXGIAdapter*> adapters;
//	// ここに特定の名前を持つアダプターオブジェクトが入る
//	IDXGIAdapter* tmpAdapter = nullptr;
//	for (int i = 0;
//		_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
//		++i)
//	{
//		adapters.push_back(tmpAdapter);
//	}
//	for (auto adpt : adapters)
//	{
//		DXGI_ADAPTER_DESC adesc = {};
//		adpt->GetDesc(&adesc); // アダプターの説明オブジェクト取得
//		std::wstring strDesc = adesc.Description;
//		// 探したいアダプターの名前を確認
//		if (strDesc.find(L"NVIDIA") != std::string::npos)
//		{
//			tmpAdapter = adpt;
//			break;
//		}
//	}
//
//	// Direct3D デバイスの初期化
//	D3D_FEATURE_LEVEL featureLevel;
//	for (auto lv : levels)
//	{
//		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
//		{
//			featureLevel = lv;
//			break; // 生成可能なバージョンが見つかったらループを打ち切り
//		}
//	}
//
//	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
//		IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));
//	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
//		_cmdAllocator.Get(), nullptr,
//		IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));
//
//	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
//	// タイムアウト無し
//	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//	// アダプターを 1 つしか使わないときは 0 でよい
//	cmdQueueDesc.NodeMask = 0;
//	// プライオリティは特に指定なし
//	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
//	// コマンドリストと合わせる
//	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
//	// キュー生成
//	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));
//
//	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
//	swapchainDesc.Width = window_width;
//	swapchainDesc.Height = window_height;
//	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	swapchainDesc.Stereo = false;
//	swapchainDesc.SampleDesc.Count = 1;
//	swapchainDesc.SampleDesc.Quality = 0;
//	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
//	swapchainDesc.BufferCount = 2;
//	// バックバッファーは伸び縮み可能
//	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
//	// フリップ後は速やかに破棄
//	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//	// 特に指定なし
//	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
//	// ウィンドウ⇔フルスクリーン切り替え可能
//	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
//	result = _dxgiFactory->CreateSwapChainForHwnd(
//		_cmdQueue.Get(), hwnd,
//		&swapchainDesc, nullptr, nullptr,
//		(IDXGISwapChain1**)_swapchain.GetAddressOf()); //	本来はQueryInterface などを用いて
//										 // IDXGISwapChain4 への変数チェックをするが、
//										 //	ここではわかりやすさ重視のためキャストで対応
//
//	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
//	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビューなので RTV
//	heapDesc.NodeMask = 0;
//	heapDesc.NumDescriptors = 2; // 表裏の 2 つ
//	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//
//	ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
//	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf()));
//
//	DXGI_SWAP_CHAIN_DESC swcDesc = {};
//	result = _swapchain->GetDesc(&swcDesc);
//
//	// レンダーターゲットビュー設定
//	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
//	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ガンマ補正なし
//	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
//
//	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
//	for (UINT idx = 0; idx < swcDesc.BufferCount; ++idx)
//	{
//		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
//		
//		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeaps->GetCPUDescriptorHandleForHeapStart());
//		handle.Offset(idx, _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
//		_dev->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
//	}
//
//	// 深度バッファーの作成
//	D3D12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
//		DXGI_FORMAT_D32_FLOAT,
//		window_width, window_height,
//		1U, 1U, 1U, 0U, // arraySize, mipLevels, sampleCount, SampleQuality
//		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
//
//	// 深度値用ヒーププロパティ
//	const D3D12_HEAP_PROPERTIES depthHeapProp =
//		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
//
//	// このクリアする際の値が重要な意味を持つ
//	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
//
//	ComPtr<ID3D12Resource> depthBuffer = nullptr;
//	result = _dev->CreateCommittedResource(
//		&depthHeapProp,
//		D3D12_HEAP_FLAG_NONE,
//		&depthResDesc,
//		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値書き込みに使用
//		&depthClearValue,
//		IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf()));
//
//	// 深度のためのディスクリプタヒープ作成
//	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // 深度に使うことがわかればよい
//	dsvHeapDesc.NumDescriptors = 1; // 深度ビューは1 つ
//	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // デプスステンシルビューとして使う
//	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
//	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf()));
//
//	// 深度ビュー作成
//	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
//	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値に32 ビット使用
//	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D テクスチャ
//	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // フラグは特になし
//
//	_dev->CreateDepthStencilView(
//		depthBuffer.Get(),
//		&dsvDesc,
//		dsvHeap->GetCPUDescriptorHandleForHeapStart());
//
//
//	ComPtr<ID3D12Fence> _fence = nullptr;
//	UINT64 _fenceVal = 0;
//	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));
//
//	// ウィンドウ表示
//	ShowWindow(hwnd, SW_SHOW);
//
//	ComPtr<ID3D12Resource> gradTex = CreateGrayGradationTexture(_dev.Get());
//	ComPtr<ID3D12Resource> whiteTex = CreateWhiteTexture(_dev.Get());
//	ComPtr<ID3D12Resource> blackTex = CreateBlackTexture(_dev.Get());
//
//	// PMD ヘッダー構造体
//	struct PMDHeader
//	{
//		float version; // 例：00 00 80 3F == 1.00
//		char model_name[20]; // モデル名
//		char comment[256]; // モデルコメント
//	};
//
//	char signature[3] = {}; // シグネチャ
//	PMDHeader pmdheader = {};
//	std::string strModelPath = "Model/初音ミク.pmd";
//	//std::string strModelPath = "Model/巡音ルカ.pmd";
//	//std::string strModelPath = "Model/初音ミクmetal.pmd";
//	FILE* fp;
//	fopen_s(&fp, strModelPath.c_str(), "rb");
//
//	fread(signature, sizeof(signature), 1, fp);
//	fread(&pmdheader, sizeof(pmdheader), 1, fp);
//
//	constexpr size_t pmdvertex_size = 38; // 頂点1つあたりのサイズ
//
//	unsigned int vertNum; // 頂点数を取得
//	fread(&vertNum, sizeof(vertNum), 1, fp);
//
//#pragma pack(1) // ここから1 バイトパッキングとなり、アライメントは発生しない
//	// PMD マテリアル構造体
//	struct PMDMaterial
//	{
//		XMFLOAT3 diffuse;		// ディフューズ色
//		float alpha;			// ディフューズα
//		float specularity;		// スペキュラの強さ（乗算値）
//		XMFLOAT3 specular;		// スペキュラ色
//		XMFLOAT3 ambient;		// アンビエント色
//		unsigned char toonIdx;	// トゥーン番号（後述）
//		unsigned char edgeFlg;	// マテリアルごとの輪郭線フラグ
//
//		// 注意：ここに2 バイトのパディングがある！！
//
//		unsigned int indicesNum;// このマテリアルが割り当てられる
//		// インデックス数（後述）
//		char texFilePath[20];	// テクスチャファイルパス＋α（後述）
//	}; // 70 バイトのはずだが、パディングが発生するため72 バイトになる
//#pragma pack() // パッキング指定を解除（デフォルトに戻す）
//
//
//
//#pragma pack(push, 1)
//	struct PMD_VERTEX
//	{
//		XMFLOAT3 pos;
//		XMFLOAT3 normal;
//		XMFLOAT2 uv;
//		uint16_t bone_no[2];
//		uint8_t  weight;
//		uint8_t  EdgeFlag;
//		uint16_t dummy;
//	};
//#pragma pack(pop)
//	std::vector<PMD_VERTEX> vertices(vertNum);// バッファーの確保
//	for (unsigned int i = 0; i < vertNum; i++)
//	{
//		fread(&vertices[i], pmdvertex_size, 1, fp);
//	}
//
//	ComPtr<ID3D12Resource> vertBuff = nullptr;
//	auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMD_VERTEX));
//	result = _dev->CreateCommittedResource(
//		&heapprop,
//		D3D12_HEAP_FLAG_NONE,
//		&resdesc, // サイズ変更
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));
//
//	PMD_VERTEX* vertMap = nullptr; // 型をVertex に変更
//	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
//	std::copy(std::begin(vertices), std::end(vertices), vertMap);
//	vertBuff->Unmap(0, nullptr);
//
//	D3D12_VERTEX_BUFFER_VIEW vbView = {};
//	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
//	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX));//全バイト数
//	vbView.StrideInBytes = sizeof(vertices[0]); // 1 頂点あたりのバイト数
//
//	unsigned int indicesNum;//インデックス数
//	fread(&indicesNum, sizeof(indicesNum), 1, fp);
//
//	std::vector<unsigned short> indices;
//	indices.resize(indicesNum);
//	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);
//
//	ComPtr<ID3D12Resource> idxBuff = nullptr;
//	// 設定は、バッファーのサイズ以外、頂点バッファーの設定を使い回してよい
//	resdesc.Width = indices.size() * sizeof(indices[0]);
//	result = _dev->CreateCommittedResource(
//		&heapprop,
//		D3D12_HEAP_FLAG_NONE,
//		&resdesc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));
//
//	// 作ったバッファーにインデックスデータをコピー
//	unsigned short* mappedIdx = nullptr;
//	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
//	std::copy(std::begin(indices), std::end(indices), mappedIdx);
//	idxBuff->Unmap(0, nullptr);
//
//	// インデックスバッファービューを作成
//	D3D12_INDEX_BUFFER_VIEW ibView = {};
//	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
//	ibView.Format = DXGI_FORMAT_R16_UINT;
//	ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));
//
//	unsigned int materialNum; // マテリアル数
//	fread(&materialNum, sizeof(materialNum), 1, fp);
//
//	std::vector<PMDMaterial> pmdMaterials(materialNum);
//
//	fread(
//		pmdMaterials.data(),
//		pmdMaterials.size() * sizeof(PMDMaterial),
//		1,
//		fp); // 一気に読み込む
//
//	// シェーダー側に投げられるマテリアルデータ
//	struct MaterialForHlsl
//	{
//		XMFLOAT3 diffuse; // ディフューズ色
//		float alpha; // ディフューズα
//		XMFLOAT3 specular; // スペキュラ色
//		float specularity; // スペキュラの強さ（乗算値）
//		XMFLOAT3 ambient; // アンビエント色
//	};
//
//	// それ以外のマテリアルデータ
//	struct AdditionalMaterial
//	{
//		std::string texPath; // テクスチャファイルパス
//		int toonIdx; // トゥーン番号
//		bool edgeFlg; // マテリアルごとの輪郭線フラグ
//	};
//
//	// 全体をまとめるデータ
//	struct Material
//	{
//		unsigned int indicesNum; // インデックス数
//		MaterialForHlsl material;
//		AdditionalMaterial additional;
//	};
//
//
//	std::vector<Material>materials(pmdMaterials.size());
//	// コピー
//	for (int i = 0; i < pmdMaterials.size(); ++i) 
//	{
//		materials[i].indicesNum = pmdMaterials[i].indicesNum;
//		materials[i].material.diffuse = pmdMaterials[i].diffuse;
//		materials[i].material.alpha = pmdMaterials[i].alpha;
//		materials[i].material.specular = pmdMaterials[i].specular;
//		materials[i].material.specularity = pmdMaterials[i].specularity;
//		materials[i].material.ambient = pmdMaterials[i].ambient;
//		materials[i].additional.toonIdx = pmdMaterials[i].toonIdx;
//	}
//
//	vector<ComPtr<ID3D12Resource>> textureResources(materialNum);
//	vector<ComPtr<ID3D12Resource>> sphResources(materialNum, nullptr);
//	vector<ComPtr<ID3D12Resource>> spaResources(materialNum, nullptr);
//	vector<ComPtr<ID3D12Resource>> toonResources(materialNum, nullptr);
//	for (int i = 0; i < pmdMaterials.size(); i++)
//	{
//		// てゅーんリソースの読み込み
//		string toonFilePath = "toon/";
//		char toonFileName[16];
//		sprintf_s(toonFileName, "toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
//		toonFilePath += toonFileName;
//		toonResources[i] = LoadTextureFromFile(toonFilePath, _dev.Get());
//
//		if (strlen(pmdMaterials[i].texFilePath) == 0)
//		{
//			textureResources[i] = nullptr;
//		}
//
//		std::string texFileName = pmdMaterials[i].texFilePath;
//		std::string sphFileName = "";
//		std::string spaFileName = "";
//
//		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0) 
//		{ // スプリッタがある
//			auto namepair = SplitFileName(texFileName);
//			if (GetExtension(namepair.first) == "sph")
//			{
//				texFileName = namepair.second;
//				sphFileName = namepair.first;
//			}
//			else if (GetExtension(namepair.first) == "spa")
//			{
//				texFileName = namepair.second;
//				spaFileName = namepair.first;
//			}
//			else
//			{
//				texFileName = namepair.first; // 他の拡張子でもとにかく最初の物を入れておく
//				if (GetExtension(namepair.second) == "sph")
//				{
//					sphFileName = namepair.second;
//				}
//				else
//				{
//					spaFileName = namepair.second;
//				}
//			}
//		}
//		else
//		{
//			string ext = GetExtension(pmdMaterials[i].texFilePath);
//			if (GetExtension(pmdMaterials[i].texFilePath) == "sph")
//			{
//				sphFileName = pmdMaterials[i].texFilePath;
//				texFileName = "";
//			}
//			else if (ext == "spa")
//			{
//				spaFileName = pmdMaterials[i].texFilePath;
//				texFileName = "";
//			}
//		}
//
//		// モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
//		auto texFilePath = GetTexturePathFromModelAndTexPath(
//			strModelPath,texFileName.c_str());
//		textureResources[i] = LoadTextureFromFile(texFilePath, _dev.Get());
//
//		if (sphFileName != "") {
//			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
//			sphResources[i] = LoadTextureFromFile(sphFilePath, _dev.Get());
//		}
//		if (spaFileName != "") {
//			auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
//			spaResources[i] = LoadTextureFromFile(spaFilePath, _dev.Get());
//		}
//	}
//
//	// マテリアルバッファーを作成
//	auto materialBuffSize = sizeof(MaterialForHlsl);
//	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;
//
//	ComPtr<ID3D12Resource> materialBuff = nullptr;
//
//	const D3D12_HEAP_PROPERTIES heapPropMat =
//		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//	const D3D12_RESOURCE_DESC resDescMat = CD3DX12_RESOURCE_DESC::Buffer(
//		materialBuffSize * materialNum);
//	result = _dev->CreateCommittedResource(
//		&heapPropMat,
//		D3D12_HEAP_FLAG_NONE,
//		&resDescMat,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf())
//	);
//
//	// マップマテリアルにコピー
//	char* mapMaterial = nullptr;
//	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
//	for (auto& m : materials) {
//		*((MaterialForHlsl*)mapMaterial) = m.material; // データコピー
//		mapMaterial += materialBuffSize; // 次のアライメント位置まで進める（256 の倍数）
//	}
//	materialBuff->Unmap(0, nullptr);
//
//	ComPtr<ID3D12DescriptorHeap> materialDescHeap = nullptr;
//
//	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
//	materialDescHeapDesc.Flags =
//		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	materialDescHeapDesc.NodeMask = 0;
//	materialDescHeapDesc.NumDescriptors = materialNum * 5;  // マテリアル数分（定数1つ、テクスチャ4つ）
//	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//
//	result = _dev->CreateDescriptorHeap(
//		&materialDescHeapDesc, IID_PPV_ARGS(materialDescHeap.ReleaseAndGetAddressOf()));
//
//	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
//
//	matCBVDesc.BufferLocation =
//		materialBuff->GetGPUVirtualAddress(); // バッファーアドレス
//	matCBVDesc.SizeInBytes =
//		static_cast<UINT>(materialBuffSize); // マテリアルの256 アライメントサイズ
//
//	// 通常テクスチャビュー作成
//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//
//	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // デフォルト
//	srvDesc.Shader4ComponentMapping =
//		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 後述
//	srvDesc.ViewDimension =
//		D3D12_SRV_DIMENSION_TEXTURE2D; // 2D テクスチャ
//	srvDesc.Texture2D.MipLevels = 1; // ミップマップは使用しないので1
//
//	// 先頭の記録
//	auto matDescHeapH =
//		materialDescHeap->GetCPUDescriptorHandleForHeapStart();
//	UINT incSize = _dev->GetDescriptorHandleIncrementSize(
//		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//
//	for (UINT i = 0; i < materialNum; ++i)
//	{
//		// マテリアル用定数バッファービュー
//		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
//		matDescHeapH.ptr += incSize;
//		matCBVDesc.BufferLocation += materialBuffSize;
//		
//		if (textureResources[i] == nullptr) 
//		{
//			srvDesc.Format = whiteTex->GetDesc().Format;
//			_dev->CreateShaderResourceView(
//				whiteTex.Get(), &srvDesc, matDescHeapH);
//		}
//		else
//		{
//			srvDesc.Format = textureResources[i]->GetDesc().Format;
//			_dev->CreateShaderResourceView(
//				textureResources[i].Get(),
//				&srvDesc,
//				matDescHeapH);
//		}
//
//		matDescHeapH.ptr += incSize;
//
//		if (sphResources[i] == nullptr)
//		{
//			srvDesc.Format = whiteTex->GetDesc().Format;
//			_dev->CreateShaderResourceView(
//				whiteTex.Get(), &srvDesc, matDescHeapH);
//		}
//		else
//		{
//			srvDesc.Format = sphResources[i]->GetDesc().Format;
//			_dev->CreateShaderResourceView(
//				sphResources[i].Get(), &srvDesc, matDescHeapH);
//		}
//		matDescHeapH.ptr += incSize;
//
//		if (spaResources[i] == nullptr)
//		{
//			srvDesc.Format = blackTex->GetDesc().Format;
//			_dev->CreateShaderResourceView(
//				blackTex.Get(), &srvDesc, matDescHeapH);
//		}
//		else
//		{
//			srvDesc.Format = spaResources[i]->GetDesc().Format;
//			_dev->CreateShaderResourceView(
//				spaResources[i].Get(), &srvDesc, matDescHeapH);
//		}
//		matDescHeapH.ptr += incSize;
//
//		if (toonResources[i] == nullptr)
//		{
//			srvDesc.Format = gradTex->GetDesc().Format;
//			_dev->CreateShaderResourceView(gradTex.Get(), &srvDesc, matDescHeapH);
//		}
//		else
//		{
//			srvDesc.Format = toonResources[i]->GetDesc().Format;
//			_dev->CreateShaderResourceView(toonResources[i].Get(), &srvDesc, matDescHeapH);
//		}
//		matDescHeapH.ptr += incSize;
//	}
//
//	fclose(fp);
//
//	ComPtr<ID3DBlob> _vsBlob = nullptr;
//	ComPtr<ID3DBlob> _psBlob = nullptr;
//
//	ComPtr<ID3DBlob> errorBlob = nullptr;
//	result = D3DCompileFromFile(
//		L"BasicVertexShader.hlsl", // シェーダ―名
//		nullptr, // define はなし
//		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
//		"BasicVS", "vs_5_0", // 関数はBasicVS、対象シェーダーはvs_5_0
//		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用および最適化なし
//		0,
//		&_vsBlob, &errorBlob); // エラー時はerrorBlob にメッセージが入る
//	if (FAILED(result)) {
//		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
//			::OutputDebugStringA("ファイルが見当たりません");
//		}
//		else {
//			std::string errstr;
//			errstr.resize(errorBlob->GetBufferSize());
//			std::copy_n((char*)errorBlob->GetBufferPointer(),
//				errorBlob->GetBufferSize(), errstr.begin());
//			errstr += "\n";
//			OutputDebugStringA(errstr.c_str());
//		}
//		exit(1); // 行儀悪いかな…
//	}
//
//	result = D3DCompileFromFile(
//		L"BasicPixelShader.hlsl", // シェーダ―名
//		nullptr, // define はなし
//		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
//		"BasicPS", "ps_5_0", // 関数はBasicPS、対象シェーダーはps_5_0
//		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用および最適化なし
//		0,
//		&_psBlob, &errorBlob); // エラー時はerrorBlob にメッセージが入る
//	if (FAILED(result)) {
//		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
//			::OutputDebugStringA("ファイルが見当たりません");
//		}
//		else {
//			std::string errstr;
//			errstr.resize(errorBlob->GetBufferSize());
//			std::copy_n((char*)errorBlob->GetBufferPointer(),
//				errorBlob->GetBufferSize(), errstr.begin());
//			errstr += "\n";
//			OutputDebugStringA(errstr.c_str());
//		}
//		exit(1); // 行儀悪いかな…
//	}
//
//	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
//	{
//		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"BONE_NO", 0, DXGI_FORMAT_R16G16_UINT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//		{"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT, 0,
//			D3D12_APPEND_ALIGNED_ELEMENT,
//			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//	};
//
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
//
//	gpipeline.pRootSignature = nullptr; // あとで設定する
//
//	gpipeline.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());
//	gpipeline.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());
//
//	// デフォルトのサンプルマスクを表す定数（0xffffffff）
//	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//
//	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングしない
//
//	//深度ステンシル
//	gpipeline.DepthStencilState.DepthEnable = true; // 深度バッファーを使う
//	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 書き込む
//	gpipeline.DepthStencilState.DepthFunc =
//		D3D12_COMPARISON_FUNC_LESS; // 小さいほうを採用
//	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
//
//	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//
//	gpipeline.InputLayout.pInputElementDescs = inputLayout; // レイアウト先頭アドレス
//	gpipeline.InputLayout.NumElements = _countof(inputLayout); // レイアウト配列の要素数
//
//	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // ストリップ時のカットなし
//	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 点で構成
//
//	gpipeline.NumRenderTargets = 1; // 今は１つのみ
//	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0～1に正規化されたRGBA
//
//	gpipeline.SampleDesc.Count = 1; // サンプリングは1ピクセルにつき1
//	gpipeline.SampleDesc.Quality = 0; // クオリティは最低
//
//	CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {}; // テクスチャと定数の2つ
//	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);// 定数[b0] 座標変換用
//	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);// 定数[b1] 座標変換用
//	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);// テクスチャ4つ
//
//	CD3DX12_ROOT_PARAMETER rootparam[2] = {};
//	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);
//	rootparam[1].InitAsDescriptorTable(2, &descTblRange[1]);
//
//	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
//	samplerDesc[0].Init(0);
//	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
//
//	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
//	rootSignatureDesc.Init(
//		2, rootparam,
//		2, samplerDesc,
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//	ComPtr<ID3DBlob> rootSigBlob = nullptr;
//	result = D3D12SerializeRootSignature(
//		&rootSignatureDesc, // ルートシグネチャ設定
//		D3D_ROOT_SIGNATURE_VERSION_1_0, // ルートシグネチャバージョン
//		&rootSigBlob, // シェーダーを作った時と同じ
//		&errorBlob); // エラー処理も同じ
//
//	ComPtr<ID3D12RootSignature> rootsignature = nullptr;
//	result = _dev->CreateRootSignature(
//		0, // nodemask。0 でよい
//		rootSigBlob->GetBufferPointer(), // シェーダ―の時と同様
//		rootSigBlob->GetBufferSize(), // シェーダーの時と同様
//		IID_PPV_ARGS(rootsignature.GetAddressOf()));
//	rootSigBlob->Release(); // 不要になったので解放
//
//	gpipeline.pRootSignature = rootsignature.Get();
//
//	ComPtr<ID3D12PipelineState> _pipelinestate = nullptr;
//	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf()));
//
//	CD3DX12_VIEWPORT viewport(_backBuffers[0]);
//
//	// ビューポート全てを表示する設定
//	CD3DX12_RECT scissorrect(0, 0, window_width, window_height);
//
//	// WICテクスチャのロード
//	TexMetadata metadata = {};
//	ScratchImage scratchImg = {};
//
//	result = LoadFromWICFile(
//		L"img/textest.png", WIC_FLAGS_NONE,
//		&metadata, scratchImg);
//
//	auto img = scratchImg.GetImage(0, 0, 0); // 生データ抽出
//
//	// 中間バッファーとしてのアップロードヒープ設定
//	const D3D12_HEAP_PROPERTIES uploadHeapProp =
//		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//
//	D3D12_RESOURCE_DESC resDesc = {};
//	resDesc.Format = DXGI_FORMAT_UNKNOWN; // 単なるデータの塊なのでUNKNOWN
//	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // 単なるバッファーとして指定
//
//	resDesc.Width =
//		AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
//		* img->height;
//	resDesc.Height = 1;
//	resDesc.DepthOrArraySize = 1;
//	resDesc.MipLevels = 1;
//
//	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // 連続したデータ
//	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 特にフラグなし
//
//	resDesc.SampleDesc.Count = 1; // 通常テクスチャなのでアンチエイリアシングしない
//	resDesc.SampleDesc.Quality = 0;
//
//	// 中間バッファー作成
//	ComPtr<ID3D12Resource> uploadbuff = nullptr;
//
//	result = _dev->CreateCommittedResource(
//		&uploadHeapProp,
//		D3D12_HEAP_FLAG_NONE, // 特に指定なし
//		&resDesc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(uploadbuff.ReleaseAndGetAddressOf())
//	);
//
//	// テクスチャの為のヒープ設定
//	const D3D12_HEAP_PROPERTIES texHeapProp =
//		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
//
//	// リソース設定（変数は使いまわし）
//	resDesc.Format = metadata.format;
//	resDesc.Width = static_cast<UINT>(metadata.width); // 幅
//	resDesc.Height = static_cast<UINT>(metadata.height); // 高さ
//	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
//	resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
//	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
//	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // レイアウトは決定しない
//
//	ComPtr<ID3D12Resource> texbuff = nullptr;
//	result = _dev->CreateCommittedResource(
//		&texHeapProp,
//		D3D12_HEAP_FLAG_NONE, // 特に指定なし
//		&resDesc,
//		D3D12_RESOURCE_STATE_COPY_DEST, // コピー先
//		nullptr,
//		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf()));
//
//	uint8_t* mapforImg = nullptr; // image->pixels と同じ型にする
//	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg); // マップ
//
//	auto srcAddress = img->pixels;
//
//	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
//
//	for (int y = 0; y < img->height; ++y)
//	{
//		std::copy_n(srcAddress,
//			rowPitch,
//			mapforImg); // コピー
//
//		// 1 行ごとのつじつまを合わせる
//		srcAddress += img->rowPitch;
//		mapforImg += rowPitch;
//	}
//
//	//std::copy_n(img->pixels, img->slicePitch, mapforImg); // コピー
//	uploadbuff->Unmap(0, nullptr); // アンマップ
//
//	D3D12_RESOURCE_BARRIER BarrierDesc = {};
//	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	BarrierDesc.Transition.pResource = texbuff.Get();
//	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
//	BarrierDesc.Transition.StateBefore =
//		D3D12_RESOURCE_STATE_COPY_DEST; // ここが重要
//	BarrierDesc.Transition.StateAfter =
//		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // ここも重要
//
//	_cmdList->ResourceBarrier(1, &BarrierDesc);
//	_cmdList->Close();
//
//	// コマンドリストの実行
//	ID3D12CommandList* cmdlists[] = { _cmdList.Get()};
//	_cmdQueue->ExecuteCommandLists(1, cmdlists);
//
//	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
//
//	if (_fence->GetCompletedValue() != _fenceVal)
//	{
//		auto event = CreateEvent(nullptr, false, false, nullptr);
//		_fence->SetEventOnCompletion(_fenceVal, event);
//		WaitForSingleObject(event, INFINITE);
//		CloseHandle(event);
//	}
//	_cmdAllocator->Reset(); // キューをクリア
//	_cmdList->Reset(_cmdAllocator.Get(), nullptr);
//
//	D3D12_TEXTURE_COPY_LOCATION src = {};
//
//	// コピー元（アップロード側）設定
//	src.pResource = uploadbuff.Get(); // 中間バッファー
//	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; // フットプリント指定
//	src.PlacedFootprint.Offset = 0;
//	src.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
//	src.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
//	src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
//	src.PlacedFootprint.Footprint.RowPitch =
//		static_cast<UINT>(AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
//	src.PlacedFootprint.Footprint.Format = img->format;
//
//	D3D12_TEXTURE_COPY_LOCATION dst = {};
//
//	// コピー先設定
//	dst.pResource = texbuff.Get();
//	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
//	dst.SubresourceIndex = 0;
//
//	_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
//
//	// 定数バッファー作成
//	XMMATRIX worldMat = XMMatrixRotationY(0);
//
//	XMFLOAT3 eye(0, 10, -15);
//	XMFLOAT3 target(0, 10, 0);
//	XMFLOAT3 up(0, 1, 0);
//
//	auto viewMat = XMMatrixLookAtLH(
//		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
//
//	auto projMat = XMMatrixPerspectiveFovLH(
//		XM_PIDIV2, // 画角は90°
//		static_cast<float>(window_width)
//		/ static_cast<float>(window_height), // アスペクト比
//		1.0f, // 近いほう
//		100.0f // 遠いほう
//	);
//
//	// シェーダー側に渡すための基本的な行列データ
//	struct SceneData
//	{
//		XMMATRIX world; // モデル本体を回転させたり移動させてたりする行列
//		XMMATRIX view; // ビュー行列
//		XMMATRIX proj; // プロジェクション行列
//		XMFLOAT3 eye; // 視点座標
//	};
//
//	ComPtr<ID3D12Resource> constBuff = nullptr;
//	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
//	_dev->CreateCommittedResource(
//		&heapProp,
//		D3D12_HEAP_FLAG_NONE,
//		&resDesc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf())
//	);
//
//	SceneData* mapScene; // マップ先を示すポインター
//	result = constBuff->Map(0, nullptr, (void**)&mapScene); // マップ
//	mapScene->world = worldMat;
//	mapScene->view = viewMat;
//	mapScene->proj = projMat;
//	mapScene->eye = eye;
//
//	ComPtr<ID3D12DescriptorHeap> basicDescHeap = nullptr;
//	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
//	// シェーダーから見えるように
//	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	// マスクは0
//	descHeapDesc.NodeMask = 0;
//	// SRC 1つと CBV 1つ
//	descHeapDesc.NumDescriptors = 1;
//	// シェーダーリソースビュー用
//	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	// 生成
//	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));
//
//	// デスクリプタの先頭ハンドルを取得しておく
//	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
//	
//	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
//	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
//	cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
//
//	// 定数バッファービューの作成
//	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
//
//	float angle = 0.0f;	
//
//	while (true)
//	{
//		MSG msg;
//		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
//		{
//			// アプリケーションが終わるときに message が WM_QUITになる
//			if (msg.message == WM_QUIT)
//			{
//				break;
//			}
//
//			TranslateMessage(&msg);
//			DispatchMessage(&msg);
//		}
//
//		//angle += 1.0f;
//		mapScene->world = worldMat;
//		mapScene->view = viewMat;
//		mapScene->proj = projMat;
//
//		// DirectX処理
//		// バックバッファのインデックスを取得
//		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
//
//		auto BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
//			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
//			D3D12_RESOURCE_STATE_RENDER_TARGET); // これだけで済む
//		_cmdList->ResourceBarrier(1, &BarrierDesc); // バリア指定実行
//
//		// パイプラインステートのセット
//		_cmdList->SetPipelineState(_pipelinestate.Get());
//
//		// レンダーターゲットを指定
//		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvH(rtvHeaps->GetCPUDescriptorHandleForHeapStart());
//		rtvH.Offset(bbIdx, _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
//		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvH(dsvHeap->GetCPUDescriptorHandleForHeapStart());
//		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);
//
//		// 画面クリア
//		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
//		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
//		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
//
//		// ルートシグネチャのセット
//		_cmdList->SetGraphicsRootSignature(rootsignature.Get());
//		// ディスクリプタヒープのセット
//		_cmdList->SetDescriptorHeaps(1, basicDescHeap.GetAddressOf());
//		// ルートパラメーターとディスクリプタヒープの関連付け
//		_cmdList->SetGraphicsRootDescriptorTable(
//			0, // ルートパラメーターインデックス
//			basicDescHeap->GetGPUDescriptorHandleForHeapStart()); // ヒープアドレス
//		_cmdList->SetDescriptorHeaps(1, materialDescHeap.GetAddressOf());
//		_cmdList->SetGraphicsRootDescriptorTable(
//			1,
//			materialDescHeap->GetGPUDescriptorHandleForHeapStart());
//		// ビューポートとシザー矩形のセット
//		_cmdList->RSSetViewports(1, &viewport);
//		_cmdList->RSSetScissorRects(1, &scissorrect);
//		// プリミティブトポロジのセット
//		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//		// 頂点情報のセット
//		_cmdList->IASetVertexBuffers(0, 1, &vbView);
//		_cmdList->IASetIndexBuffer(&ibView);
//
//		_cmdList->SetDescriptorHeaps(1, materialDescHeap.GetAddressOf());
//		auto materialH = materialDescHeap->
//			GetGPUDescriptorHandleForHeapStart(); // ヒープ先頭
//
//		unsigned int idxOffset = 0; // 最初はオフセットなし
//
//		UINT cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(
//			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
//
//		for (auto& m : materials)
//		{
//			_cmdList->SetGraphicsRootDescriptorTable(1, materialH);
//
//			_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);
//
//			// ヒープポインターとインデックスを次に進める
//			materialH.ptr += cbvsrvIncSize;
//
//			idxOffset += m.indicesNum;
//		}
//
//		// 描画(Draw)命令
//		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);
//
//		// 前後だけ入れ替える
//		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
//			_backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET,
//			D3D12_RESOURCE_STATE_PRESENT);
//		_cmdList->ResourceBarrier(1, &BarrierDesc);
//
//		// 命令のクローズ
//		_cmdList->Close();
//
//		// コマンドリストの実行
//		ID3D12CommandList* cmdlists[] = { _cmdList.Get()};
//		_cmdQueue->ExecuteCommandLists(1, cmdlists);
//
//		////待ち
//		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
//
//		while (_fence->GetCompletedValue() != _fenceVal) {
//			auto event = CreateEvent(nullptr, false, false, nullptr);
//			_fence->SetEventOnCompletion(_fenceVal, event); // イベントハンドルの取得
//			WaitForSingleObject(event, INFINITE); // イベントが発生するまで無限に待つ
//			CloseHandle(event); // イベントハンドルを閉じる
//		}
//
//		_cmdAllocator->Reset(); // キューをクリア
//		_cmdList->Reset(_cmdAllocator.Get(), nullptr); // 再びコマンドリストをためる準備
//
//		// フリップ
//		_swapchain->Present(1, 0);
//	}
//
//	// もうクラスは使わないので登録解除する
//	UnregisterClass(w.lpszClassName, w.hInstance);
//
//	return 0;
//}
//
