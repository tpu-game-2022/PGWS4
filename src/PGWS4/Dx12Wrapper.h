#pragma once
#include <Windows.h>
#include <vector>
#include <map>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXTex.h>
#include <wrl.h>

class Dx12Wrapper
{
private:
	unsigned int _screen_size[2];

	// DirectX12 システム
	Microsoft::WRL::ComPtr<ID3D12Device> _dev = nullptr;
	Microsoft::WRL::ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapchain = nullptr;

	// 同期
	Microsoft::WRL::ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

	// 画面スクリーン
	unsigned int _window_width = 1280;
	unsigned int _window_height = 720;
	Microsoft::WRL::ComPtr<ID3D12Resource> _depthBuffer = nullptr;
	std::vector<ID3D12Resource*> _backBuffers;//バックバッファ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _rtvHeaps = nullptr;//レンダーターゲット用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap = nullptr;//深度バッファビュー用デスクリプタヒープ

	CD3DX12_VIEWPORT _viewport;//ビューポート
	CD3DX12_RECT _scissorrect;//シザー矩形

	// リソース管理
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> _textureTable;
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path,
		DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> _loadLambdaTable;



	//シーンを構成するバッファまわり
	DirectX::XMMATRIX _worldMat;
	DirectX::XMMATRIX _viewMat;
	DirectX::XMMATRIX _projMat;

	struct SceneData
	{
		DirectX::XMMATRIX world; // ワールド行列
		DirectX::XMMATRIX view; // ビュー行列
		DirectX::XMMATRIX proj; // プロジェクション行列
		DirectX::XMFLOAT3 eye; // 視点座標
	};
	SceneData* _mappedSceneData; // マップ先を示すポインター
	Microsoft::WRL::ComPtr<ID3D12Resource> _sceneConstBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _sceneDescHeap = nullptr;



private:
	static Microsoft::WRL::ComPtr<IDXGIFactory6> InitializeGraphicsInterface();
	static Microsoft::WRL::ComPtr<ID3D12Device> InitializeDevice(IDXGIFactory6* dxgiFactory);
	static void InitializeCommand(Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& cmdAllocator,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& cmdList,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>& cmdQueue, ID3D12Device* dev);
	static Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(unsigned int width, unsigned int height,
		IDXGIFactory6* dxgiFactory, ID3D12CommandQueue* cmdQueue, HWND hwnd);
	static void InitializeBackBuffers(std::vector<ID3D12Resource*>& backBuffers, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& rtvHeaps,
		ID3D12Device* dev, IDXGISwapChain4* swapchain);
	void InitializeDepthBuffer(Microsoft::WRL::ComPtr<ID3D12Resource>& depthBuffer, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& dsvHeap,
		unsigned int width, unsigned int height, ID3D12Device* dev);
	UINT64 InitializeFence(Microsoft::WRL::ComPtr<ID3D12Fence>& fence, ID3D12Device* dev);

	void InitializeMatrixes();
	void CreateSceneView();

	// 初期化の部分処理
	Microsoft::WRL::ComPtr<ID3D12Resource> LoadTextureFromFile(const char* texPath);
	void InitializeTextureLoaderTable(std::map<std::string, LoadLambda_t>& loadLambdaTable);


public:
	Dx12Wrapper(HWND hwnd, unsigned int width, unsigned int height);
	~Dx12Wrapper();

	void BeginDraw();	// フレーム全体の描画準備
	void EndDraw();		// フレーム全体の片付け

	void ApplySceneDescHeap();

	// その他、公開するメソッド
	Microsoft::WRL::ComPtr<ID3D12Device> Device() { return _dev; }//デバイス
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList() {return _cmdList;}//コマンドリスト

	///テクスチャパスから必要なテクスチャバッファへのポインタを返す
	Microsoft::WRL::ComPtr<ID3D12Resource> GetTextureByPath(const char* texpath);
};


//Microsoft::WRL::ComPtr<IDXGISwapChain4> Swapchain();//スワップチェイン

//void SetScene();

