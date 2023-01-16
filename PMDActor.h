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

	// マテリアルデータ
	struct MaterialForHlsl// シェーダー側に投げられるマテリアルデータ
	{
		DirectX::XMFLOAT3 diffuse; // ディフューズ色
		float alpha; // ディフューズα
		DirectX::XMFLOAT3 specular; // スペキュラ色
		float specularity; // スペキュラの強さ（乗算値）
		DirectX::XMFLOAT3 ambient; // アンビエント色
	};

	struct AdditionalMaterial// それ以外のマテリアルデータ
	{
		std::string texPath; // テクスチャファイルパス
		int toonIdx; // トゥーン番号
		bool edgeFlg; // マテリアルごとの輪郭線フラグ
	};

	struct Material// 全体をまとめるデータ
	{
		unsigned int indicesNum; // インデックス数
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};
	unsigned int _materialNum; // マテリアル数
	std::vector<Material> materials;
	Microsoft::WRL::ComPtr<ID3D12Resource> _materialBuff = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _toonResources;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialDescHeap = nullptr;//マテリアルヒープ(5個ぶん)

	//頂点
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _idxBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	// 座標変換
	struct Transform {
		//内部に持ってるXMMATRIXメンバが16バイトアライメントであるため
		//Transformをnewする際には16バイト境界に確保する
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};
	Transform _transform;
	DirectX::XMMATRIX* _mappedMatrices = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformMat = nullptr;//座標変換行列(今はワールドのみ)
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _transformHeap = nullptr;//座標変換ヒープ

	//ボーン関連
	std::vector<DirectX::XMMATRIX>_boneMatrices;//GPUへコピーするためのボーン情報

	struct BoneNode{
		int boneIdx;		//ボーンインデックス
		DirectX::XMFLOAT3 startPos;//ボーン基準点（回転中心）
		std::vector<BoneNode*>children;//子ノード
	};
	std::map<std::string, BoneNode>_boneNodeTable;//名前で骨を検索できるように
	
	void RecursiveMatrixMultipy(BoneNode& node, const DirectX::XMMATRIX& mat);

	float _angle;//テスト用Y軸回転

public:
	// 初期化の部分処理
	HRESULT LoadPMDFile(const char* path);//PMDファイルのロード
	void CreateMaterialData();//読み込んだマテリアルをもとにマテリアルバッファを作成
	void CreateMaterialAndTextureView();//マテリアル＆テクスチャのビューを作成
	void CreateTransformView();

public:
	PMDActor(const char* filepath, PMDRenderer& renderer);
	~PMDActor();

	///クローンは頂点およびマテリアルは共通のバッファを見るようにする
	PMDActor* Clone();

	void Update();
	void Draw();
};

