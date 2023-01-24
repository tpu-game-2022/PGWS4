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

	//シェーダー側に投げられるマテリアルデータ
	struct MaterialForHlsl
	{
		DirectX::XMFLOAT3 diffuse;//ディフューズ色
		float alpha;//ディフューズα
		DirectX::XMFLOAT3 specular;//スぺキュラ色
		float specularity;//スぺキュラの強さ(乗算値)
		DirectX::XMFLOAT3 ambient;//アビエント色
	};

	//それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		std::string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	//全体をまとめるデータ
	struct Material
	{
		unsigned int indicesNum;
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	std::vector<Material> materials;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _toonResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> _materialBuff = nullptr;
	unsigned int _materialNum;//マテリアル数
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialDescHeap = nullptr;//マテリアルヒープ

	Microsoft::WRL::ComPtr<ID3D12Resource> _vertBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> _idxBuff = nullptr;
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	struct Transform {
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};
	Transform _transform;
	DirectX::XMMATRIX* _mappedMatrices = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformMat = nullptr;//座標変換行列(今はワールドのみ)
	
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _transformHeap = nullptr;//座標変換ヒープ

	//ボーン関連
	std::vector<DirectX::XMMATRIX> _boneMatrices;// GPUへコピーするためのボーン情報

	struct BoneNode {
		int boneIdx;					//ボーンインデックス
		DirectX::XMFLOAT3 startPos;		//ボーン基準点(回転中心)
		std::vector<BoneNode*> children;//子ノード
	};
	std::map<std::string, BoneNode> _boneNodeTable;// 名前で骨を検索できるように

	void RecursiveMatrixMultipy(BoneNode& node, const DirectX::XMMATRIX& mat);

	///キーフレーム構造体
	struct KeyFrame {
		unsigned int frameNo;//フレーム№(アニメーション開始からの経過時間)
		DirectX::XMVECTOR quaternion;//クォータニオン
		DirectX::XMFLOAT2 p1, p2;
		KeyFrame(
			unsigned int fno,
			const DirectX::XMVECTOR& q,
			const DirectX::XMFLOAT2 ip1, const DirectX::XMFLOAT2 ip2):
			frameNo(fno),
			quaternion(q),p1(ip1),p2(ip2) {}
	};

	std::map<std::string, std::vector<KeyFrame>> _motiondata;

	DWORD _startTime;//アニメーション開始時点のミリ秒時刻
	unsigned int _duration = 0;
	void MotionUpdate();

	float _angle;//テスト用Y軸回転

public:
	HRESULT LoadPMDFile(const char* path);//PMDファイルのロード
	void CreateMaterialData();//読み込んだマテリアルをもとにマテリアルバッファを作成
	void CreateMaterialAndTextureView();//マテリアル＆テクスチャのビューを作成
	void CreateTransformView();

public:
	PMDActor(const char* filepath, PMDRenderer& renderer);
	~PMDActor();
	PMDActor* Clone();

	void LoadVMDFile(const char* filepath, const char* name);

	void Update();
	void Draw();

	void PlayAnimaton();
};