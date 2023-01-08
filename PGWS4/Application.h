#pragma once
#include<Windows.h>
#include <string>
//読み込み関数
#include<map>
#include <functional>
//表記に必要
#include<dxgi1_6.h>
//DirectX12のオブジェクトとして各命令を出すのに必要
#include<d3d12.h>
//コード減らし用
#include<d3dx12.h>
//ライブラリ読み込み
#include<DirectXTex.h>
#include<wrl.h>

class Application
{
private:
	Application();
	~Application();
	Application(const Application&) = delete;
	void operator=(const Application&) = delete;

	Microsoft::WRL::ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath, ID3D12Device* dev);

public:
	//ウィンドウ周り
	WNDCLASSEX _windowClass;
	HWND _hwnd;
	HWND CreateGameWindow(WNDCLASSEX _windowClass, int _window_width, int _window_height);

	//DirectX12システム
	Microsoft::WRL::ComPtr<ID3D12Device> _dev = nullptr;
	Microsoft::WRL::ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapchain = nullptr;
	void InitializeCommand();
	void CreateSwapChain();
	void InitializeBackBuffers();
	void InitializeDepthBuffer();

	//同期
	Microsoft::WRL::ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

	//画面スクリーン
	const unsigned int _window_width = 1280;
	const unsigned int _window_height = 720;
	Microsoft::WRL::ComPtr<ID3D12Resource> _depthBuffer = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _backBuffers; //バックバッファ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _rtvHeaps = nullptr;//レンダーターゲット用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap = nullptr;//深度バッファビュー用デスクリプタヒープ
	CD3DX12_VIEWPORT _viewport;//ビューポート
	CD3DX12_RECT _scissorrect;//シザー矩形

	//デフォルトテクスチャ(白、黒、グレイスケールグラデーション)
	Microsoft::WRL::ComPtr<ID3D12Resource> _whiteTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _blackTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _gradTex = nullptr;
	void InitialixeTextureLoaderTable(FILE* fp, std::string strModelPath);

	//リソース管理
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> _resourceTable;
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path,
		DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> _loadLambdaTable;

	//描画状態の設定
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelinestate = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootsigunature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _constBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _basicDescHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialDescHeap = nullptr;
	void CreateRootSignature(Microsoft::WRL::ComPtr<ID3DBlob> errorBlob);

	//シェーダー側に渡すための基本的な環境データ
	struct SceneData
	{
		DirectX::XMMATRIX world; //ワールド行列
		DirectX::XMMATRIX view; //ビュー行列
		DirectX::XMMATRIX proj; //プロジェクション行列
		DirectX::XMFLOAT3 eye; //視点座標
	};
	SceneData* _mapScene; //マップ先を示すポインター

	//シーン情報
	DirectX::XMMATRIX _worldMat;
	DirectX::XMMATRIX _viewMat;
	DirectX::XMMATRIX _projMat;

	//マテリアルデータ
	struct MaterialForHlsl //シェーダー側に投げられるマテリアルデータ
	{
		DirectX::XMFLOAT3 diffuse; //ディフューズ色
		float alpha; //ディフューズa
		DirectX::XMFLOAT3 specular; //スペキュラ色
		float specularity; //スペキュラの強さ（乗算地）
		DirectX::XMFLOAT3 ambient; //アンビエント色
	};

	struct AdditionalMaterial //それ以外のマテリアルデータ
	{
		std::string texPath; //テクスチャファイルパス
		int toonIdx; //トゥーン番号
		bool edgeFlg; //マテリアルごとの輪郭線フラグ
	};

	struct Material //全体をまとめるデータ
	{
		unsigned int indicesNum; //インデックス数
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};
	std::vector<Material> materials;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _toonResources;

	unsigned int _materialNum; //マテリアル数
	Microsoft::WRL::ComPtr<ID3D12Resource> _materialBuff = nullptr;

	//モデル情報
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _idxBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	//シングルトンインスタンス
	static Application& Instance();

	//初期化
	bool Init();

	//ループ起動
	void Run();

	//後処理
	void Terminate();
};