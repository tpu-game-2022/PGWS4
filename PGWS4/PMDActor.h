#pragma once
#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<map>
#include<string>
#include<wrl.h>

class Dx12Wrapper;
class PMDRenderer;

class PMDActor
{
	friend PMDRenderer;
private:
	PMDRenderer& _renderer;
	Dx12Wrapper& _dx12;

	struct MaterialForHlsl
	{
		DirectX::XMFLOAT3 diffuse;
		float alpha;
		DirectX::XMFLOAT3 specular;
		float specularity;
		DirectX::XMFLOAT3 ambient;
	};

	struct AdditionalMaterial
	{
		std::string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	struct Material
	{
		unsigned int indicesNum;
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	unsigned int _materialNum;
	std::vector<Material> materials;
	Microsoft::WRL::ComPtr<ID3D12Resource> _materialBuff = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _toonResources;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialDescHeap = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> _vertBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _idxBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	struct Transform
	{
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	Transform _transform;
	//XMMATRIXの配列としてまとめてデータを転送(マップ)
	DirectX::XMMATRIX* _mappedMatrices = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformMat = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _transformHeap = nullptr;

	//ボーン関連(private内)
	std::vector<DirectX::XMMATRIX> _boneMatrices;//GPUへコピーするためのボーン情報

	struct BoneNode
	{
		int boneIdx;					//ボーンインデックス
		DirectX::XMFLOAT3 startPos;		//ボーン基準点(回転中心)
		std::vector<BoneNode*> children;//子ノード
	};
	std::map<std::string, BoneNode> _boneNodeTable;//名前で骨を検索できるように

	//再帰処理で末端まで回転を伝播させる
	void RecursiveMatrixMultipy(BoneNode& node, const DirectX::XMMATRIX& mat);

	//キーフレーム構造体
	struct KeyFrame
	{
		unsigned int frameNo;//フレームNo.(アニメーション開始からの経過時間)
		DirectX::XMVECTOR quaternion;//クォータニオン
		KeyFrame(
			unsigned int fno,
			const DirectX::XMVECTOR& q) :
			frameNo(fno),
			quaternion(q){}
	};
	std::map<std::string, std::vector<KeyFrame>> _motiondata;

	float _angle;

public:
	HRESULT LoadPMDFile(const char* path);
	void CreateMaterialData();
	void CreateMaterialAndTextureView();
	void CreateTransformView();

public:
	PMDActor(const char* filepath, PMDRenderer& renderer);
	~PMDActor();

	//クローンは頂点およびマテリアルは共通のバッファを見るようにする
	PMDActor* Clone();

	//読み込み関数
	void LoadVMDFile(const char* filepath, const char* name);

	void Update();
	void Draw();
};